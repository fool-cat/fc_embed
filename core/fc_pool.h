/**
 * @file fc_pool.h
 * @author fool_cat (2696652257@qq.com)
 * @brief 提供一种单块固定大小O(1)分配复杂度,不连续-固定大小-可链式的内存池分配管理器
 * @version 1.0
 * @date 2025-09-24
 *
 * @copyright Copyright (c) 2025
 *
 */

// > 单次包含宏定义
#ifndef _FC_POOL_H_
#define _FC_POOL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "fc_compiler.h"
#include "fc_config.h"

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    // 默认不启用动态内存支持
    #define FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC 0
#endif

#ifndef fc_always_inline
    #define fc_always_inline static inline
#endif

    typedef enum
    {
        FC_POOL_DYNAMIC_MALLOC,   // 分配新内存
        FC_POOL_DYNAMIC_FREE,     // 释放内存块
        FC_POOL_DYNAMIC_REALLOC,  // 基于原有内存重新分配内存块
    } fc_pool_dynamic_type_t;

    typedef void *fc_pool_dynamic_mem_t;
    typedef bool (*fc_pool_dynamic_cb_t)(fc_pool_dynamic_type_t type, fc_pool_dynamic_mem_t *mem, size_t size);

    typedef void (*fc_pool_walker_t)(bool end, void *ptr, size_t used, void *user);

    typedef struct _fc_pool_header_t fc_pool_header_t;
    struct _fc_pool_header_t
    {
        fc_pool_header_t *next;  // 指向下一个内存块
        union
        {
            size_t            record_now;   // list_free使用,统计空闲块数(剩余块)
            fc_pool_header_t *tail;         // fifo_used使用,指向最后一个节点
            fc_pool_header_t *linear_last;  // 未分配的链表内存块中使用,如果是连续节点的第一个节点就指向此连续地址的最后一个节点,否则指向自身所在节点
            struct                          // 每个内存节点使用
            {
                size_t link_flag : 1;    // 后面是否还存在链式内存,第0位,pool内部至少4字节对齐,所以最低两位必定不会被使用,手动设置为1可以明确标识,不会误设置
                size_t : 1;              // 未使用,保留位
                size_t : 6;              // 未使用,保留位
                size_t block_count : 8;  // 本块内存有多少个连续内存块
                size_t used_size : 16;   // 当前块已使用大小,目前限制单块最大64KB,16bit可优化计算,后续可以调整位域占比实现更大单块支持
            };  // 从这往后就是块内容起始位置
        };
    };

    typedef struct _fc_pool_t fc_pool_t;
    struct _fc_pool_t
    {
        fc_pool_header_t list_free;  // 空闲链表
        fc_pool_header_t fifo_used;  // 已使用链表,用于fifo操作,非必须

        size_t block_size;  // 每块内存大小(字节)
        size_t record_min;  // 最小记录(剩余块)

        void *mem_start;  // 记录原始内存地址,建议对齐sizeof(size_t)
        void *mem_end;
#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
        fc_pool_dynamic_cb_t alloc_cb;  // 内存分配回调函数,可为NULL
#endif

        bool sort_free_enable;  // 是否启用排序释放内存,默认不启用(O1)
    };

    // 对象初始化,返回负值表示失败,返回正值表示可用的内存块数
    int  fc_pool_init(fc_pool_t *pool, void *mem, size_t mem_size, size_t block_size);
    void fc_pool_sort_enable(fc_pool_t *pool, bool sort_free);  // 设置是否启用排序释放内存,默认不启用(O1),初始化后才能调用,运行过程中不可改变

    //+********************************* 辅助API **********************************/
    fc_always_inline size_t fc_pool_node_size(fc_pool_t *pool)  // 获取一个内存块节点的大小
    {
        return (pool->block_size + sizeof(fc_pool_header_t));
    }
    fc_always_inline void *fc_pool_skip_header(void *ptr)  // 获取ptr跳过头部后的地址
    {
        return ((void *)((uint8_t *)ptr + sizeof(fc_pool_header_t)));
    }
    fc_always_inline fc_pool_header_t *fc_pool_rewind_header(void *ptr)  // 从ptr指向的内存块开始,返回内存块头部地址
    {
        return ((fc_pool_header_t *)((uint8_t *)ptr - sizeof(fc_pool_header_t)));
    }
    fc_always_inline size_t fc_pool_per_size(fc_pool_t *pool)  // 每块内存用户可用大小(字节)
    {
        return pool->block_size;
    }

    //+********************************* 基础信息查询 **********************************/
    size_t fc_pool_record_min(fc_pool_t *pool);  // 内存池最小记录(剩余块)
    size_t fc_pool_record_now(fc_pool_t *pool);  // 内存池当前记录(剩余块)

    //+********************************* 内存块状态管理及链式操作 **********************************/
    size_t fc_pool_used_size(void *ptr);                                  // 获取当前(链式)内存块已使用大小,ptr当前内存块地址
    bool   fc_pool_mark_used(void *ptr, size_t used_size);                // 标记当前内存块已使用大小,ptr当前内存块地址
    void   fc_pool_end(void *ptr);                                        // 给指定链式内存块的最后一块做标记,创建的时候默认已经标记
    void   fc_pool_link(void *front, void *back);                         // 将front和back链接起来,非连续内存块
    bool   fc_pool_linear_check(void *ptr);                               // 检查ptr是否为连续内存块,如果是返回true,否则返回false
    void   fc_pool_walk(void *ptr, fc_pool_walker_t walker, void *user);  // 遍历链式非连续内存块

    void fc_pool_fifo_walk(fc_pool_t *pool, fc_pool_walker_t walker, void *user);  // 遍历整个fifo used队列链表,遍历后释放内存
    bool fc_pool_merge(fc_pool_t *pool, void *front, void *back);                  // 将front和back合并,将back内存拼接到front后面,返回是否成功合并
    // void fc_pool_half_sort(fc_pool_t *pool);                                       // 对空闲链表一半进行排序,仅在sort_free_enable在false时有意义 // TODO

    //+********************************* 类似标准内存分配 **********************************/
    void *fc_pool_alloc(fc_pool_t *pool, size_t *size);  // 区别于malloc,传入size的指针分配成功将设置为实际分配(用户可用)的大小,返回可用的内存地址,固定O(1)复杂度
    // sort_free_enable == false时,free复杂度为O1,true的情况下以下free复杂度为O(n)
    void fc_pool_free(fc_pool_t *pool, void *ptr);  // 如果是链式非连续内存块,会将整个链式内存块释放掉,O(n)复杂度,n为链式内存块的块数
    // realloc在任何情况下均为O(n)复杂度
    void *fc_pool_realloc(fc_pool_t *pool, void *ptr, size_t size);  // 等效realloc,最坏O(n)复杂度
    // sort_free_enable == false时,以下API分配超过1块时很大概率失败,不超过1块则O(1)复杂度
    void *fc_pool_malloc(fc_pool_t *pool, size_t size);                // 等效malloc,尝试分配size字节的连续内存,如果失败则返回NULL,最坏O(n)复杂度
    void *fc_pool_calloc(fc_pool_t *pool, size_t count, size_t size);  // 等效calloc,最坏O(n)复杂度

    //+********************************* fc_pool_header_t对象当fifo使用 **********************************/
    bool  fc_header_fifo_empty(fc_pool_header_t *header);                 // fifo是否为空
    void  fc_header_fifo_push(fc_pool_header_t *header, void *head_ptr);  // 将整个链式非连续内存块添加到fifo
    void *fc_header_fifo_pop(fc_pool_header_t *header);                   // 从fifo中弹出一块链式非连续内存块
    void *fc_header_fifo_peek(fc_pool_header_t *header);                  // 从fifo中查看一块链式非连续内存块,不弹出

