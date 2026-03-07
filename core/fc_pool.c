/**
 * @file fc_pool.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-09-26
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "fc_compiler.h"
#include "fc_config.h"
#include "fc_pool.h"

#include "fc_arch.h"  //提供原子操作

//+********************************* 辅助宏 **********************************/
// 结束标记
#define FC_POOL_TAG_END 0x0

// 使用标记
#define FC_POOL_TAG_USE 0x1

// 暂时不实现内存溢出检测标记
#if 0
// 内存溢出检测金丝雀
    #define FC_POOL_CANARY_MAGIC_WORD 0xDEADBEEF
static const uint32_t _fc_pool_canary_magic_word = FC_POOL_CANARY_MAGIC_WORD;
#endif

//+********************************* 可配置项 **********************************/
#ifndef fc_assert
    #define fc_assert(x) ((void)(0))
#endif  //\ fc_assert

// 动态内存API一般是全部一起重定义
#ifndef FC_LIB_MALLOC
    #include <stdlib.h>
    #define FC_LIB_MALLOC malloc
#endif

#ifndef FC_LIB_FREE
    #include <stdlib.h>
    #define FC_LIB_FREE free
#endif

#ifndef FC_LIB_REALLOC
    #include <stdlib.h>
    #define FC_LIB_REALLOC realloc
#endif

#ifndef FC_ATOMIC_SCOPE
    #define FC_ATOMIC_SCOPE
#endif

#ifndef FC_POOL_ALLOC_FAIL_HOOK
    #define FC_POOL_ALLOC_FAIL_HOOK(pool) ((void)(0))
#endif

#ifndef MIN
    #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
    #define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ABS
    #define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

//+*********************************  **********************************/

/**
 * @brief 内存池初始化
 *
 * @param pool
 * @param mem
 * @param mem_size
 * @param block_size
 * @return int 可使用内存块数,返回负数表示失败,请检查参数合法性
 */
int fc_pool_init(fc_pool_t *pool, void *mem, size_t mem_size, size_t block_size)
{
    fc_assert(pool != NULL);
    fc_assert(mem != NULL);

    uint8_t          *start_addr = (uint8_t *)mem;
    size_t            align = sizeof(size_t);
    size_t            per_block_size = block_size + sizeof(fc_pool_header_t);
    fc_pool_header_t *node = NULL;

    memset(pool, 0, sizeof(fc_pool_t));

    pool->mem_start = mem;
    pool->mem_end = (void *)((size_t)mem + mem_size);
#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    pool->alloc_cb = NULL;  // 默认不绑定,需要外部调用FC_POOL_CATCH_ALLOC_CB进行绑定
#endif

    // 如果start_addr不对齐sizeof(size_t),则对齐
    if (((size_t)start_addr) % align != 0)
    {
        start_addr = (void *)(((size_t)start_addr + align - 1) & ~(align - 1));
    }

    {  // 约束切块大小
        // per_block_size大小至少为一个头部+sizeof(size_t)为了对齐
        if (per_block_size < (sizeof(fc_pool_header_t) + sizeof(size_t)))
        {
            per_block_size = sizeof(fc_pool_header_t) + sizeof(size_t);
        }

        // per_block_size大小为sizeof(size_t)的向上取整倍数
        if (per_block_size % align != 0)
        {
            per_block_size = (per_block_size + align - 1) & ~(align - 1);
        }
    }

    if (mem_size < per_block_size || mem == NULL)
    {
        return -1;  // 检查参数合法性
    }

    pool->block_size = per_block_size - sizeof(fc_pool_header_t);
    pool->fifo_used.next = NULL;
    pool->fifo_used.tail = NULL;
    pool->list_free.record_now = 0;  // 初始化空闲块数

    {
        node = &(pool->list_free);

        for (size_t i = (size_t)start_addr; i <= (size_t)mem + mem_size - per_block_size; i += per_block_size)
        {
            node->next = (fc_pool_header_t *)i;  // 链接下一个节点
            node = node->next;                   // 移动到下一个节点
            node->next = NULL;                   // 初始化下一个节点指向NULL
            node->linear_last = node;            // 指向自己,自旋指示当前处于连续内存块
            // pool->list_free.record_now++;        // 统计内存块数
        }

        pool->list_free.next->linear_last = node;  // 第一个节点线性节点指向最后一个节点
        pool->list_free.record_now = (((size_t)mem + mem_size) - (size_t)(start_addr)) / per_block_size;

        pool->sort_free_enable = false;  // 默认不启用排序释放内存
    }

    pool->record_min = pool->list_free.record_now;

    return ((int)pool->list_free.record_now);
}

/**
 * @brief
 *
 * @param pool
 * @param sort_free
 */
void fc_pool_sort_enable(fc_pool_t *pool, bool sort_free)
{
    fc_assert(pool != NULL);
    pool->sort_free_enable = sort_free;
}

//+********************************* 基础信息查询 **********************************/

/**
 * @brief 获取内存池最小记录(剩余块)
 *
 * @param pool
 * @return size_t
 */
size_t fc_pool_record_min(fc_pool_t *pool)
{
    fc_assert(pool != NULL);
    return pool->record_min;
}

/**
 * @brief 获取内存池当前剩余块数
 *
 * @param pool
 * @return size_t
 */
size_t fc_pool_record_now(fc_pool_t *pool)
{
    fc_assert(pool != NULL);
    return pool->list_free.record_now;
}

