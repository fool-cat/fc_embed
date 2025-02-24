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
 * 强烈推荐作者的微信公众号(裸机思维)<为宏正名>系列文章
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

// 支持使用overlay文件覆盖默认配置
#ifdef FC_CONFIG_HEADER
    #include FC_CONFIG_HEADER
#endif

// for IAR
#undef __IS_COMPILER_IAR__
#if defined(__IAR_SYSTEMS_ICC__)
    #define __IS_COMPILER_IAR__ 1
#endif

// for arm compiler 5
#undef __IS_COMPILER_ARM_COMPILER_5__
#if ((__ARMCC_VERSION >= 5000000) && (__ARMCC_VERSION < 6000000))
    #define __IS_COMPILER_ARM_COMPILER_5__ 1
#endif

// for arm compiler 6

#undef __IS_COMPILER_ARM_COMPILER_6__
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    #define __IS_COMPILER_ARM_COMPILER_6__ 1
#endif
#undef __IS_COMPILER_ARM_COMPILER__
#if defined(__IS_COMPILER_ARM_COMPILER_5__) && __IS_COMPILER_ARM_COMPILER_5__ || defined(__IS_COMPILER_ARM_COMPILER_6__) && __IS_COMPILER_ARM_COMPILER_6__
    #define __IS_COMPILER_ARM_COMPILER__ 1
#endif

// for clang
#undef __IS_COMPILER_LLVM__
#if defined(__clang__) && !__IS_COMPILER_ARM_COMPILER_6__
    #define __IS_COMPILER_LLVM__ 1
#else

    // for gcc
    #undef __IS_COMPILER_GCC__
    #if defined(__GNUC__) && !(defined(__IS_COMPILER_ARM_COMPILER__) || defined(__IS_COMPILER_LLVM__) || defined(__IS_COMPILER_IAR__))
        #define __IS_COMPILER_GCC__ 1
    #endif

#endif

#ifdef __PERF_COUNT_PLATFORM_SPECIFIC_HEADER__
    #include __PERF_COUNT_PLATFORM_SPECIFIC_HEADER__
#endif

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunknown-warning-option"
    #pragma clang diagnostic ignored "-Wreserved-identifier"
    #pragma clang diagnostic ignored "-Wdeclaration-after-statement"
    #pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
    #pragma clang diagnostic ignored "-Wgnu-statement-expression"
    #pragma clang diagnostic ignored "-Wunused-but-set-variable"
    #pragma clang diagnostic ignored "-Wshadow"
    #pragma clang diagnostic ignored "-Wshorten-64-to-32"
    #pragma clang diagnostic ignored "-Wcompound-token-split-by-macro"
    #pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#elif defined(__IS_COMPILER_ARM_COMPILER_5__)
    #pragma diag_suppress 550
#elif defined(__IS_COMPILER_GCC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    #pragma GCC diagnostic ignored "-Wformat="
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

#if __PLOOC_VA_NUM_ARGS() != 0
    #error Please enable GNU extensions!
#endif

// clang-format off
#undef __CONNECT2
#undef __CONNECT3
#undef __CONNECT4
#undef __CONNECT5
#undef __CONNECT6
#undef __CONNECT7
#undef __CONNECT8
#undef __CONNECT9

#undef CONNECT2
#undef CONNECT3
#undef CONNECT4
#undef CONNECT5
#undef CONNECT6
#undef CONNECT7
#undef CONNECT8
#undef CONNECT9

#define __CONNECT2(__A, __B)                        __A##__B
#define __CONNECT3(__A, __B, __C)                   __A##__B##__C
#define __CONNECT4(__A, __B, __C, __D)              __A##__B##__C##__D
#define __CONNECT5(__A, __B, __C, __D, __E)         __A##__B##__C##__D##__E
#define __CONNECT6(__A, __B, __C, __D, __E, __F)    __A##__B##__C##__D##__E##__F
#define __CONNECT7(__A, __B, __C, __D, __E, __F, __G)                           \
                                                    __A##__B##__C##__D##__E##__F##__G
#define __CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H)                      \
                                                    __A##__B##__C##__D##__E##__F##__G##__H
#define __CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I)                 \
                                                    __A##__B##__C##__D##__E##__F##__G##__H##__I

#define ALT_CONNECT2(__A, __B)              __CONNECT2(__A, __B)
#define CONNECT2(__A, __B)                  __CONNECT2(__A, __B)
#define CONNECT3(__A, __B, __C)             __CONNECT3(__A, __B, __C)
#define CONNECT4(__A, __B, __C, __D)        __CONNECT4(__A, __B, __C, __D)
#define CONNECT5(__A, __B, __C, __D, __E)   __CONNECT5(__A, __B, __C, __D, __E)
#define CONNECT6(__A, __B, __C, __D, __E, __F)                                  \
                                            __CONNECT6(__A, __B, __C, __D, __E, __F)
