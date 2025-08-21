/**
 * @file fc_stdio.c
 * @author fool_cat (2696652257@qq.com)
 * @brief 统一编译
 * @version 1.0
 * @date 2025-02-20
 *
 * @copyright Copyright (c) 2025
 *
 */

// overlay的方式覆盖默认配置
#ifdef FC_CONFIG_HEADER
    #if defined(FC_HEADER_WRAP)
        #define FC_HEADER_STRINGFY(x) #x
        #define FC_INCLUDE_FILE(x) FC_HEADER_STRINGFY(x)
        #include FC_INCLUDE_FILE(FC_CONFIG_HEADER)
    #elif defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000) /* ARM Compiler */
        #define FC_HEADER_STRINGFY(x) #x
        #define FC_INCLUDE_FILE(x) FC_HEADER_STRINGFY(x)
        #include FC_INCLUDE_FILE(FC_CONFIG_HEADER)
    #else
        #include FC_CONFIG_HEADER
    #endif
#endif

#include "fc_stdio.h"

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

//+********************************* 格式化API **********************************/

// sprintf
#include "fc_sprintf.c"

// snprintf
#include "fc_snprintf.c"

// vsprintf
#include "fc_vsprintf.c"

// vsnprintf
#include "fc_vsnprintf.c"

// fprintf
#include "fc_fprintf.c"

// vfprintf
#include "fc_vfprintf.c"  // core function

//+********************************* port API **********************************/

#include "fc_port_vprintf.c"
