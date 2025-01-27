/**
 * @file ring_fifo.h
 * @author fool_dog (2696652257@qq.com)
 * @brief 环形队列参考linux内核kfifo和CherryRB(https://github.com/cherry-embedded/CherryRB)
 * 这个环形队列要求的是绝对的性能,可以保证在只有一个消费者和一个生产者的情况下不需要加锁机制
 * @version 1.0
 * @date 2025-01-07
 *
 * @copyright Copyright (c) 2025
 *
 */

// > 单次包含宏定义
#ifndef _RING_FIFO_H_
#define _RING_FIFO_H_

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

    // #define RING_ASSERT(x) ((void)0)
#ifndef RING_ASSERT
    #include <assert.h>
    #define RING_ASSERT(x) assert(x)
#endif  //\ RING_ASSERT

#ifndef RING_MEMCPY
    static inline void __ring_memcpy(void* dst, void* src, size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            ((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
        }
    }
    #define RING_MEMCPY(dst, src, size) __ring_memcpy((void*)(dst), (void*)(src), size)
#else
    #include <string.h>
    #define RING_MEMCPY(dst, src, size) memcpy((void*)dst, (void*)src, size)
#endif  //\ RING_MEMCPY

    typedef struct
    {
        size_t in;    // 写入位置
        size_t out;   // 读出位置
        size_t mask;  // 掩码,用于优化取模运算,等于size-1,例如256,掩码为255(0xFF)
        size_t size;  // 缓冲池大小,考虑到可能需要频繁获取剩余大小,用mask+1需要更多运算,所以直接存储
        void*  pool;  // 缓冲池,缓冲池大小必须为2的幂次方pow(2,n),例如256,512,1024...

        size_t linear_size_write;  // 连续写入大小,记录linear_write_setup时的大小
        size_t linear_size_read;   // 连续读取大小,记录linear_read_setup时的大小
        size_t busy_in;            // 写入忙标志位,用于异步再入时判断是否正在写入
        size_t busy_out;           // 读出忙标志位,用于异步再出时判断是否正在读出
    } ring_fifo_t;

    static inline int  ring_fifo_init(ring_fifo_t* rb, void* pool, size_t size);
    static inline void ring_fifo_reset(ring_fifo_t* rb);
    static inline void ring_fifo_reset_read(ring_fifo_t* rb);

    static inline size_t ring_fifo_get_size(ring_fifo_t* rb);
    static inline size_t ring_fifo_get_used(ring_fifo_t* rb);
    static inline size_t ring_fifo_get_free(ring_fifo_t* rb);

    static inline bool ring_fifo_check_full(ring_fifo_t* rb);
    static inline bool ring_fifo_check_empty(ring_fifo_t* rb);

    static inline bool ring_fifo_write_byte(ring_fifo_t* rb, uint8_t byte);
    static inline bool ring_fifo_overwrite_byte(ring_fifo_t* rb, uint8_t byte);
    static inline bool ring_fifo_peek_byte(ring_fifo_t* rb, uint8_t* byte);
    static inline bool ring_fifo_read_byte(ring_fifo_t* rb, uint8_t* byte);
    static inline bool ring_fifo_drop_byte(ring_fifo_t* rb);

    static inline size_t ring_fifo_write(ring_fifo_t* rb, void* data, size_t size);
    static inline size_t ring_fifo_overwrite(ring_fifo_t* rb, void* data, size_t size);
    static inline size_t ring_fifo_peek(ring_fifo_t* rb, void* data, size_t size);
    static inline size_t ring_fifo_read(ring_fifo_t* rb, void* data, size_t size);
    static inline size_t ring_fifo_drop(ring_fifo_t* rb, size_t size);

    static inline void*  ring_fifo_linear_write_setup(ring_fifo_t* rb, size_t* size);
    static inline void*  ring_fifo_linear_read_setup(ring_fifo_t* rb, size_t* size);
    static inline size_t ring_fifo_linear_write_done(ring_fifo_t* rb, size_t size);
    static inline size_t ring_fifo_linear_read_done(ring_fifo_t* rb, size_t size);

    typedef enum
    {
        RING_FIFO_BUSY_IN = 0x01,
        RING_FIFO_BUSY_OUT = 0x02,
    } ring_fifo_busy_t;

    static inline void ring_fifo_mark_busy(ring_fifo_t* rb, ring_fifo_busy_t in);
    static inline void ring_fifo_mark_idle(ring_fifo_t* rb, ring_fifo_busy_t in);
    static inline bool ring_fifo_is_busy(ring_fifo_t* rb, ring_fifo_busy_t in);

#define RING_CREATE_FIFO(name, pow_2_size)                     \
    static ring_fifo_t name = {0};                             \
    static uint8_t     name##_pool[pow_2_size];                \
    static int         ring_fifo_init_##name(void* pool)       \
    {                                                          \
        return ring_fifo_init(&name, name##_pool, pow_2_size); \
    }

    //+********************************* 函数实现 **********************************/

    /**
     * @brief 初始化环形队列
     * @param rb 环形队列指针
     * @param pool 缓冲池指针
     * @param size 缓冲池大小
     * @return int 0:成功, -1:失败
     */
    static inline int ring_fifo_init(ring_fifo_t* rb, void* pool, size_t size)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(pool != NULL);
        RING_ASSERT(size >= 2);
        RING_ASSERT((size & (size - 1)) == 0);

        if (rb == NULL || pool == NULL || size < 2 || (size & (size - 1)) != 0)
            return -1;

        rb->in = 0;
        rb->out = 0;
        rb->mask = size - 1;
        rb->size = size;
        rb->pool = pool;

        return 0;
    }

    /**
     * @brief 重置环形队列
     * @param rb 环形队列指针
     */
    static inline void ring_fifo_reset(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);
        rb->in = rb->out = 0;
        rb->busy_in = rb->busy_out = 0;
    }

    /**
     * @brief 重置读指针
     * @param rb 环形队列指针
     */
    static inline void ring_fifo_reset_read(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);
        rb->out = rb->in;
    }

    /**
     * @brief 获取缓冲区大小
     * @param rb 环形队列指针
     * @return size_t 缓冲区总大小
     */
    static inline size_t ring_fifo_get_size(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);
        return rb->size;
    }

    /**
     * @brief 获取已使用空间大小
     * @param rb 环形队列指针
     * @return size_t 已使用字节数
     */
    static inline size_t ring_fifo_get_used(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);
        return rb->in - rb->out;
    }

    /**
     * @brief 获取剩余可用空间大小
     * @param rb 环形队列指针
     * @return size_t 剩余可用字节数
     */
    static inline size_t ring_fifo_get_free(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);
        return rb->size - (rb->in - rb->out);
    }

    /**
     * @brief 检查缓冲区是否已满
     * @param rb 环形队列指针
     * @return bool true:已满, false:未满
     */
    static inline bool ring_fifo_check_full(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);
        return rb->in - rb->out == rb->size;
    }

    /**
     * @brief 检查缓冲区是否为空
     * @param rb 环形队列指针
     * @return bool true:为空, false:非空
     */
    static inline bool ring_fifo_check_empty(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);
        return rb->in == rb->out;
    }

    /**
     * @brief 写入一个字节
     * @param rb 环形队列指针
     * @param byte 要写入的字节
     * @return bool true:成功, false:失败
     */
    static inline bool ring_fifo_write_byte(ring_fifo_t* rb, uint8_t byte)
    {
        RING_ASSERT(rb != NULL);

        if (ring_fifo_check_full(rb))
            return false;

        ((uint8_t*)rb->pool)[rb->in & rb->mask] = byte;
        rb->in++;
        return true;
    }

    /**
     * @brief 强制写入一个字节
     * @param rb 环形队列指针
     * @param byte 要写入的字节
     * @return bool true:成功, false:失败
     */
    static inline bool ring_fifo_overwrite_byte(ring_fifo_t* rb, uint8_t byte)
    {
        RING_ASSERT(rb != NULL);

        if (ring_fifo_check_full(rb))
            rb->out++;

        ((uint8_t*)rb->pool)[rb->in & rb->mask] = byte;
        rb->in++;
        return true;
    }

    /**
     * @brief 查看下一个字节但不取出
     * @param rb 环形队列指针
     * @param byte 用于存储查看到的字节
     * @return bool true:成功, false:失败
     */
    static inline bool ring_fifo_peek_byte(ring_fifo_t* rb, uint8_t* byte)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(byte != NULL);

        if (ring_fifo_check_empty(rb))
            return false;

        *byte = ((uint8_t*)rb->pool)[rb->out & rb->mask];
        return true;
    }

    /**
     * @brief 读取一个字节
     * @param rb 环形队列指针
     * @param byte 用于存储读取到的字节
     * @return bool true:成功, false:失败
     */
    static inline bool ring_fifo_read_byte(ring_fifo_t* rb, uint8_t* byte)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(byte != NULL);

        if (ring_fifo_check_empty(rb))
            return false;

        *byte = ((uint8_t*)rb->pool)[rb->out & rb->mask];
        rb->out++;
        return true;
    }

    /**
     * @brief 丢弃一个字节
     * @param rb 环形队列指针
     * @return bool true:成功, false:失败
     */
    static inline bool ring_fifo_drop_byte(ring_fifo_t* rb)
    {
        RING_ASSERT(rb != NULL);

        if (ring_fifo_check_empty(rb))
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
    static inline size_t ring_fifo_write(ring_fifo_t* rb, void* data, size_t size)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(data != NULL);

        size_t free = ring_fifo_get_free(rb);
        size_t write_size = (size < free) ? size : free;

        RING_MEMCPY((uint8_t*)rb->pool + (rb->in & rb->mask), data, write_size);
        rb->in += write_size;

        return write_size;
    }

    /**
     * @brief 批量强制写入数据
     * @param rb 环形队列指针
     * @param data 要写入的数据
     * @param size 要写入的字节数
     * @return size_t 实际写入字节数
     */
    static inline size_t ring_fifo_overwrite(ring_fifo_t* rb, void* data, size_t size)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(data != NULL);

        if (size > rb->size)
        {
            data = (uint8_t*)data + (size - rb->size);
            size = rb->size;
            rb->out = rb->in + size;
        }
        else if (size > ring_fifo_get_free(rb))
        {
            rb->out = rb->in + size;
        }

        RING_MEMCPY((uint8_t*)rb->pool + (rb->in & rb->mask), data, size);
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
    static inline size_t ring_fifo_peek(ring_fifo_t* rb, void* data, size_t size)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(data != NULL);

        size_t used = ring_fifo_get_used(rb);
        size_t read_size = (size < used) ? size : used;

        RING_MEMCPY(data, (uint8_t*)rb->pool + (rb->out & rb->mask), read_size);
        return read_size;
    }

    /**
     * @brief 批量读取数据
     * @param rb 环形队列指针
     * @param data 用于存储读取到的数据
     * @param size 要读取的字节数
     * @return size_t 实际读取字节数
     */
    static inline size_t ring_fifo_read(ring_fifo_t* rb, void* data, size_t size)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(data != NULL);

        size_t used = ring_fifo_get_used(rb);
        size_t read_size = (size < used) ? size : used;

        RING_MEMCPY(data, (uint8_t*)rb->pool + (rb->out & rb->mask), read_size);
        rb->out += read_size;

        return read_size;
    }

    /**
     * @brief 批量丢弃数据
     * @param rb 环形队列指针
     * @param size 要丢弃的字节数
     * @return size_t 实际丢弃字节数
     */
    static inline size_t ring_fifo_drop(ring_fifo_t* rb, size_t size)
    {
        RING_ASSERT(rb != NULL);

        size_t used = ring_fifo_get_used(rb);
        size_t drop_size = (size < used) ? size : used;

        rb->out += drop_size;

        return drop_size;
    }

    /**
     * @brief 获取可连续写入的内存区域
     * @param rb 环形队列指针
     * @param size 用于存储可写入字节数
     * @return void* 可写入区域起始地址, 失败返回NULL
     */
    static inline void* ring_fifo_linear_write_setup(ring_fifo_t* rb, size_t* size)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(size != NULL);

        size_t free = ring_fifo_get_free(rb);
        size_t linear_size = rb->size - (rb->in & rb->mask);
        rb->linear_size_write = (free < linear_size) ? free : linear_size;
        *size = rb->linear_size_write;

        return (uint8_t*)rb->pool + (rb->in & rb->mask);
    }

    /**
     * @brief 获取可连续读取的内存区域
     * @param rb 环形队列指针
     * @param size 用于存储可读取字节数
     * @return void* 可读取区域起始地址, 失败返回NULL
     */
    static inline void* ring_fifo_linear_read_setup(ring_fifo_t* rb, size_t* size)
    {
        RING_ASSERT(rb != NULL);
        RING_ASSERT(size != NULL);

        size_t used = ring_fifo_get_used(rb);
        size_t linear_size = rb->size - (rb->out & rb->mask);
        rb->linear_size_read = (used < linear_size) ? used : linear_size;
        *size = rb->linear_size_read;

        return (uint8_t*)rb->pool + (rb->out & rb->mask);
    }

    /**
     * @brief 完成连续写入操作
     * @param rb 环形队列指针
     * @param size 实际写入字节数
     * @return size_t 实际写入字节数
     */
    static inline size_t ring_fifo_linear_write_done(ring_fifo_t* rb, size_t size)
    {
        RING_ASSERT(rb != NULL);

        rb->in += size;
        rb->linear_size_write = 0;
        return size;
    }

    /**
     * @brief 完成连续读取操作
     * @param rb 环形队列指针
     * @param size 实际读取字节数
     * @return size_t 实际读取字节数
     */
    static inline size_t ring_fifo_linear_read_done(ring_fifo_t* rb, size_t size)
    {
        RING_ASSERT(rb != NULL);

        rb->out += size;
        rb->linear_size_read = 0;
        return size;
    }

    /**
     * @brief 标记环形队列为忙状态
     * @param rb 环形队列指针
     * @param in 忙标志位
     */
    static inline void ring_fifo_mark_busy(ring_fifo_t* rb, ring_fifo_busy_t in)
    {
        RING_ASSERT(rb != NULL);

        switch (in)
        {
        case RING_FIFO_BUSY_IN:
            rb->busy_in = 1;
            break;
        case RING_FIFO_BUSY_OUT:
            rb->busy_out = 1;
            break;
        default:
            RING_ASSERT(0);
            break;
        }
    }

    /**
     * @brief 标记环形队列为闲状态
     * @param rb 环形队列指针
     * @param in 忙标志位
     */
    static inline void ring_fifo_mark_idle(ring_fifo_t* rb, ring_fifo_busy_t in)
    {
        RING_ASSERT(rb != NULL);

        switch (in)
        {
        case RING_FIFO_BUSY_IN:
            rb->busy_in = 0;
            break;
        case RING_FIFO_BUSY_OUT:
            rb->busy_out = 0;
            break;
        default:
            RING_ASSERT(0);
            break;
        }
    }

    /**
     * @brief 检查环形队列是否为忙状态
     * @param rb 环形队列指针
     * @param in 忙标志位
     * @return bool true:忙, false:闲
     */
    static inline bool ring_fifo_is_busy(ring_fifo_t* rb, ring_fifo_busy_t in)
    {
        RING_ASSERT(rb != NULL);

        bool busy = false;

        switch (in)
        {
        case RING_FIFO_BUSY_IN:
            busy = rb->busy_in ? true : false;
            break;
        case RING_FIFO_BUSY_OUT:
            busy = rb->busy_out ? true : false;
            break;
        default:
            RING_ASSERT(0);
            break;
        }

        return busy;
    }

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _RING_FIFO_H_