#define CONNECT7(__A, __B, __C, __D, __E, __F, __G)                             \
                                            __CONNECT7(__A, __B, __C, __D, __E, __F, __G)
#define CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H)                        \
                                            __CONNECT8(__A, __B, __C, __D, __E, __F, __G, __H)
#define CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I)                   \
                                            __CONNECT9(__A, __B, __C, __D, __E, __F, __G, __H, __I)

/* link symbol */
#define CONNECT(...)                                                            \
            ALT_CONNECT2(CONNECT, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#undef __using1
#undef __using2
#undef __using3
#undef __using4
#undef using

#define __using1(__declare)                                                     \
            for (__declare, *CONNECT3(__using_, __LINE__,_ptr) = NULL;          \
                 CONNECT3(__using_, __LINE__,_ptr)++ == NULL;                   \
                )

#define __using2(__declare, __on_leave_expr)                                    \
            for (__declare, *CONNECT3(__using_, __LINE__,_ptr) = NULL;          \
                 CONNECT3(__using_, __LINE__,_ptr)++ == NULL;                   \
                 (__on_leave_expr)                                              \
                )

#define __using3(__declare, __on_enter_expr, __on_leave_expr)                   \
            for (__declare, *CONNECT3(__using_, __LINE__,_ptr) = NULL;          \
                 CONNECT3(__using_, __LINE__,_ptr)++ == NULL ?                  \
                    ((__on_enter_expr),1) : 0;                                  \
                 (__on_leave_expr)                                              \
                )

#define __using4(__dcl1, __dcl2, __on_enter_expr, __on_leave_expr)              \
            for (__dcl1, __dcl2, *CONNECT3(__using_, __LINE__,_ptr) = NULL;     \
                 CONNECT3(__using_, __LINE__,_ptr)++ == NULL ?                  \
                    ((__on_enter_expr),1) : 0;                                  \
                 (__on_leave_expr)                                              \
                )

/* single cycle */
#define using(...)                                                              \
                CONNECT2(__using, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#undef __with1
#undef __with2
#undef __with3
#undef with

#define __with1(__addr)                                                         \
            using(__typeof__(*__addr) *_=(__addr))
#define __with2(__type, __addr)                                                 \
            using(__type *_=(__addr))
#define __with3(__type, __addr, __item)                                         \
            using(__type *_=(__addr), *__item = _, _=_,_=_ )

/* Utilities */
#define with(...)                                                               \
            CONNECT2(__with, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#undef _

/* Utilities */
#ifndef dimof
    #define dimof(__array)          (sizeof(__array)/sizeof(__array[0]))
#endif

/* Utilities */
#define SAFE_NAME(__NAME)   CONNECT3(__,__NAME,__LINE__)

#undef foreach1
#undef foreach2
#undef foreach3
#undef foreach

#define foreach1(__array)                                                       \
            using(__typeof__(__array[0]) *_ = __array)                          \
            for (   uint_fast32_t SAFE_NAME(count) = dimof(__array);            \
                    SAFE_NAME(count) > 0;                                       \
                    _++, SAFE_NAME(count)--                                     \
                )

#define foreach2(__type, __array)                                               \
            using(__type *_ = __array)                                          \
            for (   uint_fast32_t SAFE_NAME(count) = dimof(__array);            \
                    SAFE_NAME(count) > 0;                                       \
                    _++, SAFE_NAME(count)--                                     \
                )

#define foreach3(__type, __array, __item)                                       \
            using(__type *_ = __array, *__item = _, _ = _, _ = _ )              \
            for (   uint_fast32_t SAFE_NAME(count) = dimof(__array);            \
                    SAFE_NAME(count) > 0;                                       \
                    _++, __item = _, SAFE_NAME(count)--                         \
                )

/* Utilities */
#define foreach(...)                                                            \
            CONNECT2(foreach, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)
// clang-format on
//+*********************************  **********************************/
/* Common Utilities */
#define FC_UNUSED(x) (void)(x)

/* stringfy */
#define __FC_STRINGIFY(x...) #x
#define FC_STRINGFY(x...) __FC_STRINGIFY(x)

/* compile time assertion */
#ifndef FC_STATIC_ASSERT
    #define FC_STATIC_ASSERT(expn, msg) typedef char SAFE_NAME(fc_static_assert_fail_at_)[(expn) ? 1 : -1]
#endif

/* dynamic assert */
#ifndef fc_assert
    // 直接卡死在断言失败的位置,方便调试
    #define fc_assert(exp) \
        do                 \
        {                  \
            if (!(exp))    \
                while (1)  \
                {          \
                }          \
        } while (0)
#endif

#endif  // __FC_HELPER_H__
