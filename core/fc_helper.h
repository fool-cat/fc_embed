/****************************************************************************
 *  Copyright 2024 Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/**
 * @file fc_helper.h
 * @author fool-cat (2696652257@qq.com)
 * @brief 部分辅助定义,大量使用PLOOC(https://github.com/GorgonMeducer/PLOOC)和perf_counter(https://github.com/GorgonMeducer/perf_counter)的源码
 * 强烈推荐作者的微信公众号(裸机思维)<为宏正名>系列文章,需开启GNU C99支持
 * 更新到perf_counter的@tag v2.5.4版本
 * @version 1.0
 * @date 2025-01-30
 *
 * @copyright Copyright (c) 2025
 *
 */

//> 单次包含宏定义
#ifndef __FC_HELPER_H__
#define __FC_HELPER_H__

#include <stdint.h>

#include "fc_compiler.h"

/*============================ MACROS ========================================*/
/*!
 * \addtogroup gHelper 4 Helper
 * @{
 */

// for IAR
#if defined(__IAR_SYSTEMS_ICC__)
    #undef __IS_COMPILER_IAR__
    #define __IS_COMPILER_IAR__ 1

// TI Arm Compiler (armcl)
#elif defined(__TI_ARM__)
    #undef __IS_COMPILER_TI_ARM__
    #define __IS_COMPILER_TI_ARM__ 1

// TASKING Compiler
#elif defined(__TASKING__)
    #undef __IS_COMPLER_TASKING__
    #define __IS_COMPLER_TASKING__ 1

// COSMIC Compiler
#elif defined(__CSMC__)
    #undef __IS_COMPILER_COSMIC__
    #define __IS_COMPILER_COSMIC__ 1

// for arm compiler 5
#elif ((__ARMCC_VERSION >= 5000000) && (__ARMCC_VERSION < 6000000))
    #undef __IS_COMPILER_ARM_COMPILER_5__
    #define __IS_COMPILER_ARM_COMPILER_5__ 1

// for arm compiler 6
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    #undef __IS_COMPILER_ARM_COMPILER_6__
    #define __IS_COMPILER_ARM_COMPILER_6__ 1

// TI Arm Clang Compiler (tiarmclang)
#elif defined(__ti__)
    #undef __IS_COMPILER_TI_ARM_CLANG__
    #define __IS_COMPILER_TI_ARM_CLANG__ 1
#else

    // for other clang
    #if defined(__clang__) &&              \
        !__IS_COMPILER_ARM_COMPILER_6__ && \
        !__IS_COMPILER_TI_ARM_CLANG__
        #undef __IS_COMPILER_LLVM__
        #define __IS_COMPILER_LLVM__ 1

    // for gcc
    #elif defined(__GNUC__) && !(defined(__IS_COMPILER_ARM_COMPILER__) || defined(__IS_COMPILER_LLVM__) || defined(__IS_COMPILER_IAR__))
        #undef __IS_COMPILER_GCC__
        #define __IS_COMPILER_GCC__ 1
    #endif

#endif

#undef __IS_COMPILER_ARM_COMPILER__
#if defined(__IS_COMPILER_ARM_COMPILER_5__) && __IS_COMPILER_ARM_COMPILER_5__ || defined(__IS_COMPILER_ARM_COMPILER_6__) && __IS_COMPILER_ARM_COMPILER_6__
    #define __IS_COMPILER_ARM_COMPILER__ 1
#endif

#ifndef __PLOOC_VA_NUM_ARGS_IMPL
    #define __PLOOC_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, \
                                     _12, _13, _14, _15, _16, __N, ...) __N
#endif

