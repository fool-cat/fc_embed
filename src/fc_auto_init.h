/**
 * @file fc_auto_init.h
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-23
 *
 * @copyright Copyright (c) 2025
 *
 */

// > 单次包含宏定义
#ifndef _FC_AUTO_INIT_H_
#define _FC_AUTO_INIT_H_

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

#include "fc_compiler.h"
#include "fc_helper.h"

#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000) /* ARM Compiler */
    #define SECTION_EXTERN(section_name)                   \
        extern const size_t CONNECT(section_name, $$Base); \
        extern const size_t CONNECT(section_name, $$Limit)

    #define ELEM_EXPORT(section_name, elem) \
        static fc_typeof(&elem) const fc_used fc_section(FC_STRINGFY(section_name)) CONNECT(section_name, _, elem, _, __LINE__) = &elem

    #define section_foreach(section_name, type_ptr) \
        SECTION_EXTERN(section_name);               \
        for (type_ptr = (fc_typeof(type_ptr))(&CONNECT(section_name, $$Base)); (size_t)type_ptr < (size_t)(&CONNECT(section_name, $$Limit)); type_ptr++)

#elif defined(__IAR_SYSTEMS_ICC__) || defined(__ICCARM__) || defined(__ICCRX__) /* for IAR Compiler */

    #warning "IAR Compiler need test"

    #define SECTION_EXTERN(section_name) \
        PRAGMA(section = FC_STRINGFY(section_name))

    #define ELEM_EXPORT(section_name, elem) \
        SECTION_EXTERN(section_name);       \
        static const fc_used fc_section(FC_STRINGFY(section_name)) fc_typeof(&elem) CONNECT(section_name, _, elem, _, __LINE__) = &elem

    #define section_foreach(section_name, type_ptr) \
        for (type_ptr = (fc_typeof(type_ptr))(&CONNECT(section_name, _start)); (size_t)type_ptr < (size_t)(&CONNECT(section_name, _end)); type_ptr++)

#elif defined(__GNUC__) /* GNU GCC Compiler */

    #warning "GNU GCC Compiler need test"

    #define SECTION_EXTERN(section_name)                     \
        extern const size_t CONNECT(__start_, section_name); \
        extern const size_t CONNECT(__stop_, section_name)

    #define ELEM_EXPORT(section_name, elem) \
        static const fc_used fc_section(FC_STRINGFY(section_name)) fc_typeof(&elem) CONNECT(section_name, _, elem, _, __LINE__) = &elem

    #define section_foreach(section_name, type_ptr) \
        SECTION_EXTERN(section_name);               \
        for (type_ptr = (fc_typeof(type_ptr))(&CONNECT(__start_, section_name)); (size_t)type_ptr < (size_t)(&CONNECT(__stop_, section_name)); type_ptr++)

#else /* Unkown Compiler */
    #error not supported tool chain
#endif /* __ARMCC_VERSION */

//+********************************* 建议使用宏 **********************************/

// 这个宏用于声明一个段的起始和结束地址,用于遍历段中的元素,当需要使用fc_foreach宏时,需要先使用这个宏
#define FC_EXTERN(section_name) SECTION_EXTERN(section_name)

// 这个宏用于导出一个元素到指定段中, 这个宏会自动将元素的地址导出到指定段中
//! 注意一个段中只能包含一类元素,不同类型的元素需要放到不同的段中,编译器本身无法保证段中元素的类型一致,需要用户自己保证
#define FC_EXPORT(section_name, elem) ELEM_EXPORT(section_name, elem)

// 这个宏用于遍历指定段中的元素,需要先使用SECTION_EXTERN宏声明段的起始和结束地址
#define fc_foreach(section_name, type_ptr) section_foreach(section_name, type_ptr)

    //+********************************* 自定义的四个初始段 **********************************/

    /**
     * @brief 自动初始化的函数类型为无参无返类型
     *
     *
     * 0. main函数之前自动初始化
     * 1. main函数配置完时钟之后自动初始化
     * 2. main函数配置执行完之后自动初始化,操作系统执行之前
     * 3. 操作系统创建的第一个任务内自动初始化
     *
     * 在描述的位置调用指定函数
     */
    extern void fc_section0_init_func(void);
    extern void fc_section1_init_func(void);
    extern void fc_section2_init_func(void);
    extern void fc_section3_init_func(void);

    typedef void (*fc_auto_init_func_t)(void);

    // 导出到指定段中,目前只有0~3段
#define INIT_EXPORT(num, func) ELEM_EXPORT(CONNECT(fc_auto_init_, num), func)

#define INIT_EXPORT_ENV(func) INIT_EXPORT(0, func)

#define INIT_EXPORT_CLOCK(func) INIT_EXPORT(1, func)

#define INIT_EXPORT_DEVICE(func) INIT_EXPORT(2, func)

#define INIT_EXPORT_APP(func) INIT_EXPORT(3, func)

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_AUTO_INIT_H_
