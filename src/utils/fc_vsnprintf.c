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
    // 调用write的时候缓冲区一定用完了,置为空防止后续写入
    f->p_now = NULL;  // 直接置空,防止后续写入
    return len;       // 直接返回模拟写入成功
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

    if (s && n >= 2)
    {
        f.p_now = s;
        f.p_start = s;
        f.p_end = s + n - 1;  // 预留一个字节给\0
    }
    else
    {
        f.p_now = NULL;  // 完全通过write函数控制
    }
    f.io.write = __fc_vsnprintf_write;

    fc_vfprintf(&f, fmt, ap);

    if (f.n >= n && s && n > 0)
    {
        s[n - 1] = '\0';  // 截断
    }
    else if (s && f.p_now)
    {
        *f.p_now = '\0';  // 结尾
    }

    return f.n;
}
