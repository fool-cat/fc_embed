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

#include "fc_compiler.h"
#include "fc_config.h"
#include "fc_pool.h"

#include "fc_arch.h"  //提供原子操作

//+********************************* 可配置项 **********************************/
#ifndef fc_assert
    #define fc_assert(x) ((void)(0))
#endif  //\ fc_assert

// #define FC_POOL_MAGIC_NUM 0xAA
#define FC_POOL_TAG_END 0x1

#ifndef FC_LIB_MALLOC
    #include <stdlib.h>
    #define FC_LIB_MALLOC malloc
#endif

#ifndef FC_LIB_FREE
    #include <stdlib.h>
    #define FC_LIB_FREE free
#endif

#ifndef FC_ATOMIC_SCOPE
    #define FC_ATOMIC_SCOPE
#endif

#ifndef FC_POOL_ALLOC_FAIL_HOOK
    #define FC_POOL_ALLOC_FAIL_HOOK(pool) ((void)(0))
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
    fc_pool_header_t *node = &(pool->list_free);

    memset(pool, 0, sizeof(fc_pool_t));

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    pool->mem_src = mem;
    pool->mem_size = mem_size;
    pool->alloc = NULL;  // 默认不绑定,需要外部调用FC_POOL_CATCH_ALLOC_CB进行绑定
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
    pool->fifo_used.pool.tail = NULL;
    pool->list_free.pool.record_now = 0;  // 初始化空闲块数

    for (size_t i = (size_t)start_addr; i < (size_t)mem + mem_size - per_block_size; i += per_block_size)
    {
        node->next = (fc_pool_header_t *)i;  // 指向下一个节点
        node = node->next;                   // 移动到下一个节点
        node->next = NULL;                   // 初始化下一个节点为空
        pool->list_free.pool.record_now++;   // 统计内存块数
    }

    pool->record_min = pool->list_free.pool.record_now;

    return ((int)pool->list_free.pool.record_now);
}

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

    FC_ATOMIC_SCOPE
    {
        node = pool->list_free.next;
        pool->list_free.next = (node) ? node->next : pool->list_free.next;  // 移动空闲链表头部
        pool->list_free.pool.record_now -= (node) ? 1 : 0;                  // 统计空闲块数
    }

    if (node)  // 有空闲内存
    {
        node->next = NULL;  // 不指向任何节点

        {
            node->pool.tail = NULL;                  // 不指向任何节点,使用这种方式清零联合体
            node->pool.tag.size = pool->block_size;  // 默认标记为全部使用
            node->pool.tag.end = FC_POOL_TAG_END;    // 默认标记为最后一块
            // node->pool.tag.magic = FC_POOL_MAGIC_NUM;  // 魔数
        }

        ret_ptr = (void *)((uint8_t *)node + sizeof(fc_pool_header_t));  // 返回内存地址,跳过头部

        pool->record_min = (pool->list_free.pool.record_now < pool->record_min) ? pool->list_free.pool.record_now : pool->record_min;

        *size = pool->block_size;  // 返回可用大小
    }

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    else if (pool->alloc)
    {
        // 调用用户自定义的内存分配回调函数
        fc_pool_dynamic_mem_t mem = NULL;
        pool->alloc(FC_POOL_DYNAMIC_MALLOC, &mem, (pool->block_size + sizeof(fc_pool_header_t)));

        if (mem)
        {
            ret_ptr = (void *)((uint8_t *)mem + sizeof(fc_pool_header_t));  // 返回内存地址,跳过头部
            *size = pool->block_size;                                       // 返回可用大小
        }
        else
        {
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
    size_t            static_count = 0;

    // 找到头部
    node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    while (node)
    {
        // 如果内存地址不在内存池静态内存区内
        if ((size_t)node < (size_t)pool->mem_src ||
            (size_t)node >= ((size_t)pool->mem_src + pool->mem_size))
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

            static_count++;
            node = node->next;         // node移动到下一个节点
            static_tail->next = NULL;  // 断开这个节点与后面链接
        }
    }
#else
    // 对于纯静态链来说,本身就是链好的,只需要找到最后一个节点即可
    static_head = node;
    static_tail = node;
    static_count++;  // 已经存在一个节点
    while (static_tail->next)
    {
        static_count++;
        static_tail = static_tail->next;
    }
#endif

    if (static_head)  // 静态内存释放仅仅是加入空闲链表,非常快
    {
        FC_ATOMIC_SCOPE
        {
            static_tail->next = pool->list_free.next;  // 尾部指向空闲链表头部
            pool->list_free.next = static_head;        // 空闲链表头部指向新释放的内存块头部
            pool->list_free.pool.record_now += static_count;
        }
    }

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC

    while (dynamic_head && pool->alloc)  // 如果是动态分配的内存,调用用户自定义的内存释放回调函数释放内存
    {
        // 根据地址找到起始位置
        fc_pool_dynamic_mem_t mem = (fc_pool_dynamic_mem_t)((uint8_t *)dynamic_head - sizeof(fc_pool_header_t));

        pool->alloc(FC_POOL_DYNAMIC_FREE, &mem, pool->block_size + sizeof(fc_pool_header_t));  // 释放动态内存块

        dynamic_head = dynamic_head->next;
    }

#endif
}

