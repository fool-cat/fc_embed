/**
 * @file fc_fifo.h
 * @author fool_cat (2696652257@qq.com)
 * @brief 字节流-环形队列参考linux内核kfifo和CherryRB(https://github.com/cherry-embedded/CherryRB)
 * 这个环形队列要求的是绝对的性能,可以保证在只有一个消费者和一个生产者的情况下不需要加锁机制
 * @version 1.0
 * @date 2025-01-07
 *
 * @copyright Copyright (c) 2025
 *
 */

//> 单次包含宏定义
#ifndef __FC_FIFO_H__
#define __FC_FIFO_H__

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "fc_compiler.h"
#include "fc_config.h"

#ifndef fc_always_inline
    #define fc_always_inline static inline
#endif

// 断言只在PC验证的时候用,验证完成后应关闭断言,这个fifo实现追求性能为主
#ifndef fc_fifo_assert
    #define fc_fifo_assert(x) ((void)0)
#endif  //\ fc_fifo_assert

#ifndef FC_USE_STD_MEMCPY
    #define FC_USE_STD_MEMCPY 1 /**< 是否使用标准库的memcpy */
#endif

#if FC_USE_STD_MEMCPY

    // 高优化等级的时候memcpy可能会出现内存对齐问题,所以这里提供一份单字节拷贝的版本可选
    #include <string.h>
    #define fc_fifo_memcpy(dst, src, size) memcpy(dst, src, size)

#else

    #ifndef fc_fifo_memcpy
        #define fc_fifo_memcpy(dst, src, size)                   \
            {                                                    \
                for (size_t _i = 0; _i < size; _i++)             \
                {                                                \
                    ((uint8_t *)dst)[_i] = ((uint8_t *)src)[_i]; \
                }                                                \
            }
    #endif  //\ fc_fifo_memcpy

