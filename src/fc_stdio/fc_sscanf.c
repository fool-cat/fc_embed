/**
 * @file fc_sscanf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-20
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>
#include "fc_stdio.h"

int sscanf(const char* s, const char* fmt, ...)
{
    int     ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsscanf(s, fmt, ap);
    va_end(ap);
    return ret;
}
