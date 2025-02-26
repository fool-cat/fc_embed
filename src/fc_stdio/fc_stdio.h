/**
 * @file fc_stdio.h
 * @author fool_cat (2696652257@qq.com)
 * @brief 核心实现参考xprintf,极度简化标准stdio
 * @version 1.0
 * @date 2025-02-17
 *
 * @copyright Copyright (c) 2025
 *
 */

// > 单次包含宏定义
#ifndef _FC_STDIO_CONFIG_H_
#define _FC_STDIO_CONFIG_H_

#include <stdarg.h>
#include <stdio.h>

#ifdef FC_CONFIG_HEADER
    #include FC_CONFIG_HEADER
#endif

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

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

/* input */
#ifndef XF_INPUT_ECHO
    #define XF_INPUT_ECHO 0 /* 1: Echo back input chars in xgets function */
#endif

    // clang-format on

    //+********************************* 仿标准stdio **********************************/

    /**
     * @brief FC_FILE对象的读写会优先使用内存地址进行操作,其次才是调用write和read函数
     *
     */
    typedef struct _FC_FILE FC_FILE;
    struct _FC_FILE
    {
        char*  p_start;
        char*  p_now;
        char*  p_end;
        size_t n;  // 写入或者读出的字节数

        void* user; /* 用户自定义数据 */

        // write和read函数传入长度为0表示一些尾处理,需要处理长度以及处理内存等操作
        int (*write)(FC_FILE* f, const void* buf, int len);  // 返回实际写入的长度,实际每次只会写入1字节
        int (*read)(FC_FILE* f, void* buf, int len);         // 返回实际读取的长度,实际每次只会读取1字节
    };

    typedef int (*fc_file_write_t)(FC_FILE* f, const void* buf, int len);
    typedef int (*fc_file_read_t)(FC_FILE* f, void* buf, int len);

    //+********************************* 格式化函数 **********************************/

    extern int fc_sprintf(char* buf, const char* fmt, ...);
    extern int fc_snprintf(char* buf, size_t size, const char* fmt, ...);
    extern int fc_vsprintf(char* buf, const char* fmt, va_list arp);
    extern int fc_vsnprintf(char* buf, size_t size, const char* fmt, va_list arp);  // 字符串格式化核心函数

    extern int fc_sscanf(const char* __restrict, const char* __restrict, ...);
    extern int fc_vsscanf(const char* __restrict, const char* __restrict, va_list);
    extern int fc_vfscanf(FC_FILE* f, const char* fmt, va_list arp);

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_STDIO_CONFIG_H_