#ifndef __PLOOC_VA_NUM_ARGS
    #define __PLOOC_VA_NUM_ARGS(...)                                              \
        __PLOOC_VA_NUM_ARGS_IMPL(0, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, \
                                 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#undef __COMPILER_HAS_GNU_EXTENSIONS__
#if __PLOOC_VA_NUM_ARGS() == 0
    #define __COMPILER_HAS_GNU_EXTENSIONS__ 1
#endif

#if !__COMPILER_HAS_GNU_EXTENSIONS__
    #warning Please enable GNU extensions!
#endif

#undef __IS_COMPILER_SUPPORT_C99__
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define __IS_COMPILER_SUPPORT_C99__ 1
#endif

#undef __IS_COMPILER_SUPPORT_C11__
#if defined(__STDC_VERSION__) && __STDC_VERSION__ > 199901L
    #define __IS_COMPILER_SUPPORT_C11__ 1
#endif

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunknown-warning-option"
    #pragma clang diagnostic ignored "-Wreserved-identifier"
    #pragma clang diagnostic ignored "-Wtypedef-redefinition"
    #pragma clang diagnostic ignored "-Wmissing-declarations"
    #pragma clang diagnostic ignored "-Wempty-body"
    #pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"
    #pragma clang diagnostic ignored "-Wmissing-field-initializers"
    #pragma clang diagnostic ignored "-Wmissing-declarations"
    #pragma clang diagnostic ignored "-Wmissing-braces"
#elif __IS_COMPILER_ARM_COMPILER_5__
    /*! arm compiler 5 */
    #pragma push
    #pragma diag_suppress 1, 64, 174, 177, 188, 68, 513, 144, 2525
#elif __IS_COMPILER_IAR__
/*! IAR */
#elif __IS_COMPILER_GCC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-declarations"
    #pragma GCC diagnostic ignored "-Wempty-body"
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat="
    #pragma GCC diagnostic ignored "-Wmissing-braces"
    #pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

// clang-format on
/*----------------------------------------------------------------------------*
 * Helpers                                                                    *
 *----------------------------------------------------------------------------*/
#undef _

#undef __FC_CONNECT2
#undef __FC_CONNECT3
#undef __FC_CONNECT4
#undef __FC_CONNECT5
#undef __FC_CONNECT6
#undef __FC_CONNECT7
#undef __FC_CONNECT8
#undef __FC_CONNECT9

#undef FC_CONNECT2
#undef FC_CONNECT3
#undef FC_CONNECT4
#undef FC_CONNECT5
#undef FC_CONNECT6
#undef FC_CONNECT7
#undef FC_CONNECT8
#undef FC_CONNECT9
#undef ALT_FC_CONNECT2

#undef SAFE_NAME
#undef FC_SAFE_NAME

#undef FC_CONNECT

#undef __MACRO_EXPANDING
#define __MACRO_EXPANDING(...) __VA_ARGS__

#define __FC_CONNECT2(__A, __B) __A##__B
#define __FC_CONNECT3(__A, __B, __C) __A##__B##__C
#define __FC_CONNECT4(__A, __B, __C, __D) __A##__B##__C##__D
#define __FC_CONNECT5(__A, __B, __C, __D, __E) \
    __A##__B##__C##__D##__E
#define __FC_CONNECT6(__A, __B, __C, __D, __E, __F) \
    __A##__B##__C##__D##__E##__F
#define __FC_CONNECT7(__A, __B, __C, __D, __E, __F, __G) \
    __A##__B##__C##__D##__E##__F##__G
#define __FC_CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H) \
    __A##__B##__C##__D##__E##__F##__G##__H
#define __FC_CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I) \
    __A##__B##__C##__D##__E##__F##__G##__H##__I

#define ALT_FC_CONNECT2(__A, __B) __FC_CONNECT2(__A, __B)
#define FC_CONNECT2(__A, __B) __FC_CONNECT2(__A, __B)
#define FC_CONNECT3(__A, __B, __C) __FC_CONNECT3(__A, __B, __C)
#define FC_CONNECT4(__A, __B, __C, __D) \
    __FC_CONNECT4(__A, __B, __C, __D)
#define FC_CONNECT5(__A, __B, __C, __D, __E) \
    __FC_CONNECT5(__A, __B, __C, __D, __E)
#define FC_CONNECT6(__A, __B, __C, __D, __E, __F) \
    __FC_CONNECT6(__A, __B, __C, __D, __E, __F)
#define FC_CONNECT7(__A, __B, __C, __D, __E, __F, __G) \
    __FC_CONNECT7(__A, __B, __C, __D, __E, __F, __G)
#define FC_CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H) \
    __FC_CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H)