/**
 * @brief 获取每块内存用户可使用大小(字节)
 *
 * @param pool
 * @return size_t
 */
size_t fc_pool_per_size(fc_pool_t *pool)
{
    fc_assert(pool != NULL);
    return pool->block_size;
}

/**
 * @brief 标记当前内存块已使用大小
 *
 * @param ptr
 * @param used_size
 */
void fc_pool_mark_used(void *ptr, size_t used_size)
{
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));
    node->pool.tag.size = (used_size > node->pool.tag.size) ? node->pool.tag.size : used_size;
}

/**
 * @brief 标记链式内存块的最后一块
 *
 * @param ptr 链式内存块的头部或者任意一块内存的用户起始地址
 */
void fc_pool_end(void *ptr)
{
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));

    while (node->next)  // 找到尾部
    {
        node = node->next;
    }

    node->pool.tag.end = FC_POOL_TAG_END;  // 标记为最后一块
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

    fc_pool_header_t *node_front = (fc_pool_header_t *)((uint8_t *)front - sizeof(fc_pool_header_t));
    fc_pool_header_t *node_back = (fc_pool_header_t *)((uint8_t *)back - sizeof(fc_pool_header_t));

    while (node_front->next)  // 找到front的尾部
    {
        node_front = node_front->next;
    }

    node_front->pool.tag.end = 0;               // 取消front的最后一块标记
    node_back->pool.tag.end = FC_POOL_TAG_END;  // 标记back的最后一块

    node_front->next = node_back;
}

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
    return pool->list_free.pool.record_now;
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

    void  *tail = NULL;
    void  *head = NULL;
    void  *ptr = NULL;
    size_t block_size = 0;
    size_t per_size = fc_pool_per_size(pool);

    head = fc_pool_alloc(pool, &block_size);
    if (head == NULL)
    {
        return NULL;  // 分配失败
    }

    tail = head;
    if (size <= per_size)
    {
        // fc_pool_end(tail);  // 标记最后一块,分配时默认已经标记
        return head;
    }

    for (;;)
    {
        size -= per_size;

        ptr = fc_pool_alloc(pool, &block_size);
        if (ptr == NULL)
        {
            fc_pool_free(pool, head);  // 分配失败,释放已分配的内存
            return NULL;
        }

        fc_pool_link(tail, ptr);  // 连接前后两块内存
        tail = ptr;               // 移动尾部

        if (size <= per_size)
        {
            // fc_pool_end(tail);  // 标记最后一块,分配时默认已经标记
            break;
        }
    }

    return head;
}

/**
 * @brief 等效与从一个char数组的offset位置写入write_size字节数据到链式非连续内存块中,返回实际写入的字节数
 *  将ptr当作连续内存,等效于从(char*)ptr + offset位置写入write_size字节数据
 * @param pool
 * @param ptr
 * @param offset
 * @param write_size
 * @return size_t
 */
