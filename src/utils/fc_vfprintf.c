/*------------------------------------------------------------------------/
/  Universal String Handler for Console Input and Output
/-------------------------------------------------------------------------/
/
/ Copyright (C) 2021, ChaN, all right reserved.
/
/ xprintf module is an open source software. Redistribution and use of
/ xprintf module in source and binary forms, with or without modification,
/ are permitted provided that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/-------------------------------------------------------------------------*/

/**
 * @file fc_vfprintf.c
 * @author fool_cat (2696652257@qq.com)
 * @brief 修改自xprintf,实现了一个简单的vfprintf
 * @version 1.0
 * @date 2025-02-17
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "fc_config.h"
#include "../fc_stdio.h"

//+********************************* xprintf core function config **********************************/
// clang-format off
/* output */
#ifndef XF_USE_LLI
    #define XF_USE_LLI  1 /* 1: Enable long long integer in size prefix ll */
#endif

#ifndef XF_USE_FP
    #define XF_USE_FP   1 /* 1: Enable support for floating point in type e and f */
#endif

#ifndef XF_DPC
    #define XF_DPC      '.' /* Decimal separator for floating point */
#endif

// clang-format on

#ifndef SZB_OUTPUT
    #define SZB_OUTPUT 32
#endif

//+*********************************  **********************************/

#if XF_USE_FP
    /*----------------------------------------------*/
    /* Floating point output                        */
    /*----------------------------------------------*/
    #include <math.h>

static int ilog10(double n) /* Calculate log10(n) in integer output */
{
    int rv = 0;

    while (n >= 10)
    { /* Decimate digit in right shift */
        if (n >= 100000)
        {
            n /= 100000;
            rv += 5;
        }
        else
        {
            n /= 10;
            rv++;
        }
    }
    while (n < 1)
    { /* Decimate digit in left shift */
        if (n < 0.00001)
        {
            n *= 100000;
            rv -= 5;
        }
        else
        {
            n *= 10;
            rv--;
        }
    }
    return rv;
}

static double i10x(int n) /* Calculate 10^n */
{
    double rv = 1;

    while (n > 0)
    { /* Left shift */
        if (n >= 5)
        {
            rv *= 100000;
            n -= 5;
        }
        else
        {
            rv *= 10;
            n--;
        }
    }
    while (n < 0)
    { /* Right shift */
        if (n <= -5)
        {
            rv /= 100000;
            n += 5;
        }
        else
        {
            rv /= 10;
            n++;
        }
    }
    return rv;
}

static void ftoa(
    char  *buf,  /* Buffer to output the generated string */
    double val,  /* Real number to output */
    int    prec, /* Number of fractinal digits */
    char   fmt   /* Notation */
)
{
    int         d;
    int         e = 0, m = 0;
    char        sign = 0;
    double      w;
    const char *er = 0;

    if (isnan(val))
    { /* Not a number? */
        er = "NaN";
    }
    else
    {
        if (prec < 0)
            prec = 6; /* Default precision (6 fractional digits) */
        if (val < 0)
        { /* Nagative value? */
            val = -val;
            sign = '-';
        }
        else
        {
            sign = '+';
        }
        if (isinf(val))
        { /* Infinite? */
            er = "INF";
        }
        else
        {
            if (fmt == 'f')
            {                           /* Decimal notation? */
                val += i10x(-prec) / 2; /* Round (nearest) */
                m = ilog10(val);
                if (m < 0)
                    m = 0;
                if (m + prec + 3 >= SZB_OUTPUT)
                    er = "OV"; /* Buffer overflow? */
            }
            else
            { /* E notation */
                if (val != 0)
                {                                        /* Not a true zero? */
                    val += i10x(ilog10(val) - prec) / 2; /* Round (nearest) */
                    e = ilog10(val);
                    if (e > 99 || prec + 6 >= SZB_OUTPUT)
                    { /* Buffer overflow or E > +99? */
                        er = "OV";
                    }
                    else
                    {
                        if (e < -99)
                            e = -99;
                        val /= i10x(e); /* Normalize */
                    }
                }
            }
        }
        if (!er)
        { /* Not error condition */
            if (sign == '-')
                *buf++ = sign; /* Add a - if negative value */
            do
            {                /* Put decimal number */
                w = i10x(m); /* Snip the highest digit d */
                d = val / w;
                val -= d * w;
                if (m == -1)
                    *buf++ = XF_DPC; /* Insert a decimal separarot if get into fractional part */
                *buf++ = '0' + d;    /* Put the digit */
            } while (--m >= -prec); /* Output all digits specified by prec */
            if (fmt != 'f')
            { /* Put exponent if needed */
                *buf++ = fmt;
                if (e < 0)
                {
                    e = -e;
                    *buf++ = '-';
                }
                else
                {
                    *buf++ = '+';
                }
                *buf++ = '0' + e / 10;
                *buf++ = '0' + e % 10;
            }
        }
    }
    if (er)
    { /* Error condition? */
        if (sign)
            *buf++ = sign; /* Add sign if needed */
        do
            *buf++ = *er++;
        while (*er); /* Put error symbol */
    }
    *buf = 0; /* Term */
}
#endif /* XF_USE_FLOAT */