//+********************************* 内存块状态管理及链式操作 **********************************/

/**
 * @brief 标记当前内存块已使用大小
 *
 * @param ptr
 * @param used_size
 * @return true
 * @return false
 */
bool fc_pool_mark_used(void *ptr, size_t used_size)
{
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = fc_pool_rewind_header(ptr);  // 从ptr指向的内存块开始,返回内存块头部地址

    // 已经是最后一块内存且大小足够
    if ((FC_POOL_TAG_END == node->link_flag) || (node->used_size >= used_size))
    {
        node->used_size = (used_size > node->used_size) ? node->used_size : used_size;
        return true;
    }

    return false;
}

/**
 * @brief 标记链式内存块的最后一块,这个API基本不需要调用
 *
 * @param ptr 链式内存块的头部或者任意一块内存的用户起始地址
 */
void fc_pool_end(void *ptr)
{
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = fc_pool_rewind_header(ptr);  // 从ptr指向的内存块开始,返回内存块头部地址

    while (node->next)  // 找到尾部
    {
        node = node->next;
    }

    node->link_flag = FC_POOL_TAG_END;  // 标记为最后一块
}

/**
 * @brief 连接两个非连续内存块,形成链式内存块
 *
 * @param front 需要链接的前一块内存的用户起始地址,可以是链式内存块的任意一块
 * @param back 需要链接的后一块内存的用户起始地址,
 */
void fc_pool_link(void *front, void *back)
{
    fc_assert(front != NULL);
    fc_assert(back != NULL);

    fc_pool_header_t *node_front = fc_pool_rewind_header(front);  // 从front指向的内存块开始,返回内存块头部地址
    fc_pool_header_t *node_back = fc_pool_rewind_header(back);    // 从back指向的内存块开始,返回内存块头部地址

    while (node_front->next)  // 找到front的尾部
    {
        node_front = node_front->next;
    }

    node_front->link_flag = !FC_POOL_TAG_END;  // 取消front的最后一块标记
    node_back->link_flag = FC_POOL_TAG_END;    // 标记back的最后一块

    node_front->next = node_back;
}

