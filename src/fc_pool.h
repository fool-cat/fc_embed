/**
 * @file fc_pool.h
 * @author fool_cat (2696652257@qq.com)
 * @brief 提供一种O(1)分配复杂度,不连续-固定大小-可链式的内存池分配管理器
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

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*fc_pool_walker_t)(bool end, void *ptr, size_t used, void *user);

    typedef struct _fc_pool_header_t fc_pool_header_t;
    struct _fc_pool_header_t
    {
        fc_pool_header_t *next;  // 指向下一个内存块
        union
        {
            size_t            record_now;  // list_free使用,统计空闲块数(剩余块)
            fc_pool_header_t *tail;        // fifo_used使用,指向最后一个节点
            struct                         // 每个内存节点使用
            {
                size_t size : 16;  // 当前块已使用大小
                size_t end : 8;    // 是否为链式内存块的最后一块,理论只需要1bit,8bit为了更高效率
                size_t magic : 8;  // 魔数,可做为校验使用
            } tag;                 // 从这往后就是块内容起始位置
        } pool;
    };

    typedef struct _fc_pool_t fc_pool_t;
    struct _fc_pool_t
    {
        // void *mem;  // 记录原始内存地址,建议对齐sizeof(size_t)

        fc_pool_header_t list_free;  // 空闲链表
        fc_pool_header_t fifo_used;  // 已使用链表,用于fifo操作

        size_t block_size;   // 每块内存大小(字节)
        size_t record_min;   // 最小记录(剩余块)
        size_t record_lost;  // 丢失记录(分配失败次数)
    };

    int fc_pool_init(fc_pool_t *pool, void *mem, size_t mem_size, size_t block_size);

    void *fc_pool_alloc(fc_pool_t *pool, size_t *size);  // 区别于malloc,传入size的指针,返回实际分配的大小,O(1)复杂度
    void  fc_pool_free(fc_pool_t *pool, void *ptr);      // 如果是链式非连续内存块,会将整个链式内存块释放掉,O(n)复杂度,n为链式内存块的块数

    size_t fc_pool_per_size(fc_pool_t *pool);     // 每块内存大小(字节)
    size_t fc_pool_record_min(fc_pool_t *pool);   // 内存池最小记录(剩余块)
    size_t fc_pool_record_now(fc_pool_t *pool);   // 内存池当前记录(剩余块)
    size_t fc_pool_record_lost(fc_pool_t *pool);  // 内存池丢失记录(分配失败次数)

    void fc_pool_mark_used(void *ptr, size_t used_size);                // 标记给定内存块的已使用大小
    void fc_pool_end(void *ptr);                                        // 给指定链式内存块的最后一块做标记,创建的时候默认已经标记
    void fc_pool_link(void *front, void *back);                         // 将front和back链接起来,非连续内存块
    void fc_pool_walk(void *ptr, fc_pool_walker_t walker, void *user);  // 遍历链式非连续内存块

    //+********************************* 模拟连续内存需要使用的API **********************************/
    void *fc_pool_malloc(fc_pool_t *pool, size_t size);  // 模拟malloc,直接返回链式非连续内存块的头部,返回的内存不连续,不能直接使用,需要配合fc_pool_write/fc_pool_read/fc_pool_walk来等效为连续内存
    // 使用这些API可以等效于使用一块连续内存
    size_t fc_pool_write(fc_pool_t *pool, void *ptr, size_t offset, size_t write_size);  // 写入数据到链式非连续内存块,offset为用户层面的偏移量,返回实际写入的字节数
    size_t fc_pool_read(fc_pool_t *pool, void *ptr, size_t offset, size_t read_size);    // 从链式非连续内存块读取数据,offset为用户层面的偏移量,返回实际读取的字节数
    size_t fc_pool_strip_size(fc_pool_t *pool, void *ptr);                               // 获取链式非连续内存块的总大小(字节),ptr为链式非连续内存块的头部
    // size_t fc_pool_strip_used(fc_pool_t *pool, void *ptr);                               // 获取链式非连续内存块的已使用大小(字节),ptr为链式非连续内存块的头部

    //+********************************* 进阶使用API **********************************/
    bool  fc_pool_fifo_empty(fc_pool_t *pool);                 // fifo used链表是否为空
    void  fc_pool_fifo_push(fc_pool_t *pool, void *head_ptr);  // 将整个链式非连续内存块添加到fifo
    void *fc_pool_fifo_pop(fc_pool_t *pool);                   // 从fifo中弹出一块链式非连续内存块

    void fc_pool_fifo_walk(fc_pool_t *pool, fc_pool_walker_t walker, void *user);  // 遍历整个fifo used队列链表,遍历后释放内存

    //+********************************* 提供一份默认的实现给log组件 **********************************/

    extern fc_pool_t fc_log_pool;  // log组件使用的内存池声明,在fc_log.c中定义

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_POOL_H_