//+********************************* vfprintf **********************************/
static bool _write_ch(FC_FILE *f, char ch)
{
    if (f->p_now)
    {
        *f->p_now++ = (char)ch;
        f->n++;
        if ((size_t)f->p_now >= (size_t)f->p_end)
        {
            f->p_now = NULL;
            if (f->io.write)
            {
                int _size = (int)(f->p_end - f->p_start);
                if (_size != f->io.write(f, f->p_start, _size))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }
    else if (f->io.write)
    {
        if (1 != f->io.write(f, &ch, 1))
        {
            return false;
        }
        f->n++;
    }
    else
    {
        return false;
    }

    return true;
}

// 不使用do{}while(0)的结构节省一条指令的性能开销
#undef __fc_fputc
#define __fc_fputc(f, ch)            \
    {                                \
        if (!_write_ch(f, (char)ch)) \
            goto _exit;              \
    }

// 退出的后处理
#undef __fc_exit_handle
#define __fc_exit_handle(f)          \
    if (f->io.write)                 \
    {                                \
        f->io.write(f, f->p_now, 0); \
    }

int fc_vfprintf(
    FC_FILE    *pf,  /* Pointer to the file object */
    const char *fmt, /* Pointer to the format string */
    va_list     arp  /* Pointer to arguments */
)
{
    unsigned int r, i, j, w, f;
    int          n, prec;
    char         str[SZB_OUTPUT], c, d, *p, pad;
#if XF_USE_LLI
    long long          v;
    unsigned long long uv;
#else
    long          v;
    unsigned long uv;
#endif

    for (;;)
    {
        c = *fmt++; /* Get a format character */
        if (!c)
            break; /* End of format? */
        if (c != '%')
        { /* Pass it through if not a % sequense */
            __fc_fputc(pf, c);
            continue;
        }
        f = w = 0; /* Clear parms */
        pad = ' ';
        prec = -1;
        c = *fmt++; /* Get first char of the sequense */
        if (c == '0')
        { /* Flag: left '0' padded */
            pad = '0';
            c = *fmt++;
        }
        else
        {
            if (c == '-')
            { /* Flag: left justified */
                f = 2;
                c = *fmt++;
            }
        }
        if (c == '*')
        { /* Minimum width from an argument */
            n = va_arg(arp, int);
            if (n < 0)
            { /* Flag: left justified */
                n = 0 - n;
                f = 2;
            }
            w = n;
            c = *fmt++;
        }
        else
        {
            while (c >= '0' && c <= '9')
            { /* Minimum width */
                w = w * 10 + c - '0';
                c = *fmt++;
            }
        }
        if (c == '.')
        { /* Precision */
            c = *fmt++;
            if (c == '*')
            { /* Precision from an argument */
                prec = va_arg(arp, int);
                c = *fmt++;
            }
            else
            {
                prec = 0;
                while (c >= '0' && c <= '9')
                {
                    prec = prec * 10 + c - '0';
                    c = *fmt++;
                }
            }
        }
        if (c == 'l')
        { /* Prefix: Size is long */
            f |= 4;
            c = *fmt++;
#if XF_USE_LLI
            if (c == 'l')
            { /* Prefix: Size is long long */
                f |= 8;
                c = *fmt++;
            }
#endif
        }
        if (!c)
            break; /* End of format? */
        switch (c)
        {         /* Type is... */
        case 'b': /* Unsigned binary */
            r = 2;
            break;
        case 'o': /* Unsigned octal */
            r = 8;
            break;
        case 'd': /* Signed decimal */
        case 'u': /* Unsigned decimal */
            r = 10;
            break;
        case 'x': /* Hexdecimal (lower case) */
        case 'X': /* Hexdecimal (upper case) */
            r = 16;
            break;
        case 'c': /* A character */
        {
            __fc_fputc(pf, (char)va_arg(arp, int));
            continue;
        }
        case 's':                    /* String */
            p = va_arg(arp, char *); /* Get a pointer argument */
            if (!p)
                p = (char *)""; /* Null ptr generates a null string */
            j = strlen(p);
            if (prec >= 0 && j > (unsigned int)prec)
                j = prec; /* Limited length of string body */
            for (; !(f & 2) && j < w; j++)
            {
                __fc_fputc(pf, pad); /* Left pads */
            }
            while (*p && prec--)
            {
                __fc_fputc(pf, *p++); /* String body */
            }
            while (j++ < w)
            {
                __fc_fputc(pf, ' '); /* Right pads */
            }
            continue;
#if XF_USE_FP
        case 'f':                                        /* Float (decimal) */
        case 'e':                                        /* Float (e) */
        case 'E':                                        /* Float (E) */
            ftoa(p = str, va_arg(arp, double), prec, c); /* Make fp string */
            for (j = strlen(p); !(f & 2) && j < w; j++)
            {
                __fc_fputc(pf, pad); /* Left pads */
            }
            while (*p)
            {
                __fc_fputc(pf, *p++); /* Value */
            }
            while (j++ < w)
            {
                __fc_fputc(pf, ' '); /* Right pads */
            }
            continue;
#endif
        default: /* Unknown type (passthrough) */
        {
            __fc_fputc(pf, c);
            continue;
        }
        }

        /* Get an integer argument and put it in numeral */
#if XF_USE_LLI
        if (f & 8)
        { /* long long argument? */
            v = (long long)va_arg(arp, long long);
        }
        else
        {
            if (f & 4)
            { /* long argument? */
                v = (c == 'd') ? (long long)va_arg(arp, long) : (long long)va_arg(arp, unsigned long);
            }
            else
            { /* int/short/char argument */
                v = (c == 'd') ? (long long)va_arg(arp, int) : (long long)va_arg(arp, unsigned int);
            }
        }
#else
        if (f & 4)
        { /* long argument? */
            v = (long)va_arg(arp, long);
        }
        else
        { /* int/short/char argument */
            v = (c == 'd') ? (long)va_arg(arp, int) : (long)va_arg(arp, unsigned int);
        }
#endif
        if (c == 'd' && v < 0)
        { /* Negative value? */
            v = 0 - v;
            f |= 1;
        }
        i = 0;
        uv = v;
        do
        { /* Make an integer number string */
            d = (char)(uv % r);
            uv /= r;
            if (d > 9)
                d += (c == 'x') ? 0x27 : 0x07;
            str[i++] = d + '0';
        } while (uv != 0 && i < sizeof str);
        if (f & 1)
            str[i++] = '-'; /* Sign */
        for (j = i; !(f & 2) && j < w; j++)
        {
            __fc_fputc(pf, pad); /* Left pads */
        }
        do
        {
            __fc_fputc(pf, str[--i]);
        } while (i != 0); /* Value */
        while (j++ < w)
        {
            __fc_fputc(pf, ' '); /* Right pads */
        }
    }

_exit:
    __fc_exit_handle(pf);

    return pf->n;
}