#define fc_pool_fifo_empty(pool) \
    fc_header_fifo_empty(&((pool)->fifo_used))

#define fc_pool_fifo_push(pool, head_ptr) \
    fc_header_fifo_push(&((pool)->fifo_used), head_ptr)

#define fc_pool_fifo_pop(pool) \
    fc_header_fifo_pop(&((pool)->fifo_used))

#define fc_pool_fifo_peek(pool) \
    fc_header_fifo_peek(&((pool)->fifo_used))

    //+********************************* 其他 **********************************/
    // bool   fc_pool_check(fc_pool_t *pool);             // 检查内存池是否正常
    size_t fc_pool_max_linear_count(fc_pool_t *pool);  // 内存池最大连续内存块数
    size_t fc_pool_max_linear_size(fc_pool_t *pool);   // 内存池最大连续内存块大小(字节)

    //+********************************* 动态内存支持使用 **********************************/

    // 提供了一份默认的动态内存分配实现,需要开启FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC宏定义
    extern bool fc_pool_dynamic_default(fc_pool_dynamic_type_t type, fc_pool_dynamic_mem_t *mem, size_t size);

    // 使用自定义的alloc回调函数,需要开启FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC宏定义
    void fc_pool_catch_alloc_cb(fc_pool_t *pool, fc_pool_dynamic_cb_t alloc_cb);  // 绑定用户自定义的内存分配回调函数

#if FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC
    #define FC_POOL_CATCH_ALLOC_CB(pool, alloc_cb) fc_pool_catch_alloc_cb(pool, alloc_cb)
#else
    #define FC_POOL_CATCH_ALLOC_CB(pool, alloc_cb) ((void)0)
#endif

    //+********************************* 辅助宏 **********************************/
    // 最坏的情况如果定义不合理,将会浪费block_size-1字节的内存(完全没有使用),下面两个宏定义用于计算合理的内存需求
    // 一般建议block_size为32以上且为sizeof(size_t)的整数倍

/**
 * @brief 计算内存池所需内存大小的宏定义,前提是内存对齐sizeof(size_t)的情况
 * 例: size_t pool_mem(FC_CALC_POOL_MEM_SIZE(32, 10)/sizeof(size_t)); // 内存池可以支配10块32字节的内存(不包括维护消耗)
 */
#define FC_CALC_POOL_MEM_SIZE(block_size, block_count) ((block_size + sizeof(fc_pool_header_t)) * (block_count))

/**
 * @brief 计算至少可支配内存大小的宏定义,前提是内存对齐sizeof(size_t)的情况
 *  例: size_t pool_size(FC_CALC_POOL_USABLE_SIZE(32,320)); //  // 内存池至少可支配320字节的内存,每块32字节(不包括维护消耗)
 */
#define FC_CALC_POOL_USABLE_SIZE(block_size, useable_size) ((useable_size + block_size - 1) / block_size * (block_size + sizeof(fc_pool_header_t)))

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_POOL_H_
