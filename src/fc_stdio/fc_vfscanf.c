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
 * @file fc_vfscanf.c
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

int xgets(            /* 0:End of stream, 1:A line arrived */
          char *buff, /* Pointer to the buffer */
          int   len   /* Buffer length */
)
{
    int c, i;
    len--; /* Reserve 1 char for the terminator */

    i = 0;
    for (;;)
    {
        c = _read_ch(f); /* Get a char from the incoming stream */
        if (c < 0 || c == '\r')
            break; /* End of stream or CR? */
        if (c == '\b' && i)
        { /* BS? */
            i--;
            continue;
        }
        if (c >= ' ' && i < len)
        { /* Visible chars? */
            buff[i++] = c;
        }
    }

    buff[i] = 0; /* Terminate with a \0 */
    return (int)(c == '\r');
}

/*----------------------------------------------*/
/* Get a value of integer string                */
/*----------------------------------------------*/
/*	"123 -5   0x3ff 0b1111 0377  w "
  ^                           1st call returns 123 and next ptr
     ^                        2nd call returns -5 and next ptr
             ^                3rd call returns 1023 and next ptr
                    ^         4th call returns 15 and next ptr
                         ^    5th call returns 255 and next ptr
                            ^ 6th call fails and returns 0
*/

int xatoi(            /* 0:Failed, 1:Successful */
          char **str, /* Pointer to pointer to the string */
          long  *res  /* Pointer to the valiable to store the value */
)
{
    unsigned long val;
    unsigned char c, r, s = 0;

    *res = 0;

    while ((c = **str) == ' ')
        (*str)++; /* Skip leading spaces */

    if (c == '-')
    { /* negative? */
        s = 1;
        c = *(++(*str));
    }

    if (c == '0')
    {
        c = *(++(*str));
        switch (c)
        {
        case 'x': /* hexdecimal */
            r = 16;
            c = *(++(*str));
            break;
        case 'b': /* binary */
            r = 2;
            c = *(++(*str));
            break;
        default:
            if (c <= ' ')
                return 1; /* single zero */
            if (c < '0' || c > '9')
                return 0; /* invalid char */
            r = 8;        /* octal */
        }
    }
    else
    {
        if (c < '0' || c > '9')
            return 0; /* EOL or invalid char */
        r = 10;       /* decimal */
    }

    val = 0;
    while (c > ' ')
    {
        if (c >= 'a')
            c -= 0x20;
        c -= '0';
        if (c >= 17)
        {
            c -= 7;
            if (c <= 9)
                return 0; /* invalid char */
        }
        if (c >= r)
            return 0; /* invalid char for current radix */
        val = val * r + c;
        c = *(++(*str));
    }
    if (s)
        val = 0 - val; /* apply sign if needed */

    *res = val;
    return 1;
}

#if XF_USE_FP
/*----------------------------------------------*/
/* Get a value of the real number string        */
/*----------------------------------------------*/
/* Float version of xatoi
 */

int xatof(             /* 0:Failed, 1:Successful */
          char  **str, /* Pointer to pointer to the string */
          double *res  /* Pointer to the valiable to store the value */
)
{
    double        val;
    int           s, f, e;
    unsigned char c;

    *res = 0;
    s = f = 0;

    while ((c = **str) == ' ')
        (*str)++; /* Skip leading spaces */
    if (c == '-')
    { /* Negative? */
        c = *(++(*str));
        s = 1;
    }
    else if (c == '+')
    { /* Positive? */
        c = *(++(*str));
    }
    if (c == XF_DPC)
    {           /* Leading dp? */
        f = -1; /* Start at fractional part */
        c = *(++(*str));
    }
    if (c <= ' ')
        return 0; /* Wrong termination? */
    val = 0;
    while (c > ' ')
    { /* Get a value of decimal */
        if (c == XF_DPC)
        { /* Embedded dp? */
            if (f < 0)
                return 0; /* Wrong dp? */
            f = -1;       /* Enter fractional part */
        }
        else
        {
            if (c < '0' || c > '9')
                break; /* End of decimal? */
            c -= '0';
            if (f == 0)
            { /* In integer part */
                val = val * 10 + c;
            }
            else
            { /* In fractional part */
                val += i10x(f--) * c;
            }
        }
        c = *(++(*str));
    }
    if (c > ' ')
    { /* It may be an exponent */
        if (c != 'e' && c != 'E')
            return 0; /* Wrong character? */
        c = *(++(*str));
        if (c == '-')
        {
            c = *(++(*str));
            s |= 2; /* Negative exponent */
        }
        else if (c == '+')
        {
            c = *(++(*str)); /* Positive exponent */
        }
        if (c <= ' ')
            return 0; /* Wrong termination? */
        e = 0;
        while (c > ' ')
        { /* Get value of exponent */
            c -= '0';
            if (c > 9)
                return 0; /* Not a numeral? */
            e = e * 10 + c;
            c = *(++(*str));
        }
        val *= i10x((s & 2) ? -e : e); /* Apply exponent */
    }

    if (s & 1)
        val = -val; /* Negate sign if needed */

    *res = val;
    return 1;
}
#endif /* XF_USE_FP */
