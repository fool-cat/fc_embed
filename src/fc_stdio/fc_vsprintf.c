/**
 * @file fc_vsprintf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-19
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <limits.h>
#include <stdarg.h>
#include "fc_stdio.h"

#ifndef INT_MAX
    #define INT_MAX 0x7fffffff
#endif

int fc_vsprintf(char* s, const char* fmt, va_list ap)
{
    return fc_vsnprintf(s, INT_MAX, fmt, ap);
}