// 反转内存
static inline void __reverse_memory(uint8_t *start, uint8_t *end)
{
    uint8_t temp = 0;
    while (start < end)
    {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}

/**
 * @brief front和back是逻辑上的先后,front物理地址不一定在back之前,但必须是连续内存
 *
 * @param pool
 * @param front
 * @param back
 * @return true
 * @return false
 */
bool fc_pool_merge(fc_pool_t *pool, void *front, void *back)
{
    fc_assert(pool != NULL);
    fc_assert(front != NULL);
    fc_assert(back != NULL);

    // 必须都是连续内存
    if (!fc_pool_linear_check(front) || !fc_pool_linear_check(back))
    {
        return false;
    }

    fc_pool_header_t *node_front = fc_pool_rewind_header(front);
    fc_pool_header_t *node_back = fc_pool_rewind_header(back);
    size_t            node_size = fc_pool_node_size(pool);

    // 检查是否相邻
    size_t offset;
    if ((size_t)node_front < (size_t)node_back)  // front内存块在back内存块之前
    {
        offset = node_back - node_front;
        if ((node_front->block_count + node_back->block_count) * node_size != offset)
        {
            return false;
        }
        node_front->block_count += node_back->block_count;
        node_front->used_size += node_back->used_size;
        memmove(((uint8_t *)front + node_front->used_size), (uint8_t *)back, node_back->used_size);

        return true;
    }
    else if ((size_t)node_front > (size_t)node_back)  // back内存块在front内存块之前
    {
        offset = node_front - node_back;
        if ((node_front->block_count + node_back->block_count) * node_size != offset)
        {
            return false;
        }

        // 保存原始数据大小
        size_t front_data_size = node_front->used_size;
        size_t back_data_size = node_back->used_size;
        size_t gap = offset - back_data_size;
        // 数据区域
        uint8_t *front_data = (uint8_t *)front;
        uint8_t *buffer_head = (uint8_t *)back;  // back地址空间在起点

        // 合并到前面的块（node_back）
        node_back->block_count += node_front->block_count;
        node_back->used_size += node_front->used_size;

        // 当前:[back_data][back_leave][header][front_data][front_leave],其中 front_leave/back_leave 可能为0,back_data可能大于front_data+front_leave
        // 目标:[front_data][back_data]

        // back_data开始 反转到 front_data结尾
        __reverse_memory(buffer_head, front_data + front_data_size - 1);
        // front_data反转自身,此时front_data已经在头部位置(buffer_head)
        __reverse_memory(buffer_head, buffer_head + front_data_size - 1);
        // back_data反转自身,此时反转的back_data位置为buffer_head+(front_data_size - gap)
        __reverse_memory(buffer_head + front_data_size - gap, buffer_head + back_data_size - 1);

        // 合并back_data到front_data后面
        memmove(buffer_head + front_data_size, buffer_head + front_data_size + gap, back_data_size);

        return true;
    }

    // 同一个内存块?返回失败
    return false;
}

/**
 * @brief
 *
 * @param ptr
 * @return true
 * @return false
 */
bool fc_pool_linear_check(void *ptr)
{
    fc_assert(ptr != NULL);
    fc_pool_header_t *node = fc_pool_rewind_header(ptr);
    return (NULL == node->next);  // 不存在后续节点
}

/**
 * @brief 遍历链式非连续内存块,不会释放内存,自行确保函数的线程安全
 *
 * @param ptr 链式非连续内存块的头部的用户起始地址
 * @param walker
 * @param user
 */
void fc_pool_walk(void *ptr, fc_pool_walker_t walker, void *user)
{
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = fc_pool_rewind_header(ptr);  // 从ptr指向的内存块开始,返回内存块头部地址
    bool              end_flag = false;

    while (node)
    {
        if ((FC_POOL_TAG_END == node->link_flag) || (NULL == node->next))
        {
            end_flag = true;
        }

        if (walker)
        {
            walker(end_flag, fc_pool_skip_header(node), node->used_size, user);
        }

        if (end_flag)
        {
            break;
        }
        node = node->next;
    }
}

/**
 * @brief 遍历整个fifo used队列链表,遍历后释放内存
 *
 * @param pool
 * @param walker
 * @param user
 */
void fc_pool_fifo_walk(fc_pool_t *pool, fc_pool_walker_t walker, void *user)
{
    fc_assert(pool != NULL);

    void *ptr = fc_header_fifo_pop(&(pool->fifo_used));

    while (NULL != ptr)
    {
        fc_pool_walk(ptr, walker, user);
        fc_pool_free(pool, ptr);
        ptr = fc_header_fifo_pop(&(pool->fifo_used));
    }
}

//+********************************* 类似标准内存分配 **********************************/

/**
 * @brief 内存分配,从list_free剥离,静态内存区内O(1)复杂度
 *
 * @param pool
 * @param size
 * @return void* 返回内存地址,跳过头部,如果分配失败返回NULL,用户可以直接使用的内存地址及大小(*size)
 */
void *fc_pool_alloc(fc_pool_t *pool, size_t *size)
{
    fc_assert(pool != NULL);
    fc_assert(size != NULL);

    void             *ret_ptr = NULL;
    fc_pool_header_t *node = NULL;

    {
        FC_ATOMIC_SCOPE
        {
            node = pool->list_free.next;
            if (node)
            {
                pool->list_free.next = node->next;  // 移动空闲链表头部
                pool->list_free.record_now--;       // 统计空闲块数
                pool->record_min = MIN(pool->record_min, pool->list_free.record_now);
                if ((size_t)(node->linear_last) != (size_t)node)  // 非自旋节点,证明下一个节点跟这个节点是连续的(一定存在下一个节点),无需判断(NULL != node->next)
                {
                    node->next->linear_last = node->linear_last;  // 使下一个节点指向连续内存块的最后一个节点
                }
            }
        }
    }

    if (node)  // 有空闲内存
    {
        node->next = NULL;  // 不指向任何节点

        {
            node->used_size = pool->block_size;  // 默认标记为全部使用
            node->link_flag = FC_POOL_TAG_END;   // 默认标记为最后一块
            node->block_count = 1;               // 只用到了1个内存块
        }

        ret_ptr = fc_pool_skip_header(node);  // 返回内存地址,跳过头部

        *size = pool->block_size;  // 返回可用大小
    }

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    else if (pool->alloc_cb)
    {
        // 调用用户自定义的内存分配回调函数
        if (pool->alloc_cb(FC_POOL_DYNAMIC_MALLOC, (fc_pool_dynamic_mem_t *)&ret_ptr, fc_pool_node_size(pool)))
        {
            ret_ptr = fc_pool_skip_header(ret_ptr);  // 返回内存地址,跳过头部
            node = fc_pool_rewind_header(ret_ptr);   // 从ret_ptr指向的内存块开始,返回内存块头部地址
            {
                node->used_size = pool->block_size;  // 默认标记为全部使用
                node->link_flag = FC_POOL_TAG_END;   // 默认标记为最后一块
                node->block_count = 1;               // 只用到了1个内存块
            }

            *size = pool->block_size;  // 返回可用大小
        }
        else
        {
            ret_ptr = NULL;
            FC_POOL_ALLOC_FAIL_HOOK(pool);

            // 分配失败将size置0,防止误用
            *size = 0;  // 也可以不用管
        }
    }
#endif

    else
    {
        FC_POOL_ALLOC_FAIL_HOOK(pool);

        // 分配失败将size置0,防止误用
        *size = 0;  // 也可以不用管
    }

    return ret_ptr;
}

/**
 * @brief 释放内存,添加到list_free头部,自身静态内存区内O(n)复杂度,n为链式内存块的块数,影响非常小
 *
 * @param pool
 * @param ptr
 */
void fc_pool_free(fc_pool_t *pool, void *ptr)
{
    fc_assert(pool != NULL);
    fc_assert(ptr != NULL);

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    fc_pool_header_t *dynamic_head = NULL;
    fc_pool_header_t *dynamic_tail = NULL;
#endif

    fc_pool_header_t *static_head = NULL;
    fc_pool_header_t *static_tail = NULL;
    fc_pool_header_t *node = NULL;
    fc_pool_header_t *link_node = NULL;
    size_t            link_count = 0;
    size_t            block_count = 0;
    size_t            node_size = fc_pool_node_size(pool);

    // 找到头部
    node = fc_pool_rewind_header(ptr);  // 从ptr指向的内存块开始,返回内存块头部地址

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    while (node)
    {
        // 如果内存地址不在内存池静态内存区内
        if ((size_t)node < (size_t)pool->mem_start ||
            (size_t)node >= ((size_t)pool->mem_end))
        {
            if (dynamic_head == NULL)
            {
                dynamic_head = node;
                dynamic_tail = node;
            }
            else
            {
                dynamic_tail->next = node;
                dynamic_tail = node;
            }

            node = node->next;          // node移动到下一个节点
            dynamic_tail->next = NULL;  // 断开这个节点与后面的链接
        }
        else
        {
            if (static_head == NULL)
            {
                static_head = node;
                static_tail = node;
            }
            else
            {
                static_tail->next = node;
                static_tail = node;
            }

            node = node->next;         // node移动到下一个节点
            static_tail->next = NULL;  // 断开这个节点与后面链接

            if (static_tail->block_count > 1)
            {  // 拆分节点
                link_node = static_tail;

                link_count = link_node->block_count;  // 先记录下来,节点自旋之后会覆盖
                block_count += link_count;
                link_node->linear_last = link_node;  // 节点自旋

                for (; link_count > 1; link_count--)
                {
                    link_node->next = (fc_pool_header_t *)((uint8_t *)link_node + node_size);  // 拆分节点
                    link_node = link_node->next;                                               // 移动到下一个节点
                    link_node->linear_last = link_node;                                        // 节点自旋
                    link_node->next = NULL;                                                    // 初始化下一个节点指向NULL
                }
                static_tail->linear_last = link_node;  // 指向最后一个连续节点

                static_tail = link_node;  // 更新static_tail
            }
        }
    }
#else
    static_head = node;
    {                             // 拆分节点
        link_node = static_head;  // 从头遍历
        do
        {
            link_count = link_node->block_count;
            block_count += link_count;
            if (link_count <= 1)
            {
                static_tail = link_node;             // 更新static_tail,最后赋值保证tail是最后一个节点就行
                link_node->linear_last = link_node;  // 节点自旋
                link_node = link_node->next;         // 移动到下一个节点
            }
            else  // 需要拆分节点
            {
                static_tail = link_node;  // 前面用tail做临时变量记录一下这一次起始的第一个节点
                node = link_node->next;   // 记住下一个节点
                for (; link_count > 1; link_count--)
                {
                    link_node->next = (fc_pool_header_t *)((uint8_t *)link_node + node_size);  // 拆分节点
                    link_node = link_node->next;                                               // 移动到下一个节点
                    link_node->linear_last = link_node;                                        // 节点自旋
                    link_node->next = NULL;                                                    // 初始化下一个节点指向NULL
                }
                link_node->next = node;                // 链接下一个节点
                static_tail->linear_last = link_node;  // 这一次起始的第一个节点指向最后一个连续节点
                static_tail = link_node;               // 更新static_tail,最后赋值保证tail是最后一个节点就行
                link_node = node;
            }
        } while (link_node);
    }
#endif

    if (true == pool->sort_free_enable)  // 排序释放
    {
        fc_pool_header_t *after = NULL;
        fc_pool_header_t *prev = NULL;
        fc_pool_header_t *prev_first = NULL;
        link_node = static_head;
        while (link_node)
        {
            node = link_node->next;  // 记住下一个节点
            block_count = 1;         // 用与指示当前连续块的个数

            static_tail = link_node;  // tail指针这里用不到,用来当临时变量使用
            for (;;)
            {
                link_node->linear_last = static_tail;  // 头节点始终指向最后连续的最后一个节点
                if (node && (size_t)node == (size_t)static_tail + node_size)
                {
                    // 后面的节点跟这里是连续的
                    static_tail = node;                             // 移动到下一个节点
                    if ((size_t)node->linear_last == (size_t)node)  // 自旋节点
                    {
                        block_count++;
                        node = node->next;  // 记录下一个节点
                    }
                    else
                    {
                        block_count += ((size_t)node->linear_last - (size_t)node) / node_size + 1;
                        static_tail = node->linear_last;  // 移动到最后一个连续节点
                        node->linear_last = node;         // 节点自旋
                        node = static_tail->next;         // 记录下一个节点
                    }
                    continue;
                }
                else
                {
                    // static_tail->next = NULL;  // 断开这个节点与后面的链接,无所谓
                    break;  // 一定在这里退出
                }
            }

            // 开始合并到空闲链表
            {
                after = NULL;
                prev = NULL;
                prev_first = NULL;
                FC_ATOMIC_SCOPE
                {
                    after = pool->list_free.next;
                    // 找到合适的位置插入,并判断能否与前后节点合并
                    for (;;)  // 最坏情况O(n)
                    {
                        if ((size_t)after > (size_t)link_node)  // 找到指定位置了
                        {
                            pool->list_free.record_now += block_count;  // 空闲块数更新
                            static_tail->next = after;                  // 最后一块链接到当前块

                            // 先判断跟后面是否连续
                            if ((size_t)(after) == (size_t)static_tail + node_size)
                            {
                                link_node->linear_last = after->linear_last;  // 头结点指向新的最后一个连续节点
                                after->linear_last = after;                   // 自旋
                            }
                            if (prev)
                            {
                                prev->next = link_node;
                                // 后判断跟前面是否连续
                                if (NULL != prev_first && (size_t)(link_node) == (size_t)prev + node_size)
                                {
                                    prev_first->linear_last = link_node->linear_last;  // 更新最前面节点指向的最后一个连续节点
                                    link_node->linear_last = link_node;                // 自旋
                                }
                            }
                            else
                            {
                                pool->list_free.next = link_node;
                            }
                            break;  // 合并完了退出
                        }
                        else if (NULL == after)  // 后面已经没有节点了,只能插入到最后
                        {
                            pool->list_free.record_now += block_count;  // 空闲块数更新
                            static_tail->next = after;                  // 最后一块链接到当前块
                            if (prev)
                            {
                                prev->next = link_node;
                                // 判断是否与前一个节点连续
                                if (NULL != prev_first && (size_t)(link_node) == (size_t)prev + node_size)
                                {
                                    prev_first->linear_last = link_node->linear_last;  // 更新最前面节点指向的最后一个连续节点
                                    link_node->linear_last = link_node;                // 自旋
                                }
                            }
                            else
                            {
                                pool->list_free.next = link_node;
                            }
                            break;  // 合并完了退出
                        }

                        prev_first = after;         // 记录前一个连续内存的第一个节点
                        prev = after->linear_last;  // 跳到最后一个连续节点,必不为NULL
                        after = prev->next;         // 跳到最后一个连续节点的下一个节点
                    }
                }
            }

            link_node = node;
        }
    }
    else  // 不排序,直接链接到空闲链表
    {
        if (static_head)
        {
            FC_ATOMIC_SCOPE
            {
                static_tail->next = pool->list_free.next;  // 尾部指向空闲链表头部
                pool->list_free.next = static_head;        // 空闲链表头部指向新释放的内存块头部
                pool->list_free.record_now += block_count;
            }
        }
    }

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC

    while (dynamic_head)  // 如果是动态分配的内存,调用用户自定义的内存释放回调函数释放内存
    {
        node = dynamic_head->next;  // 先记录下一个节点

        // 根据地址找到起始位置
        // fc_pool_dynamic_mem_t mem = (fc_pool_dynamic_mem_t)dynamic_head;
        // pool->alloc_cb(FC_POOL_DYNAMIC_FREE, &mem, fc_pool_node_size(pool));

        pool->alloc_cb(FC_POOL_DYNAMIC_FREE, (fc_pool_dynamic_mem_t *)&dynamic_head, fc_pool_node_size(pool));  // 释放动态内存块

        dynamic_head = node;
    }

#endif
}

/**
 * @brief
 *
 * @param pool
 * @param size
 * @return void*
 */
void *fc_pool_malloc(fc_pool_t *pool, size_t size)
{
    fc_assert(pool != NULL);
    // fc_assert(size <= 0xFFFF);  // 单块最大大小,取决于node->size和node->block_count

    void             *ret_ptr = NULL;
    fc_pool_header_t *node = NULL;
    size_t            block_count = 0;
    size_t            serial_leap = 0;
    size_t            node_size = 0;

    if (size <= pool->block_size)
    {
        ret_ptr = fc_pool_alloc(pool, &node_size);  // 小于一块内存的需求直接分配O(1)
        fc_pool_mark_used(ret_ptr, size);           // 标记实际使用大小

        return ret_ptr;
    }

#if 0
    if (false == pool->sort_free_enable)
    {
    #if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
        if (pool->alloc_cb)
        {
            // 调用用户自定义的内存分配回调函数
            if (pool->alloc_cb(FC_POOL_DYNAMIC_MALLOC, (fc_pool_dynamic_mem_t *)&ret_ptr, (size + sizeof(fc_pool_header_t))))
            {
                node = (fc_pool_header_t *)ret_ptr;
                {
                    node->used_size = size;                    // 连续块内存已使用的大小
                    node->link_flag = FC_POOL_TAG_END;         // 默认标记为最后一块
                    node->block_count = (uint8_t)block_count;  // 本块内存有多少个连续内存块
                }
                ret_ptr = fc_pool_skip_header(node);  // 返回内存地址,跳过头部
                fc_pool_mark_used(ret_ptr, size);     // 标记实际使用大小
            }
            else
            {
                ret_ptr = NULL;
                FC_POOL_ALLOC_FAIL_HOOK(pool);
            }
        }
    #endif

        return ret_ptr;  // 纯静态分配时为保证free的O1复杂度,超过1块时必然失败
    }
#endif

    node_size = fc_pool_node_size(pool);                                          // 加上头部大小
    block_count = (size + sizeof(fc_pool_header_t) + node_size - 1) / node_size;  // 需要的块数(向上取整)
    serial_leap = (block_count - 1) * node_size;                                  // 得到总跨度,不计入最后一个节点的大小,简化循环中的判断计算

    {  // 开始查找符合要求的节点
        FC_ATOMIC_SCOPE
        {
            node = pool->list_free.next;
            while (node)  // 最坏理论O(n),n为当前空闲块数
            {
                if ((size_t)(node->linear_last) - (size_t)node >= serial_leap)
                {
                    fc_pool_header_t *node_temp = (fc_pool_header_t *)((uint8_t *)node + serial_leap);  // 得到分配出去的最后一个节点
                    node_temp = node_temp->next;                                                        // 通过节点获取到下一个空闲节点

                    pool->list_free.next = node_temp;           // 移动空闲链表头部
                    pool->list_free.record_now -= block_count;  // 统计空闲块数
                    pool->record_min = MIN(pool->record_min, pool->list_free.record_now);

                    if (node_temp && ((size_t)(node->linear_last) - (size_t)node > serial_leap))
                    {
                        node_temp->linear_last = node->linear_last;  // 截断,使下一个节点指向连续内存块的最后一个节点
                    }

                    break;  // 找到符合要求的节点,跳出循环
                }
                else
                {
                    // 节点跳过当前连续块
                    node = node->linear_last->next;
                }
            }
        }
    }

    if (node)  // 找到了内存块
    {
        node->next = NULL;  // 不指向任何节点

        {
            node->used_size = size;                    // 连续块内存已使用的大小
            node->link_flag = FC_POOL_TAG_END;         // 默认标记为最后一块
            node->block_count = (uint8_t)block_count;  // 本块内存有多少个连续内存块
        }

        ret_ptr = fc_pool_skip_header(node);  // 返回内存地址,跳过头部
        fc_pool_mark_used(ret_ptr, size);     // 标记实际使用大小
    }

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    else if (pool->alloc_cb)
    {
        // 调用用户自定义的内存分配回调函数
        if (pool->alloc_cb(FC_POOL_DYNAMIC_MALLOC, (fc_pool_dynamic_mem_t *)&ret_ptr, (size + sizeof(fc_pool_header_t))))
        {
            node = (fc_pool_header_t *)ret_ptr;
            {
                node->used_size = size;                    // 连续块内存已使用的大小
                node->link_flag = FC_POOL_TAG_END;         // 默认标记为最后一块
                node->block_count = (uint8_t)block_count;  // 本块内存有多少个连续内存块
            }
            ret_ptr = fc_pool_skip_header(node);  // 返回内存地址,跳过头部
            fc_pool_mark_used(ret_ptr, size);     // 标记实际使用大小
        }
        else
        {
            ret_ptr = NULL;
            FC_POOL_ALLOC_FAIL_HOOK(pool);
        }
    }
#endif

    return ret_ptr;
}

/**
 * @brief
 *
 * @param pool
 * @param ptr
 * @param size
 * @return void*
 */
void *fc_pool_realloc(fc_pool_t *pool, void *ptr, size_t size)
{
    fc_assert(pool != NULL);

    // 特殊情况处理
    if (NULL == ptr && 0 == size)
    {
        return NULL;
    }

    if (NULL == ptr)
    {
        return fc_pool_malloc(pool, size);
    }

    if (0 == size)
    {
        fc_pool_free(pool, ptr);
        return NULL;
    }

    int32_t           need_count = 0;
    void             *ret_ptr = NULL;
    fc_pool_header_t *node_free = NULL;
    fc_pool_header_t *node_end = NULL;
    fc_pool_header_t *node_prev = NULL;
    fc_pool_header_t *node = fc_pool_rewind_header(ptr);
    size_t            node_size = fc_pool_node_size(pool);

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    // 如果内存地址不在内存池静态内存区内
    if ((size_t)node < (size_t)pool->mem_start ||
        (size_t)node >= ((size_t)pool->mem_end))
    {
        if (pool->alloc_cb)
        {
            // 调用用户自定义的内存分配回调函数
            if (pool->alloc_cb(FC_POOL_DYNAMIC_REALLOC, (fc_pool_dynamic_mem_t *)&node, (size + sizeof(fc_pool_header_t))))
            {
                ret_ptr = fc_pool_skip_header(node);  // 返回内存地址,跳过头部
            }
        }
    }
    else
#endif
    {
        need_count = (int32_t)(((size + sizeof(fc_pool_header_t) + node_size - 1) / node_size) - node->block_count);  // 还需要多少个连续内存块

        if (1 == need_count)  // 只需要一个内存块
        {
            node = (fc_pool_header_t *)((uint8_t *)node + node->block_count * node_size);  // 直接定位到需要取出的内存块第一个节点位置
            {
                FC_ATOMIC_SCOPE
                {
                    // 遍历空闲链表看需要取出的内存块是否还在空闲链表中
                    node_free = pool->list_free.next;
                    node_prev = &(pool->list_free);
                    while (node_free)  // 最坏O(n)
                    {
                        if ((size_t)node_free == (size_t)node)  // 链式内存是按顺序排列的,如果这块内存还未使用必定是一块链的起始
                        {
                            // 找到这块内存了
                            node_prev->next = node_free->next;  // 从空闲链表中移除

                            if (node_free->linear_last != node_free)  // 非自旋节点,证明下一个节点跟这个节点是连续的(一定存在下一个节点)
                            {
                                node_free->next->linear_last = node_free->linear_last;  // 使下一个节点指向连续内存块的最后一个节点
                            }

                            ret_ptr = ptr;  // 赋值用于下面判断realloc是否完成,不能ATOMIC域内直接return
                            break;
                        }

                        node_prev = node_free->linear_last;
                        node_free = node_free->linear_last->next;

                        if ((size_t)node_free > (size_t)node)  // 遍历到需要内存后面了证明需要分配的内存不再空闲链表中
                        {
                            break;
                        }
                    }
                }
            }
        }
        else if (need_count > 1)  // 需要两个及以上连续内存块
        {
            node = (fc_pool_header_t *)((uint8_t *)node + node->block_count * node_size);     // 直接定位到需要取出的内存块第一个节点位置
            node_end = (fc_pool_header_t *)((uint8_t *)node + (need_count - 1) * node_size);  // 得到需要分配出去的最后一个节点位置

            {
                FC_ATOMIC_SCOPE
                {
                    // 遍历空闲链表看需要取出的内存块是否还在空闲链表中
                    node_free = pool->list_free.next;
                    node_prev = &(pool->list_free);
                    while (node_free)  // 最坏O(n)
                    {
                        if ((size_t)node_free == (size_t)node && (size_t)(node_free->linear_last) >= (size_t)node_end)  // 链式内存是按顺序排列的,如果这块内存还未使用必定是一块链的起始,从这块内存开始到node_end都是空闲的
                        {
                            // 找到这块内存了
                            node_prev->next = node_end->next;  // 从空闲链表中移除

                            if (node_free->linear_last != node_end)  // 起始节点指向的最后一个连续节点不是分配出去的最后一个节点
                            {
                                node_end->next->linear_last = node_free->linear_last;  // 使下一个节点指向连续内存块的最后一个节点
                            }

                            ret_ptr = ptr;  // 赋值用于下面判断realloc是否完成,不能ATOMIC域内直接return
                            break;
                        }

                        node_prev = node_free->linear_last;
                        node_free = node_free->linear_last->next;

                        if ((size_t)node_free > (size_t)node)  // 遍历到需要内存后面了证明需要分配的内存不再空闲链表中
                        {
                            break;
                        }
                    }
                }
            }
        }
        else if (0 == need_count)  // 当前链上还有空闲内存仅仅修改标记大小
        {
            // 保持不变,仅仅修改标记大小
            fc_pool_mark_used(ptr, size);  // 标记实际使用大小
            return ptr;
        }
        else if (need_count < 0)  // 还要释放部分内存,正常使用比较少
        {
            node = (fc_pool_header_t *)((uint8_t *)node - (node->block_count + need_count) * node_size);  // 定位到需要释放的内存块第一个节点位置
            ret_ptr = fc_pool_skip_header(node);
            fc_pool_free(pool, ret_ptr);
            ret_ptr = ptr;  // 赋值用于下面判断realloc是否完成
        }
    }

    if (ret_ptr)  // 执行到这儿且不为空证明正常完成了realloc操作(基于原始内存)
    {
        fc_pool_mark_used(ret_ptr, size);  // 标记实际使用大小
    }
    else
    {
        ret_ptr = fc_pool_malloc(pool, size);  // 重新分配内存
        if (ret_ptr)                           // 分配成功
        {
            memcpy(ret_ptr, ptr, size);  // 复制内存
            fc_pool_free(pool, ptr);     // 释放旧内存
        }
    }

    return ret_ptr;
}

/**
 * @brief 分配并初始化内存,将所有字节设为0
 *
 * @param pool
 * @param count
 * @param size
 * @return void*
 */
void *fc_pool_calloc(fc_pool_t *pool, size_t count, size_t size)
{
    fc_assert(pool != NULL);

    // 1. 检查参数是否为0
    if (count == 0 || size == 0)
    {
        return NULL;
    }

    // 2. 检查乘法溢出
    // 如果 count * size 会溢出 size_t，则分配失败
    if (count > SIZE_MAX / size)
    {
        // 乘法会溢出，返回 NULL
        return NULL;
    }

    // 3. 计算总大小
    size_t total_size = count * size;

    // 4. 分配内存
    void *ptr = fc_pool_malloc(pool, total_size);
    if (ptr == NULL)
    {
        // 分配失败
        return NULL;
    }

    // 5. 将内存初始化为0
    // 使用 memset 将所有字节设为0
    memset(ptr, 0, total_size);

    return ptr;
}

//+********************************* fc_pool_header_t对象当fifo使用 **********************************/

/**
 * @brief 判断fifo是否为空
 *
 * @param header
 * @return true
 * @return false
 */
bool fc_header_fifo_empty(fc_pool_header_t *header)
{
    fc_assert(header != NULL);
    return (header->next == NULL);
}

/**
 * @brief 将整个链式非连续内存块添加到fifo used链表尾部,O(n)复杂度,n为head_ptr指向的链式内存块的节点数量
 *
 * @param header
 * @param head_ptr 必须为链式内存块的头部(第一块)的用户起始地址
 */
void fc_header_fifo_push(fc_pool_header_t *header, void *head_ptr)
{
    fc_assert(header != NULL);
    fc_assert(head_ptr != NULL);

    fc_pool_header_t *node = fc_pool_rewind_header(head_ptr);
    fc_pool_header_t *node_tail = node;

    do
    {
        if (node_tail->next == NULL)  // 找到这一次链式内存块的最后一块
        {
            break;
        }
        node_tail = node_tail->next;
    } while (node_tail);

    {
        FC_ATOMIC_SCOPE
        {
            // 如果头部为空,则头部也需要指向新节点
            if (header->next == NULL)
            {
                header->next = node;
            }
            else
            {
                header->tail->next = node;  // 尾部块指向新节点
            }

            header->tail = node_tail;  // 更新尾部

            // 无需标记,默认创建的时候已经标记
            // fc_pool_end(fc_pool_skip_header(node_tail));  // 标记最后一块
        }
    }
}

/**
 * @brief 从fifo used链表头部弹出一条链式非连续内存块,O(n)复杂度
 *
 * @param header
 * @return void* 返回链式非连续内存块的头部的用户起始地址,如果fifo used链表为空则返回NULL
 */
void *fc_header_fifo_pop(fc_pool_header_t *header)
{
    fc_assert(header != NULL);

    if (fc_header_fifo_empty(header))
    {
        return NULL;
    }

    fc_pool_header_t *node = NULL;
    fc_pool_header_t *tail = NULL;
    {
        FC_ATOMIC_SCOPE
        {
            if (header->next != NULL)  // 再次判定,避免从上次判定到这里之间其他线程调用过,这里不使用函数调用减少开销
            {
                node = header->next;
                tail = node;
                do
                {
                    if (FC_POOL_TAG_END == tail->link_flag)  // 找到这一次链式内存块的最后一块
                    {
                        break;
                    }

                    if (tail->next == NULL)  // 理论上不可能出现这种情况,前一个判断就会退出
                    {
                        break;
                    }

                    tail = tail->next;
                } while (tail);

                header->next = tail->next;  // 更新头部
                if (header->next == NULL)   // 如果头部为空,则尾部也需要置空
                {
                    header->tail = NULL;
                }

                tail->next = NULL;  // 断开链式内存块
            }
        }
    }

    return (void *)(node ? (uint8_t *)node + sizeof(fc_pool_header_t) : NULL);
}

#if 0
/**
 * @brief 检查内存池是否正常
 *
 * @param pool
 * @return true
 * @return false
 */
bool fc_pool_check(fc_pool_t *pool)
{
    fc_assert(pool != NULL);
    bool              ret = true;
    fc_pool_header_t *node = NULL;
    fc_pool_header_t *temp = NULL;

    {
        FC_ATOMIC_SCOPE
        {
            node = pool->fifo_used.next;
            while (node != NULL)
            {
                temp = node;
                if (pool->sort_free_enable)
                {
                    // 排序还要看内存顺序是否正常
                    if (temp->linear_last != temp)
                    {
                        size_t size = (((size_t)(temp->linear_last) - (size_t)temp)) / fc_pool_node_size(pool);
                        size += 1;
                        for (size_t i = 0; i < size; i++)
                        {
                            ret = (temp->next == (fc_pool_header_t *)((uint8_t *)temp + fc_pool_node_size(pool)));
                            temp = temp->next;

                            if (!ret)
                            {
                                break;
                            }
                        }
                    }

                    if (ret && temp->next != NULL)
                    {
                        ret = ((size_t)temp->next > (size_t)temp) ? true : false;
                    }
                }
                else
                {
                    // 不管排序,只需要看内存块线性链接是否正常
                    if (temp->linear_last != temp)
                    {
                        size_t size = (((size_t)(temp->linear_last) - (size_t)temp)) / fc_pool_node_size(pool);
                        size += 1;
                        for (size_t i = 0; i < size; i++)
                        {
                            ret = (temp->next == (fc_pool_header_t *)((uint8_t *)temp + fc_pool_node_size(pool)));
                            temp = temp->next;

                            if (!ret)
                            {
                                break;
                            }
                        }
                    }
                }

                if (!ret)
                {
                    break;
                }

                node = node->next;
            }
        }
    }

    return ret;
}
#endif

/**
 * @brief 统计fifo used链表中最大连续内存块数
 *
 * @param pool
 * @return size_t
 */
size_t fc_pool_max_linear_count(fc_pool_t *pool)
{
    fc_assert(pool != NULL);

    fc_pool_header_t *node = NULL;
    size_t            max_count = 0;
    size_t            count = 0;
    {
        FC_ATOMIC_SCOPE
        {
            node = pool->list_free.next;
            while (node != NULL)
            {
                if (node->linear_last != node)
                {
                    count = (((size_t)(node->linear_last) - (size_t)node)) / fc_pool_node_size(pool);
                    count += 1;
                }
                else
                {
                    count = 1;
                }

                if (count > max_count)
                {
                    max_count = count;
                }

                node = node->linear_last->next;
                // node = node->next;
            }
        }
    }

    return max_count;
}

/**
 * @brief 统计fifo used链表中最大连续内存块大小
 *
 * @param pool
 * @return size_t
 */
size_t fc_pool_max_linear_size(fc_pool_t *pool)
{
    return (fc_pool_node_size(pool) * fc_pool_max_linear_count(pool) - sizeof(fc_pool_header_t));
}

//+********************************* 提供一份默认的动态内存申请 **********************************/

fc_weak bool fc_pool_dynamic_default(fc_pool_dynamic_type_t type, fc_pool_dynamic_mem_t *mem, size_t size)
{
    bool ret = false;

    switch (type)
    {
    case FC_POOL_DYNAMIC_MALLOC:
    {
        *mem = (fc_pool_dynamic_mem_t)FC_LIB_MALLOC(size);
        ret = (NULL != *mem);
    }
    break;

    case FC_POOL_DYNAMIC_FREE:
    {
        FC_LIB_FREE(*mem);
        // *mem = NULL;
        ret = true;
    }
    break;

    case FC_POOL_DYNAMIC_REALLOC:
    {
        void *ptr = FC_LIB_REALLOC(*mem, size);
        if (NULL != ptr)
        {
            *mem = ptr;
            ret = true;
        }
    }
    break;

    default:
        break;
    }

    return ret;
}

void fc_pool_catch_alloc_cb(fc_pool_t *pool, fc_pool_dynamic_cb_t alloc_cb)
{
#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    fc_assert(pool != NULL);
    pool->alloc_cb = alloc_cb;
#endif
}
