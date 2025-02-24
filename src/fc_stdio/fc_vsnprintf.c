/**
 * @file fc_vsnprintf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-19
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "fc_stdio.h"

int fc_vsnprintf(char* s, size_t n, const char* fmt, va_list ap)
{
    FC_FILE f = {0};

    f.p_now = s;
    f.p_start = s;
    f.p_end = s + n - 1;

    n = fc_vfprintf(&f, fmt, ap);
    s[n] = '\0';
    return (int)n;
}