size_t fc_pool_write(fc_pool_t *pool, void *ptr, size_t offset, size_t write_size)
{
    fc_assert(pool != NULL);
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));
    size_t            per_size = fc_pool_per_size(pool);

    // 找到offset所在的内存块
    while (offset >= per_size)
    {
        if (node->next == NULL)  // offset超出链式内存块大小
        {
            return 0;
        }
        node = node->next;
        offset -= per_size;
    }

    size_t   copied_size = 0;
    size_t   copy_size = 0;
    uint8_t *src = (uint8_t *)ptr;
    uint8_t *dst = (uint8_t *)node + sizeof(fc_pool_header_t) + offset;
    size_t   left_size = write_size;
    size_t   can_copy_size = per_size - offset;  // 当前块还能写入的大小
    while (left_size > 0)
    {
        copy_size = (left_size > can_copy_size) ? can_copy_size : left_size;
        memcpy(dst, src + copied_size, copy_size);

        copied_size += copy_size;
        left_size -= copy_size;

        if (left_size == 0)  // 写入完成
        {
            break;
        }

        if (node->next == NULL)  // 没有下一块内存了
        {
            break;
        }

        node = node->next;
        dst = (uint8_t *)node + sizeof(fc_pool_header_t);
        can_copy_size = per_size;  // 新块可以全部写入
    }

    return copied_size;
}

/**
 * @brief 从链式非连续内存块中读取数据到ptr,等效与从一个char数组的offset位置读取read_size字节数据到ptr中,返回实际读取的字节数
 * 将ptr当作连续内存,等效于从(char*)ptr + offset位置读取read_size字节数据
 * @param pool
 * @param ptr
 * @param offset
 * @param read_size
 * @return size_t
 */
size_t fc_pool_read(fc_pool_t *pool, void *ptr, size_t offset, size_t read_size)
{
    fc_assert(pool != NULL);
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));
    size_t            per_size = fc_pool_per_size(pool);

    // 找到offset所在的内存块
    while (offset >= per_size)
    {
        if (node->next == NULL)  // offset超出链式内存块大小
        {
            return 0;
        }
        node = node->next;
        offset -= per_size;
    }

    size_t   copied_size = 0;
    size_t   copy_size = 0;
    uint8_t *dst = (uint8_t *)ptr;
    uint8_t *src = (uint8_t *)node + sizeof(fc_pool_header_t) + offset;
    size_t   left_size = read_size;
    size_t   can_copy_size = per_size - offset;  // 当前块还能读取的大小
    while (left_size > 0)
    {
        copy_size = (left_size > can_copy_size) ? can_copy_size : left_size;
        memcpy(dst + copied_size, src, copy_size);

        copied_size += copy_size;
        left_size -= copy_size;

        if (left_size == 0)  // 读取完成
        {
            break;
        }

        if (node->next == NULL)  // 没有下一块内存了
        {
            break;
        }

        node = node->next;
        src = (uint8_t *)node + sizeof(fc_pool_header_t);
        can_copy_size = per_size;  // 新块可以全部读取
    }

    return copied_size;
}

/**
 * @brief 获取链式非连续内存块的总大小(字节)
 *
 * @param pool
 * @param ptr
 * @return size_t
 */
size_t fc_pool_strip_size(fc_pool_t *pool, void *ptr)
{
    fc_assert(pool != NULL);
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));
    size_t            per_size = fc_pool_per_size(pool);
    size_t            total_size = 0;

    while (node)
    {
        total_size += per_size;

        if (node->pool.tag.end == FC_POOL_TAG_END)  // 找到这一次链式内存块的最后一块
        {
            break;
        }

        node = node->next;
    }

    return total_size;
}

/**
 * @brief 获取链式非连续内存块的已使用大小(字节)
 *
 * @param pool
 * @param ptr
 * @return size_t
 */
size_t fc_pool_strip_used(fc_pool_t *pool, void *ptr)
{
    fc_assert(pool != NULL);
    fc_assert(ptr != NULL);

    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));
    size_t            total_used_size = 0;

    while (node)
    {
        total_used_size += node->pool.tag.size;

        if (node->pool.tag.end == FC_POOL_TAG_END)  // 找到这一次链式内存块的最后一块
        {
            break;
        }

        node = node->next;
    }

    return total_used_size;
}

//+********************************* 进阶API **********************************/

/**
 * @brief 判断fifo used链表是否为空
 *
 * @param pool
 * @return true
 * @return false
 */
bool fc_pool_fifo_empty(fc_pool_t *pool)
{
    fc_assert(pool != NULL);
    return (pool->fifo_used.next == NULL);
}

/**
 * @brief 将整个链式非连续内存块添加到fifo used链表尾部,O(n)复杂度
 *
 * @param pool
 * @param head_ptr 必须为链式内存块的头部(第一块)的用户起始地址
 */
