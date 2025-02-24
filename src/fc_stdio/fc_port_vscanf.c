/**
 * @file fc_port_vscanf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-22
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>

#include "fc_stdio.h"

#include "fc_port.h"

static int __fc_port_vscanf_read(FC_FILE* f, void* buf, int len)
{
    if (len > 0)
    {
        len = fc_fifo_write(((fc_port_t*)(f->user))->rb, (void*)(buf), len);
    }
    return len;
}

int fc_port_vscanf(fc_port_t* port, const char* fmt, va_list arp)
{
    FC_FILE f = {0};

    return f.n;
}
