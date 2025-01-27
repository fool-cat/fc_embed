/**
 * @brief 宏是黑魔法,如无必要,请按照说明使用即可,不要尝试修改
 * ! 请至少使用C99标准,建议开启GNU扩展
 */

#ifndef _SECTION_H_
#define _SECTION_H_

#include <stddef.h>
#include <stdint.h>

// 拼接宏
#define SECTION_NEW_NAME(name, type) name##_##type

// 字符串化辅助宏（确保宏参数展开后再字符串化）
#define STRINGIFY(x) #x

// 再套一层宏，用于命名正常
#define SECTION_STR_WRAP(x) STRINGIFY(x)

// 字符串化宏（正确处理嵌套宏的问题）
#define SECTION_NEW_NAME_STR(name, type) SECTION_STR_WRAP(SECTION_NEW_NAME(name, type))

/**
 * @brief 定义SECTION宏
 */
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000)
    #define SECTION(x) __attribute__((section(x)))
#elif defined(__ICCARM__) || defined(__ICCRX__)
    #define SECTION(x) @x
#elif defined(__GNUC__)
    #define SECTION(x) __attribute__((section(x)))
#elif defined(_MSC_VER)
    #define SECTION(x) __declspec(allocate(x))
    #pragma section(#x, read)
#else
    #define SECTION(x)
#endif

/**
 * @brief 定义SECTION_START、SECTION_END、SECTION_SIZEOF宏
 */
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000)
    #define SECTION_START(name) name$$Base
    #define SECTION_END(name) name$$Limit
    #define SECTION_SIZEOF(name) ((size_t)name$$Limit - (size_t)name$$Base)

#elif defined(__ICCARM__) || defined(__ICCRX__)
    #define SECTION_START(name) __section_begin(#name)
    #define SECTION_END(name) __section_end(#name)
    #define SECTION_SIZEOF(name) ((size_t)__section_end(#name) - (size_t)__section_begin(#name))

#elif defined(__GNUC__)
    #define SECTION_START(name) _##name##_start
    #define SECTION_END(name) _##name##_end
    #define SECTION_SIZEOF(name) ((size_t)_##name##_end - (size_t)_##name##_start)

#elif defined(_MSC_VER)
    #define SECTION_START(name) __start_##name
    #define SECTION_END(name) __end_##name
    #define SECTION_SIZEOF(name) ((size_t)__end_##name - (size_t)__start_##name)

#else
    #error "Unknown compiler"
#endif

//+********************************* 对外使用宏 **********************************/
/**
 * @brief 定义SECTION_EXTERN宏，用于在外部源文件引用section
 */
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000)
    #define SECTION_EXTERN(name, type)

#elif defined(__ICCARM__) || defined(__ICCRX__)
    #define SECTION_EXTERN(name, type)

#elif defined(__GNUC__)
    #define SECTION_EXTERN(name, type)

#elif defined(_MSC_VER)
    #define SECTION_EXTERN(name, type)                                        \
        extern const size_t      SECTION_START(SECTION_NEW_NAME(name, type)); \
        extern const size_t      SECTION_END(SECTION_NEW_NAME(name, type));   \
        extern const type* const __##SECTION_NEW_NAME(name, type)##_array[];

#else
    #error "Unknown compiler"
#endif

#define SECTION_GET_SIZE(name) (SECTION_SIZEOF(name) / sizeof(__##name##_array[0]))
#define SECTION_GET_ELEM(name, index) (__##name##_array[index])

#if defined(_MSC_VER)
    #define SECTION_REGISTER(name, type, elem)                                                                                      \
        __pragma(data_seg(SECTION_NEW_NAME_STR(name, type))) static const type          __##name##_##elem = elem;                   \
        __declspec(allocate(SECTION_NEW_NAME_STR(name, type))) static const type* const __##name##_ptr_##elem = &__##name##_##elem; \
        __pragma(data_seg())

#else
    // #define SECTION_REGISTER(name, type, elem) \
    //     const type __##name##_##type##_##elem SECTION(SECTION_NEW_NAME_STR(name, type)) = {elem}
#endif

/**

// 期望使用的宏
#define SECTION_EXTERN(name,type)
#define SECTION_REGISTER(name,type,elem)
#define SECTION_GET_ELEM(name,type,index)
#define SECTION_GET_SIZE(name,type)






 */

#endif  //_SECTION_H_