#define FC_CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I) \
    __FC_CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I)

#define FC_CONNECT(...)         \
    ALT_FC_CONNECT2(FC_CONNECT, \
                    __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define SAFE_NAME(__NAME) FC_CONNECT3(__, __NAME, __LINE__)
#define FC_SAFE_NAME(__name) FC_CONNECT3(__, __name, __LINE__)

#undef __fc_using1
#undef __fc_using2
#undef __fc_using3
#undef __fc_using4
#undef fc_using

#define __fc_using1(__declare)                             \
    for (__declare,                                        \
         *FC_CONNECT3(__fc_using_, __LINE__, _ptr) = NULL; \
         FC_CONNECT3(__fc_using_, __LINE__, _ptr)++ == NULL;)

#define __fc_using2(__declare, __on_leave_expr)              \
    for (__declare,                                          \
         *FC_CONNECT3(__fc_using_, __LINE__, _ptr) = NULL;   \
         FC_CONNECT3(__fc_using_, __LINE__, _ptr)++ == NULL; \
         (__on_leave_expr))

#define __fc_using3(__declare, __on_enter_expr, __on_leave_expr)                          \
    for (__declare,                                                                       \
         *FC_CONNECT3(__fc_using_, __LINE__, _ptr) = NULL;                                \
         FC_CONNECT3(__fc_using_, __LINE__, _ptr)++ == NULL ? ((__on_enter_expr), 1) : 0; \
         (__on_leave_expr))

#define __fc_using4(__dcl1, __dcl2, __on_enter_expr, __on_leave_expr)                     \
    for (__dcl1, __dcl2,                                                                  \
         *FC_CONNECT3(__fc_using_, __LINE__, _ptr) = NULL;                                \
         FC_CONNECT3(__fc_using_, __LINE__, _ptr)++ == NULL ? ((__on_enter_expr), 1) : 0; \
         (__on_leave_expr))

#define fc_using(...)                             \
    FC_CONNECT2(__fc_using,                       \
                __PLOOC_VA_NUM_ARGS(__VA_ARGS__)) \
    (__VA_ARGS__)

#undef __fc_with2
#undef __fc_with3
#undef fc_with

#define __fc_with1(__addr) \
    fc_using(__typeof__(*__addr) *_ = (__addr))

#define __fc_with2(__type, __addr) \
    fc_using(__type *_ = (__addr))
#define __fc_with3(__type, __addr, __item) \
    fc_using(__type *_ = (__addr), *__item = _, _ = _, _ = _)

#define fc_with(...)                              \
    FC_CONNECT2(__fc_with,                        \
                __PLOOC_VA_NUM_ARGS(__VA_ARGS__)) \
    (__VA_ARGS__)

#undef fc_foreach2
#undef fc_foreach3
#undef fc_foreach

#define fc_foreach1(__array)                                                                               \
    fc_using(__typeof__(__array[0]) *_ = __array) for (uint_fast32_t FC_SAFE_NAME(count) = dimof(__array); \
                                                       FC_SAFE_NAME(count) > 0;                            \
                                                       _++, FC_SAFE_NAME(count)--)

#define fc_foreach2(__type, __array)                                                       \
    fc_using(__type *_ = __array) for (uint_fast32_t FC_SAFE_NAME(count) = dimof(__array); \
                                       FC_SAFE_NAME(count) > 0;                            \
                                       _++, FC_SAFE_NAME(count)--)

#define fc_foreach3(__type, __array, __item)                                                                          \
    fc_using(__type *_ = __array, *__item = _, _ = _, _ = _) for (uint_fast32_t FC_SAFE_NAME(count) = dimof(__array); \
                                                                  FC_SAFE_NAME(count) > 0;                            \
                                                                  _++, __item = _, FC_SAFE_NAME(count)--)

#define fc_foreach(...)                           \
    FC_CONNECT2(fc_foreach,                       \
                __PLOOC_VA_NUM_ARGS(__VA_ARGS__)) \
    (__VA_ARGS__)

// clang-format on

#endif  // __FC_HELPER_H__
