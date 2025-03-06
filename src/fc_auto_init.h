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

    //+********************************* 默认的初始化类型 **********************************/
    // 用于导出带优先级的元素到指定段中
    typedef void (*fc_auto_init_func_t)(void);
    typedef struct
    {
        fc_auto_init_func_t func;   // 函数指针
        size_t              order;  // 优先级
    } fc_auto_init_elem_t;

#define FC_INIT_EXPORT(section_name, func, order)                                           \
    static fc_auto_init_elem_t CONNECT(section_name, _, func, _, __LINE__) = {func, order}; \
    ELEM_EXPORT(section_name, CONNECT(section_name, _, func, _, __LINE__))

    // clang-format off
#define INIT_EXPORT_3(num, func, order) FC_INIT_EXPORT(fc_section_##num, func, order)

    // 导出到指定段中,目前只有0~3段,默认优先级为1000,优先级越低越先执行,对应段的优先级也应该越低越先执行
#define _INIT_EXPORT_ENV_1(func)            INIT_EXPORT_3(0, func, 1000)
#define _INIT_EXPORT_ENV_2(func, order)     INIT_EXPORT_3(0, func, order)

#define _INIT_EXPORT_CLOCK_1(func)          INIT_EXPORT_3(1, func, 1000)
#define _INIT_EXPORT_CLOCK_2(func, order)   INIT_EXPORT_3(1, func, order)

#define _INIT_EXPORT_DEVICE_1(func)         INIT_EXPORT_3(2, func, 1000)
#define _INIT_EXPORT_DEVICE_2(func, order)  INIT_EXPORT_3(2, func, order)

#define _INIT_EXPORT_APP_1(func)            INIT_EXPORT_3(3, func, 1000)
#define _INIT_EXPORT_APP_2(func, order)     INIT_EXPORT_3(3, func, order)
    // clang-format on

    //+********************************* 自定义的四个初始段 **********************************/

    // 这个宏用于声明一个段的起始和结束地址,用于遍历段中的元素,当需要使用fc_foreach宏时,需要先使用这个宏
#define FC_EXTERN(section_name) SECTION_EXTERN(section_name)

// 这个宏用于遍历指定段中的元素,需要先使用SECTION_EXTERN宏声明段的起始和结束地址
#define fc_foreach(section_name, type_ptr) section_foreach(section_name, type_ptr)

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
    extern void fc_section_init_env(void);  //\ __attribute__((constructor)); 无需显式调用
    extern void fc_section_init_clock(void);
    extern void fc_section_init_device(void);
    extern void fc_section_init_app(void);

    /**
     * @brief 最多允许两个参数,第一个参数为需要导出的函数名,第二个参数为优先级
     * 可以缺省优先级,默认为1000,数字越小优先级越高
     *
     *
     * @example
     * void func(void){};          // 需要导出的函数
     * INIT_EXPORT_ENV(func);      // 默认优先级1000
     * INIT_EXPORT_ENV(func, 100); // 优先级100
     */

#define INIT_EXPORT_ENV(...) CONNECT(_INIT_EXPORT_ENV_, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define INIT_EXPORT_CLOCK(...) CONNECT(_INIT_EXPORT_CLOCK_, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define INIT_EXPORT_DEVICE(...) CONNECT(_INIT_EXPORT_DEVICE_, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define INIT_EXPORT_APP(...) CONNECT(_INIT_EXPORT_APP_, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_AUTO_INIT_H_
