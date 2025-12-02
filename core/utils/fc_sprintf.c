/**
 * @file fc_sprintf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-19
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>

#include "../fc_stdio.h"

int fc_sprintf(char *s, const char *fmt, ...)
{
    int n;

    va_list ap;
    va_start(ap, fmt);
    n = fc_vsprintf(s, fmt, ap);
    va_end(ap);

    return n;
}
