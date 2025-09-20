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

#include "../fc_stdio.h"

/**
 * @brief 具体查看vfprintf.c中_write_ch函数
 *
 *
 * @param buf
 * @param len 每次只会写入1字节
 * @return int
 */
static int __fc_vsnprintf_write(FC_FILE *f, const void *buf, int len)
{
    if (f->p_start && (f->p_start + f->n <= f->p_end) && len > 0)
    {
        char *p_now = f->p_start + f->n;
        for (int i = 0; i < len; i++)
        {
            if (p_now <= f->p_end)
            {
                p_now[i] = ((char *)buf)[i];
                p_now++;
            }
            else
            {
                f->p_start = NULL;  // 优化下一次进入性能
                break;              // 超出范围后不再写入
            }
        }
    }
    return len;
}

/**
 * @brief 格式化字符串写入到缓冲区中,类似于标准库的vsnprintf
 *
 * @param s
 * @param n
 * @param fmt
 * @param ap
 * @return int 返回需要的缓冲区大小,不包括结尾的\0,大于等于n表示缓冲区不够
 */
int fc_vsnprintf(char *s, size_t n, const char *fmt, va_list ap)
{
    FC_FILE f = {0};

    f.p_now = NULL;  // 不赋值p_now,全部靠write函数写入
    f.p_start = (s && n >= 2) ? s : NULL;
    f.p_end = (s && n >= 2) ? (s + n - 1) : NULL;
    f.io.write = __fc_vsnprintf_write;

    n = fc_vfprintf(&f, fmt, ap);
    if (s && n)
    {
        s[n] = '\0';
    }
    return (int)n;
}