#endif  //\ FC_USE_STD_MEMCPY

    typedef struct _fc_fifo_t fc_fifo_t;
    struct _fc_fifo_t
    {
        size_t in;    // 写入位置
        size_t out;   // 读出位置
        size_t mask;  // 掩码,用于优化取模运算,等于size-1,例如256,掩码为255(0xFF)
        void  *pool;  // 缓冲池,缓冲池大小必须为2的幂次方(2^n),例如256,512,1024...

        size_t linear_size_write;  // 连续写入大小,记录linear_write_setup时设置的大小,0表示空闲
        size_t linear_size_read;   // 连续读取大小,记录linear_read_setup时设置的大小,0表示空闲
    };

    fc_always_inline int  fc_fifo_init(fc_fifo_t *rb, void *pool, size_t size);
    fc_always_inline void fc_fifo_reset(fc_fifo_t *rb);
    fc_always_inline void fc_fifo_reset_read(fc_fifo_t *rb);
    fc_always_inline void fc_fifo_reset_write(fc_fifo_t *rb);

    fc_always_inline size_t fc_fifo_get_size(fc_fifo_t *rb);
    fc_always_inline size_t fc_fifo_get_used(fc_fifo_t *rb);
    fc_always_inline size_t fc_fifo_get_free(fc_fifo_t *rb);

    fc_always_inline bool fc_fifo_check_full(fc_fifo_t *rb);
    fc_always_inline bool fc_fifo_check_empty(fc_fifo_t *rb);

    fc_always_inline bool fc_fifo_write_byte(fc_fifo_t *rb, uint8_t byte);
    fc_always_inline bool fc_fifo_overwrite_byte(fc_fifo_t *rb, uint8_t byte);
    fc_always_inline bool fc_fifo_peek_byte(fc_fifo_t *rb, uint8_t *byte);
    fc_always_inline bool fc_fifo_read_byte(fc_fifo_t *rb, uint8_t *byte);
    fc_always_inline bool fc_fifo_drop_byte(fc_fifo_t *rb);

    fc_always_inline size_t fc_fifo_write(fc_fifo_t *rb, void *data, size_t size);
    fc_always_inline size_t fc_fifo_overwrite(fc_fifo_t *rb, void *data, size_t size);
    fc_always_inline size_t fc_fifo_peek(fc_fifo_t *rb, void *data, size_t size);
    fc_always_inline size_t fc_fifo_read(fc_fifo_t *rb, void *data, size_t size);
    fc_always_inline size_t fc_fifo_drop(fc_fifo_t *rb, size_t size);

    fc_always_inline void  *fc_fifo_linear_write_setup(fc_fifo_t *rb, size_t *size);
    fc_always_inline void  *fc_fifo_linear_read_setup(fc_fifo_t *rb, size_t *size);
    fc_always_inline size_t fc_fifo_linear_write_done(fc_fifo_t *rb, size_t size);
    fc_always_inline size_t fc_fifo_linear_read_done(fc_fifo_t *rb, size_t size);

    // 单次线性读写限制,限制大小为缓冲区空间的1/(2^n)
    fc_always_inline void *fc_fifo_linear_write_setup_limit(fc_fifo_t *rb, size_t *size, size_t shift_n);
    fc_always_inline void *fc_fifo_linear_read_setup_limit(fc_fifo_t *rb, size_t *size, size_t shift_n);

    fc_always_inline size_t fc_fifo_linear_write_get_size(fc_fifo_t *rb);
    fc_always_inline size_t fc_fifo_linear_read_get_size(fc_fifo_t *rb);
    fc_always_inline bool   fc_fifo_linear_write_busy(fc_fifo_t *rb);
    fc_always_inline bool   fc_fifo_linear_read_busy(fc_fifo_t *rb);

    fc_always_inline bool   fc_fifo_seek_byte(fc_fifo_t *rb, uint8_t *byte, size_t offset);
    fc_always_inline size_t fc_fifo_seek(fc_fifo_t *rb, void *data, size_t size, size_t offset);

    fc_always_inline bool   fc_fifo_drop_tail_byte(fc_fifo_t *rb);
    fc_always_inline size_t fc_fifo_drop_tail(fc_fifo_t *rb, size_t size);

    /*
    // 同时操作写入和读取索引
    // 1. fc_fifo_init
    // 2. fc_fifo_reset
    // 3. fc_fifo_overwrite_byte,当装满时才操作out
    // 4. fc_fifo_overwrite,当装满时才操作out

    // 仅操作写入索引
    // 1. fc_fifo_reset_write
    // 2. fc_fifo_write_byte
    // 3. fc_fifo_write
    // 4. fc_fifo_linear_write_done,配套使用fc_fifo_linear_write_setup,在setup到done期间不要调用其他写操作
    // 5. fc_fifo_drop_tail_byte
    // 6. fc_fifo_drop_tail

    // 仅操作读取索引
    // 1. fc_fifo_reset_read
    // 2. fc_fifo_read_byte
    // 3. fc_fifo_read
    // 4. fc_fifo_linear_read_done,配套使用fc_fifo_linear_read_setup,在setup到done期间不要调用其他读操作
    // 5. fc_fifo_drop_byte
    // 6. fc_fifo_drop
    */

/**
 * @brief 类函数宏,对fc_fifo_t类型指针分配静态内存,一个指针只使用一次,不然会有大量的静态内存浪费
 * @param ptr 环形队列指针,需要一个指向fc_fifo_t类型的空指针
 * @param log2_size 缓冲池大小,实际大小为2^log2_size(对齐size_t),需要大于0
 * @note 该宏用于给环形队列指针分配静态内存,只能在函数中使用
 * 内部缓冲区使用size_t数组,是为了保证内存对齐(虽然理论上不需要)
 */
