/**
 * @file fc_compiler.h
 * @author fool-cat (2696652257@qq.com)
 * @brief 裁切自RTThread
 * @version 1.0
 * @date 2025-02-11
 *
 * @copyright Copyright (c) 2025
 *
 */

/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-18     Shell        Separate the compiler porting from rtdef.h
 */
//> 单次包含宏定义
#ifndef __FC_COMPILER_H__
#define __FC_COMPILER_H__

// clang-format off

//+********************************* GNU Extension Check **********************************/
/**
 * @brief Check if GNU extensions are enabled
 * @note fc_embed requires GNU extensions for __attribute__, typeof, etc.
 * 
 * For ARM Compiler 5 (AC5):
 *   Project Options -> C/C++ -> Check "Use MicroLIB" or enable GNU extensions
 * 
 * For ARM Compiler 6 (AC6):
 *   Project Options -> C/C++ -> Language C -> Select GNU dialect (e.g., gnu11, gnu99)
 * 
 * For GCC/Clang:
 *   GNU extensions are enabled by default
 */
#if defined(__ARMCC_VERSION)
    #if !defined(__GNUC__)
        #error "fc_embed requires GNU extensions! Please enable GNU dialect in Keil Project Options -> C/C++ settings."
    #endif
#endif

#if defined(__ARMCC_VERSION)        /* ARM Compiler */
#define fc_section(x)               __attribute__((section(x)))
#define fc_used                     __attribute__((used))
#define fc_align(n)                 __attribute__((aligned(n)))
#if __ARMCC_VERSION >= 6010050
#define fc_packed(declare)          declare __attribute__((packed))
#else
#define fc_packed(declare)          declare
#endif
#define fc_weak                     __attribute__((weak))
#define fc_typeof                   typeof
#define fc_noreturn
#define fc_inline                   static __inline
#define fc_always_inline            fc_inline
#elif defined (__IAR_SYSTEMS_ICC__) /* for IAR Compiler */
#define fc_section(x)               @ x
#define fc_used                     __root
#define PRAGMA(x)                   _Pragma(#x)
#define fc_align(n)                 PRAGMA(data_alignment=n)
#define fc_packed(declare)          declare
#define fc_weak                     __weak
#define fc_typeof                   __typeof
#define fc_noreturn
#define fc_inline                   static inline
#define fc_always_inline            fc_inline
#elif defined (__GNUC__)|| defined (__clang__) /* GNU GCC Compiler */           
#define fc_section(x)               __attribute__((section(x)))
#define fc_used                     __attribute__((used))
#define fc_align(n)                 __attribute__((aligned(n)))
#define fc_packed(declare)          declare __attribute__((packed))
#define fc_weak                     __attribute__((weak))
#define fc_typeof                   __typeof__
#define fc_noreturn                 __attribute__ ((noreturn))
#define fc_inline                   static __inline
#define fc_always_inline            static inline __attribute__((always_inline))
#elif defined (_MSC_VER)            /* for Visual Studio Compiler */
#define fc_section(x)
#define fc_used
#define fc_align(n)                 __declspec(align(n))
#define fc_packed(declare)          __pragma(pack(push, 1)) declare __pragma(pack(pop))
#define fc_weak
#define fc_typeof                   typeof
#define fc_noreturn
#define fc_inline                   static __inline
#define fc_always_inline            fc_inline
#else                              /* Unkown Compiler */
    #error not supported tool chain
#endif /* __ARMCC_VERSION */

#if defined(__GNUC__) && __GNUC__ >= 3
    #define FMT_ARG(n) __attribute__((__format_arg__(n)))
#else
    #define FMT_ARG(n)
#endif



#endif // __FC_COMPILER_H__
