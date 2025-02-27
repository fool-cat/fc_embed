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

#include "fc_stdio.h"

#include "fc_port.h"

#ifndef FC_PORT_VPRINTF_LINEAR_WRITE
    #define FC_PORT_VPRINTF_LINEAR_WRITE 0
#endif

#if FC_PORT_VPRINTF_LINEAR_WRITE
/**
 * @brief port_write的作用优化为对poer->rb的内存进行直接访问然后赋值给f->p_now,f->p_end
 *
 * @param buf
 * @param len
 * @return int
 */
static int __fc_port_vprintf_write(FC_FILE* f, const void* buf, int len)
{
    if (len > 0)
    {
        fc_fifo_linear_write_done((fc_fifo_t*)(f->user), len);

        // 还有空间
        if (fc_fifo_get_free((fc_fifo_t*)(f->user)) >= 1)
        {
            size_t size;
            f->p_start = (char*)fc_fifo_linear_write_setup((fc_fifo_t*)(f->user), &size);
            f->p_end = (char*)f->p_start + size;
            f->p_now = f->p_start;
        }
        else
        {
            len = -1;  // 保证结束
        }
    }
    else
    {
        // 为0表示结束,将环形缓冲区的写指针移动到最新位置
        fc_fifo_linear_write_done((fc_fifo_t*)(f->user), (size_t)buf - (size_t)f->p_start);
    }

    return len;
}

int fc_port_vprintf(fc_port_t* port, const char* fmt, va_list ap)
{
    FC_FILE f = {0};
    {
        size_t size = 0;
        f.user = (void*)(port->rb);
        f.p_start = (char*)fc_fifo_linear_write_setup(port->rb, &size);
        f.p_end = (char*)f.p_start + size;
        f.p_now = f.p_start;
        f.write = __fc_port_vprintf_write;
    }

    fc_vfprintf(&f, fmt, ap);

    return f.n;
}

#else

/**
 * @brief port_write直接使用rb提供的接口进行写入
 *
 * @param buf
 * @param len 实际fc_vfprintf每次只会写入1字节
 * @return int
 */
static int __fc_port_vprintf_write(FC_FILE* f, const void* buf, int len)
{
    if (len > 0)
    {
        len = fc_fifo_write((fc_fifo_t*)(f->user), (void*)(buf), len);
    }
    return len;
}

int fc_port_vprintf(fc_port_t* port, const char* fmt, va_list ap)
{
    FC_FILE f = {0};

    f.user = (void*)(port->rb);
    f.write = __fc_port_vprintf_write;

    fc_vfprintf(&f, fmt, ap);

    return f.n;
}

#endif