void fc_pool_fifo_push(fc_pool_t *pool, void *head_ptr)
{
    fc_assert(pool != NULL);
    fc_assert(head_ptr != NULL);

    // 找到头部
    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)head_ptr - sizeof(fc_pool_header_t));

    FC_ATOMIC_SCOPE
    {
        // 如果头部为空,则头部也需要指向新节点
        if (pool->fifo_used.next == NULL)
        {
            pool->fifo_used.next = node;
            pool->fifo_used.pool.tail = node;
        }
        else
        {
            pool->fifo_used.pool.tail->next = node;  // 尾部块指向新节点
        }

        do
        {
            if (node->next == NULL)  // 找到这一次链式内存块的最后一块
            {
                break;
            }
            node = node->next;
        } while (node);
        pool->fifo_used.pool.tail = node;  // 更新尾部

        // 无需标记,默认创建的时候已经标记
        // fc_pool_end((void *)((uint8_t *)node + sizeof(fc_pool_header_t)));  // 标记最后一块
    }
}

/**
 * @brief 从fifo used链表头部弹出一条链式非连续内存块,O(n)复杂度
 *
 * @param pool
 * @return void* 返回链式非连续内存块的头部的用户起始地址,如果fifo used链表为空则返回NULL
 */
void *fc_pool_fifo_pop(fc_pool_t *pool)
{
    fc_assert(pool != NULL);

    if (fc_pool_fifo_empty(pool))
    {
        return NULL;
    }

    fc_pool_header_t *node = NULL;
    fc_pool_header_t *tail = NULL;

    FC_ATOMIC_SCOPE
    {
        node = pool->fifo_used.next;
        tail = node;
        do
        {
            if (tail->pool.tag.end == FC_POOL_TAG_END)  // 找到这一次链式内存块的最后一块
            {
                break;
            }

            if (tail->next == NULL)  // 理论上不可能出现这种情况,前一个判断就会退出
            {
                break;
            }

            tail = tail->next;
        } while (tail);

        pool->fifo_used.next = tail->next;  // 更新头部
        if (pool->fifo_used.next == NULL)   // 如果头部为空,则尾部也需要置空
        {
            pool->fifo_used.pool.tail = NULL;
        }

        tail->next = NULL;  // 断开链式内存块
    }

    return (void *)(node ? (uint8_t *)node + sizeof(fc_pool_header_t) : NULL);
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

    fc_pool_header_t *node = (fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t));
    bool              end = false;

    while (node)
    {
        if ((FC_POOL_TAG_END == node->pool.tag.end) || (NULL == node->next))
        {
            end = true;
        }

        if (walker)
        {
            walker(end, (uint8_t *)node + sizeof(fc_pool_header_t), node->pool.tag.size, user);
        }

        if (end)
        {
            break;
        }
        node = node->next;
    }
}

/**
 * @brief 遍历整个fifo used队列链表,遍历后释放内存,自行确保函数的线程安全
 *
 * @param pool
 * @param walker
 * @param user
 */
void fc_pool_fifo_walk(fc_pool_t *pool, fc_pool_walker_t walker, void *user)
{
    fc_assert(pool != NULL);

    void *ptr = NULL;

    while (!fc_pool_fifo_empty(pool))
    {
        ptr = fc_pool_fifo_pop(pool);
        fc_pool_walk(ptr, walker, user);
        fc_pool_free(pool, ptr);
    }
}

//+********************************* 提供一份默认的动态内存申请 **********************************/

fc_weak void fc_pool_dynamic_default(fc_pool_dynamic_type_t type, fc_pool_dynamic_mem_t *mem, size_t size)
{
    if (type == FC_POOL_DYNAMIC_MALLOC)
    {
        *mem = (fc_pool_dynamic_mem_t)FC_LIB_MALLOC(size);
    }
    else if (type == FC_POOL_DYNAMIC_FREE)
    {
        FC_LIB_FREE(*mem);
        // *mem = NULL;
    }
}

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC

void fc_pool_catch_alloc_cb(fc_pool_t *pool, fc_pool_dynamic_cb_t alloc_cb)
{
    fc_assert(pool != NULL);
    pool->alloc = alloc_cb;
}

#endif
