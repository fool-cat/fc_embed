/**
 * @file fc_fprintf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>
#include "../fc_stdio.h"

int fc_fprintf(FC_FILE *f, const char *fmt, ...)
{
    int     ret;
    va_list ap;
    va_start(ap, fmt);
    ret = fc_vfprintf(f, fmt, ap);
    va_end(ap);
    return ret;
}
