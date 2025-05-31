/**
 * @file fc_vscanf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>
#include "fc_stdio.h"

int fc_vscanf(const char *fmt, va_list ap)
{
    int ret;
    ret = fc_vfscanf(stdin, fmt, ap);
    return ret;
}
