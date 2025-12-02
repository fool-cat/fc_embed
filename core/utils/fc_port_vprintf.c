/**
 * @file fc_port_vprintf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief 注意需要保证线程安全
 * @version 1.0
 * @date 2025-02-19
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>

#include "../fc_stdio.h"

#include "../fc_port.h"  //使用相对路径避免依赖

#ifndef FC_FIFO_VPRINTF_LINEAR_WRITE
    #define FC_FIFO_VPRINTF_LINEAR_WRITE 1
#endif

#if FC_FIFO_VPRINTF_LINEAR_WRITE

/**
 * @brief 直接使用fc_fifo_t提供的接口进行写入
 *
 * @param buf
 * @param len
 * @return int
 */
static int __fc_fifo_vprintf_write(FC_FILE *f, const void *buf, int len)
{
    (void)buf;

    if (len >= (int)FC_IO_SWAP)  // 只可能==FC_IO_SWAP
    {
        fc_fifo_linear_write_done((fc_fifo_t *)(f->user), (size_t)f->p_now - (size_t)f->p_start);

        // 还有空间
        if (fc_fifo_get_free((fc_fifo_t *)(f->user)) >= 1)
        {
            size_t size;
            f->p_start = (char *)fc_fifo_linear_write_setup((fc_fifo_t *)(f->user), &size);
            f->p_end = (char *)f->p_start + size;
            f->p_now = f->p_start;
        }
        else
        {
            len = (int)FC_IO_EOF;  // 提前结束
        }
    }
    else  // if (len == (int)FC_IO_EOF)
    {
        // 结束,将环形缓冲区的写指针移动到最新位置
        fc_fifo_linear_write_done((fc_fifo_t *)(f->user), (size_t)f->p_now - (size_t)f->p_start);
    }

    return len;
}

/**
 * @brief 将格式化字符串写入到fc_fifo_t的环形缓冲区中
 *
 * @param fifo
 * @param fmt
 * @param arp
 * @return int
 */
int fc_fifo_vprintf(fc_fifo_t *fifo, const char *fmt, va_list arp)
{
    FC_FILE f = {0};
    {
        size_t size = 0;
        f.user = (void *)(fifo);
        f.p_start = (char *)fc_fifo_linear_write_setup(fifo, &size);
        f.p_end = (char *)f.p_start + size;
        f.p_now = f.p_start;
        f.io.write = __fc_fifo_vprintf_write;
    }

    fc_vfprintf(&f, fmt, arp);

    return f.n;
}

/**
 * @brief fc_port_vprintf的核心实现,将格式化字符串写入到fc_port_t的环形缓冲区中
 *
 * @param port
 * @param fmt
 * @param arp
 * @return int
 */
int fc_port_vprintf(fc_port_t *port, size_t rb_index, const char *fmt, va_list arp)
{
    return fc_fifo_vprintf(port->rb[rb_index], fmt, arp);
}

#else

/**
 * @brief 直接使用fc_fifo_t提供的接口进行写入
 *
 * @param buf
 * @param len 实际fc_vfprintf每次只会写入1字节
 * @return int
 */
static int __fc_fifo_vprintf_write(FC_FILE *f, const void *buf, int len)
{
    if (1 == len)
    {
        // 优化一定性能
        if (false == fc_fifo_write_byte((fc_fifo_t *)(f->user), *(uint8_t *)buf))
        {
            len = (int)FC_IO_EOF;  // 提前结束
        }
    }
    #if 0
    else if (len > (int)FC_IO_SWAP)
    {
        // 不可能走这里
        len = fc_fifo_write((fc_fifo_t *)(f->user), (void *)(buf), len);
    }
    #endif

    return len;
}

/**
 * @brief 将格式化字符串写入到fc_fifo_t的环形缓冲区中
 *
 * @param fifo
 * @param fmt
 * @param arp
 * @return int
 */
int fc_fifo_vprintf(fc_fifo_t *fifo, const char *fmt, va_list arp)
{
    FC_FILE f = {0};

    f.user = (void *)(fifo);
    f.io.write = __fc_fifo_vprintf_write;

    fc_vfprintf(&f, fmt, arp);

    return f.n;
}

/**
 * @brief fc_port_vprintf的核心实现,将格式化字符串写入到fc_port_t的环形缓冲区中
 *
 * @param port
 * @param fmt
 * @param arp
 * @return int
 */
int fc_port_vprintf(fc_port_t *port, size_t rb_index, const char *fmt, va_list arp)
{
    return fc_fifo_vprintf(port->rb[rb_index], fmt, arp);
}

#endif