#define fc_fifo_static_new_at(ptr, log2_size)                                                      \
    do                                                                                             \
    {                                                                                              \
        fc_fifo_assert(ptr == NULL);                                                               \
        fc_fifo_assert(log2_size > 0);                                                             \
        static size_t    __pool[((1 << log2_size) + (sizeof(size_t) - 1)) / sizeof(size_t)] = {0}; \
        static fc_fifo_t __fifo = {0};                                                             \
        ptr = &__fifo;                                                                             \
        if (0 != fc_fifo_init(ptr, (void *)__pool, (1 << log2_size)))                              \
        {                                                                                          \
            ptr = NULL;                                                                            \
        }                                                                                          \
        fc_fifo_assert(ptr != NULL);                                                               \
    } while (0)

    //+********************************* 函数实现 **********************************/

    /**
     * @brief 初始化环形队列
     * @param rb 环形队列指针
     * @param pool 缓冲池指针
     * @param size 缓冲池大小
     * @return int 0:成功, -1:失败
     */
    fc_always_inline int fc_fifo_init(fc_fifo_t *rb, void *pool, size_t size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(pool != NULL);
        fc_fifo_assert(size >= 2);
        fc_fifo_assert((size & (size - 1)) == 0);

        if (rb == NULL || pool == NULL || size < 2 || (size & (size - 1)) != 0)
            return -1;

        rb->in = 0;
        rb->out = 0;
        rb->mask = size - 1;
        rb->pool = pool;
        rb->linear_size_read = rb->linear_size_write = 0;

        return 0;
    }

    /**
     * @brief 重置环形队列
     * @param rb 环形队列指针
     */
    fc_always_inline void fc_fifo_reset(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        rb->in = rb->out = 0;
        rb->linear_size_read = rb->linear_size_write = 0;
    }

    /**
     * @brief 重置读指针
     * @param rb 环形队列指针
     */
    fc_always_inline void fc_fifo_reset_read(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        rb->out = rb->in;
        rb->linear_size_read = 0;
    }

    /**
     * @brief 重置写指针
     * @param rb 环形队列指针
     */
    fc_always_inline void fc_fifo_reset_write(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        rb->in = rb->out;
        rb->linear_size_write = 0;
    }

    /**
     * @brief 获取缓冲区大小
     * @param rb 环形队列指针
     * @return size_t 缓冲区总大小
     */
    fc_always_inline size_t fc_fifo_get_size(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return (rb->mask + 1);
    }

    /**
     * @brief 获取已使用空间大小
     * @param rb 环形队列指针
     * @return size_t 已使用字节数
     */
    fc_always_inline size_t fc_fifo_get_used(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return (rb->in - rb->out);
    }

    /**
     * @brief 获取剩余可用空间大小
     * @param rb 环形队列指针
     * @return size_t 剩余可用字节数
     */
    fc_always_inline size_t fc_fifo_get_free(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return ((rb->mask + 1) - (rb->in - rb->out));
    }

    /**
     * @brief 检查缓冲区是否已满
     * @param rb 环形队列指针
     * @return bool true:已满, false:未满
     */
    fc_always_inline bool fc_fifo_check_full(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return ((rb->in - rb->out) == (rb->mask + 1));
    }

    /**
     * @brief 检查缓冲区是否为空
     * @param rb 环形队列指针
     * @return bool true:为空, false:非空
     */
    fc_always_inline bool fc_fifo_check_empty(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return (rb->in == rb->out);
    }

    /**
     * @brief 写入一个字节
     * @param rb 环形队列指针
     * @param byte 要写入的字节
     * @return bool true:成功, false:失败
     */
    fc_always_inline bool fc_fifo_write_byte(fc_fifo_t *rb, uint8_t byte)
    {
        fc_fifo_assert(rb != NULL);

        if (fc_fifo_check_full(rb))
            return false;

        ((uint8_t *)rb->pool)[rb->in & rb->mask] = byte;
        rb->in++;
        return true;
    }

    /**
     * @brief 强制写入一个字节
     * @param rb 环形队列指针
     * @param byte 要写入的字节
     * @return bool true:成功, false:失败
     */
    fc_always_inline bool fc_fifo_overwrite_byte(fc_fifo_t *rb, uint8_t byte)
    {
        fc_fifo_assert(rb != NULL);

        if (fc_fifo_check_full(rb))
            rb->out++;

        ((uint8_t *)rb->pool)[rb->in & rb->mask] = byte;
        rb->in++;
        return true;
    }

    /**
     * @brief 查看下一个字节但不取出
     * @param rb 环形队列指针
     * @param byte 用于存储查看到的字节
     * @return bool true:成功, false:失败
     */
    fc_always_inline bool fc_fifo_peek_byte(fc_fifo_t *rb, uint8_t *byte)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(byte != NULL);

        if (fc_fifo_check_empty(rb))
            return false;

        *byte = ((uint8_t *)rb->pool)[rb->out & rb->mask];
        return true;
    }

    /**
     * @brief 读取一个字节
     * @param rb 环形队列指针
     * @param byte 用于存储读取到的字节
     * @return bool true:成功, false:失败
     */
    fc_always_inline bool fc_fifo_read_byte(fc_fifo_t *rb, uint8_t *byte)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(byte != NULL);

        bool ret;
        ret = fc_fifo_peek_byte(rb, byte);
        rb->out += ret;
        return ret;
    }

    /**
     * @brief 丢弃一个字节
     * @param rb 环形队列指针
     * @return bool true:成功, false:失败
     */
    fc_always_inline bool fc_fifo_drop_byte(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);

        if (fc_fifo_check_empty(rb))
            return false;

        rb->out++;
        return true;
    }

    /**
     * @brief 批量写入数据
     * @param rb 环形队列指针
     * @param data 要写入的数据
     * @param size 要写入的字节数
     * @return size_t 实际写入字节数
     */
    fc_always_inline size_t fc_fifo_write(fc_fifo_t *rb, void *data, size_t size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(data != NULL);

        size_t unused;
        size_t offset;
        size_t remain;

        unused = (rb->mask + 1) - (rb->in - rb->out);

        if (size > unused)
        {
            size = unused;
        }

        offset = rb->in & rb->mask;

        remain = (rb->mask + 1) - offset;
        remain = remain > size ? size : remain;

        fc_fifo_memcpy(((uint8_t *)(rb->pool)) + offset, data, remain);
        fc_fifo_memcpy(rb->pool, (uint8_t *)data + remain, size - remain);

        rb->in += size;

        return size;
    }

    /**
     * @brief 批量强制写入数据
     * @param rb 环形队列指针
     * @param data 要写入的数据
     * @param size 要写入的字节数
     * @return size_t 实际写入字节数
     */
    fc_always_inline size_t fc_fifo_overwrite(fc_fifo_t *rb, void *data, size_t size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(data != NULL);

        size_t unused;
        size_t offset;
        size_t remain;

        unused = (rb->mask + 1) - (rb->in - rb->out);

        if (size > unused)
        {
            if (size > (rb->mask + 1))
            {
                size = (rb->mask + 1);
            }

            rb->out += size - unused;
        }

        offset = rb->in & rb->mask;

        remain = (rb->mask + 1) - offset;
        remain = remain > size ? size : remain;

        fc_fifo_memcpy(((uint8_t *)(rb->pool)) + offset, data, remain);
        fc_fifo_memcpy(rb->pool, (uint8_t *)data + remain, size - remain);

        rb->in += size;

        return size;
    }

    /**
     * @brief 批量查看数据但不取出
     * @param rb 环形队列指针
     * @param data 用于存储查看到的数据
     * @param size 要查看的字节数
     * @return size_t 实际查看字节数
     */
    fc_always_inline size_t fc_fifo_peek(fc_fifo_t *rb, void *data, size_t size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(data != NULL);

        size_t used;
        size_t offset;
        size_t remain;

        used = rb->in - rb->out;
        if (size > used)
        {
            size = used;
        }

        offset = rb->out & rb->mask;

        remain = (rb->mask + 1) - offset;
        remain = remain > size ? size : remain;

        fc_fifo_memcpy(data, ((uint8_t *)(rb->pool)) + offset, remain);
        fc_fifo_memcpy((uint8_t *)data + remain, rb->pool, size - remain);

        return size;
    }

    /**
     * @brief 批量读取数据
     * @param rb 环形队列指针
     * @param data 用于存储读取到的数据
     * @param size 要读取的字节数
     * @return size_t 实际读取字节数
     */
    fc_always_inline size_t fc_fifo_read(fc_fifo_t *rb, void *data, size_t size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(data != NULL);

        size = fc_fifo_peek(rb, data, size);
        rb->out += size;

        return size;
    }

    /**
     * @brief 批量丢弃数据
     * @param rb 环形队列指针
     * @param size 要丢弃的字节数
     * @return size_t 实际丢弃字节数
     */
    fc_always_inline size_t fc_fifo_drop(fc_fifo_t *rb, size_t size)
    {
        fc_fifo_assert(rb != NULL);

        size_t used;
        used = rb->in - rb->out;
        if (size > used)
        {
            size = used;
        }

        rb->out += size;
        return size;
    }

    /**
     * @brief 获取可连续写入的内存区域
     * @param rb 环形队列指针
     * @param size 用于存储可写入字节数
     * @return void* 可写入区域起始地址
     */
    fc_always_inline void *fc_fifo_linear_write_setup(fc_fifo_t *rb, size_t *size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(size != NULL);

        size_t free = fc_fifo_get_free(rb);
        size_t linear_size = (rb->mask + 1) - (rb->in & rb->mask);
        rb->linear_size_write = (free < linear_size) ? free : linear_size;
        *size = rb->linear_size_write;

        return ((void *)((uint8_t *)rb->pool + (rb->in & rb->mask)));
    }

    /**
     * @brief 获取可连续读取的内存区域
     * @param rb 环形队列指针
     * @param size 用于存储可读取字节数
     * @return void* 可读取区域起始地址
     */
    fc_always_inline void *fc_fifo_linear_read_setup(fc_fifo_t *rb, size_t *size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(size != NULL);

        size_t used = fc_fifo_get_used(rb);
        size_t linear_size = (rb->mask + 1) - (rb->out & rb->mask);
        rb->linear_size_read = (used < linear_size) ? used : linear_size;
        *size = rb->linear_size_read;

        return ((void *)((uint8_t *)rb->pool + (rb->out & rb->mask)));
    }

    /**
     * @brief 完成连续写入操作
     * @param rb 环形队列指针
     * @param size 实际写入字节数
     * @return size_t 实际写入字节数
     */
    fc_always_inline size_t fc_fifo_linear_write_done(fc_fifo_t *rb, size_t size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(size <= rb->linear_size_write);

        // size = (size <= rb->linear_size_write) ? size : rb->linear_size_write;

        rb->in += size;
        rb->linear_size_write = 0;  // 此次连续写入完成,重置为0清除忙状态
        return size;
    }

    /**
     * @brief 完成连续读取操作
     * @param rb 环形队列指针
     * @param size 实际读取字节数
     * @return size_t 实际读取字节数
     */
    fc_always_inline size_t fc_fifo_linear_read_done(fc_fifo_t *rb, size_t size)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(size <= rb->linear_size_read);

        // size = (size <= rb->linear_size_read) ? size : rb->linear_size_read;

        rb->out += size;
        rb->linear_size_read = 0;  // 此次连续读取完成,重置为0清除忙状态
        return size;
    }

    /**
     * @brief 获取可连续写入的内存区域,限制最大为缓冲区的1/(2^n)
     *
     * @param rb
     * @param size
     * @param shift_n
     * @return void*
     */
    fc_always_inline void *fc_fifo_linear_write_setup_limit(fc_fifo_t *rb, size_t *size, size_t shift_n)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(size != NULL);
        fc_fifo_assert(((rb->mask + 1) >> shift_n) > 0);

        size_t free = fc_fifo_get_free(rb);
        size_t linear_size = (rb->mask + 1) - (rb->in & rb->mask);
        rb->linear_size_write = (free < linear_size) ? free : linear_size;
        rb->linear_size_write = (rb->linear_size_write < ((rb->mask + 1) >> shift_n)) ? rb->linear_size_write : ((rb->mask + 1) >> shift_n);
        *size = rb->linear_size_write;

        return ((void *)((uint8_t *)rb->pool + (rb->in & rb->mask)));
    }

    /**
     * @brief 获取可连续读取的内存区域,限制最大为缓冲区的1/(2^n)
     *
     * @param rb
     * @param size
     * @param shift_n
     * @return void*
     */
    fc_always_inline void *fc_fifo_linear_read_setup_limit(fc_fifo_t *rb, size_t *size, size_t shift_n)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(size != NULL);
        fc_fifo_assert(((rb->mask + 1) >> shift_n) > 0);

        size_t used = fc_fifo_get_used(rb);
        size_t linear_size = (rb->mask + 1) - (rb->out & rb->mask);
        rb->linear_size_read = (used < linear_size) ? used : linear_size;
        rb->linear_size_read = (rb->linear_size_read < ((rb->mask + 1) >> shift_n)) ? rb->linear_size_read : ((rb->mask + 1) >> shift_n);
        *size = rb->linear_size_read;

        return ((void *)((uint8_t *)rb->pool + (rb->out & rb->mask)));
    }

    /**
     * @brief 获取连续写入的大小
     * @param rb 环形队列指针
     * @return size_t 获取上次linear_write_setup设置的大小
     */
    fc_always_inline size_t fc_fifo_linear_write_get_size(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return rb->linear_size_write;
    }

    /**
     * @brief 获取连续读取的大小
     * @param rb 环形队列指针
     * @return size_t 获取上次linear_read_setup设置的大小
     */
    fc_always_inline size_t fc_fifo_linear_read_get_size(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return rb->linear_size_read;
    }

    /**
     * @brief 检查是否有连续写入操作正在进行
     * @param rb 环形队列指针
     * @return bool true:忙, false:空闲
     */
    fc_always_inline bool fc_fifo_linear_write_busy(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return (rb->linear_size_write ? true : false);
    }

    /**
     * @brief 检查是否有连续读取操作正在进行
     * @param rb 环形队列指针
     * @return bool true:忙, false:空闲
     */
    fc_always_inline bool fc_fifo_linear_read_busy(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);
        return (rb->linear_size_read ? true : false);
    }

    /**
     * @brief 查看指定位置的字节但不取出
     * 可以等效看作从数组中读取下标为[offset-1]的元素
     * @param rb
     * @param byte
     * @param offset
     * @return fc_always_inline
     */
    fc_always_inline bool fc_fifo_seek_byte(fc_fifo_t *rb, uint8_t *byte, size_t offset)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(byte != NULL);

        size_t used = rb->in - rb->out;
        if (offset >= used)
            return false;

        *byte = ((uint8_t *)rb->pool)[(rb->out + offset) & rb->mask];
        return true;
    }

    /**
     * @brief 批量查看指定偏移位置的数据但不取出
     *
     * @param rb
     * @param data
     * @param size
     * @param offset
     * @return fc_always_inline
     */
    fc_always_inline size_t fc_fifo_seek(fc_fifo_t *rb, void *data, size_t size, size_t offset)
    {
        fc_fifo_assert(rb != NULL);
        fc_fifo_assert(data != NULL);

        size_t used;
        size_t actual_size;
        size_t data_offset;
        size_t remain;

        used = rb->in - rb->out;
        if (offset >= used)
        {
            return 0;
        }

        actual_size = used - offset;
        if (size > actual_size)
        {
            size = actual_size;
        }

        data_offset = (rb->out + offset) & rb->mask;

        remain = (rb->mask + 1) - data_offset;
        remain = remain > size ? size : remain;

        fc_fifo_memcpy(data, ((uint8_t *)(rb->pool)) + data_offset, remain);
        fc_fifo_memcpy((uint8_t *)data + remain, rb->pool, size - remain);

        return size;
    }

    /**
     * @brief 丢弃末尾一个字节
     *
     * @param rb
     * @return fc_always_inline
     */
    fc_always_inline bool fc_fifo_drop_tail_byte(fc_fifo_t *rb)
    {
        fc_fifo_assert(rb != NULL);

        if (fc_fifo_check_empty(rb))
            return false;

        rb->in--;
        return true;
    }

    /**
     * @brief 批量丢弃末尾数据
     *
     * @param rb
     * @param size
     * @return fc_always_inline
     */
    fc_always_inline size_t fc_fifo_drop_tail(fc_fifo_t *rb, size_t size)
    {
        fc_fifo_assert(rb != NULL);

        size_t used;
        used = rb->in - rb->out;
        if (size > used)
        {
            size = used;
        }

        rb->in -= size;
        return size;
    }

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  // __FC_FIFO_H__
