/**
 * @file fc_vsscanf.c
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

int fc_vsscanf(const char *s, const char *fmt, va_list ap)
{
    int     ret;
    FC_FILE f;
    f.p_start = (char *)s;
    f.p_now = (char *)s;
    f.p_end = (char *)s + strlen(s);
    ret = fc_vfscanf(&f, fmt, ap);
    return ret;
}
