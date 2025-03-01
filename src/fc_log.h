/**
 * @file fc_log.h
 * @author fool-cat (2696652257@qq.com)
 * @brief 参考letter_shell的log组件,log组件核心为行缓冲
 * @version 1.0
 * @date 2025-01-31
 *
 * @copyright Copyright (c) 2025
 *
 */

//> 单次包含宏定义
#ifndef __FC_LOG_H__
#define __FC_LOG_H__

// overlay的方式覆盖默认配置
#ifdef FC_CONFIG_HEADER
    #include FC_CONFIG_HEADER
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "fc_helper.h"

#ifndef FC_LOG_ENABLE
    #define FC_LOG_ENABLE 1 /**< 使能log */
#endif

#ifndef FC_LOG_LINE_SIZE
    #define FC_LOG_LINE_SIZE 256 /**< log输出缓冲大小 */
#endif

#ifndef FC_LOG_STACK_LINE_SIZE
    #define FC_LOG_STACK_LINE_SIZE (FC_LOG_LINE_SIZE) /**< log自身缓存被占用时在栈上使用的缓冲大小 */
#endif

#ifndef FC_LOG_USING_COLOR
    #define FC_LOG_CSI_END ""
    #define FC_LOG_USING_COLOR 1 /**< 是否使用颜色 */
#endif

#undef __MACRO_EXPANDING
#define __MACRO_EXPANDING(...) __VA_ARGS__

//! 格式需要转义的数量和格式内容的数量/类型必须匹配
#ifndef FC_LOG_PREFIX_FMT
    #define FC_LOG_PREFIX_FMT "(%d)%s" /**< 默认输出时间和当前函数名 */
#endif

#ifndef FC_LOG_PREFIX_CONTENT
    #define FC_LOG_PREFIX_CONTENT __MACRO_EXPANDING(HAL_GetTick(), __FUNCTION__)
#endif

#ifndef FC_LOG_FMT_END
    #define FC_LOG_FMT_END ":"
#endif

// clang-format off

/**
 * 终端字体颜色代码
 */
#define     CSI_BLACK           30              /**< 黑色 */
#define     CSI_RED             31              /**< 红色 */
#define     CSI_GREEN           32              /**< 绿色 */
#define     CSI_YELLOW          33              /**< 黄色 */
#define     CSI_BLUE            34              /**< 蓝色 */
#define     CSI_FUCHSIN         35              /**< 品红 */
#define     CSI_CYAN            36              /**< 青色 */
#define     CSI_WHITE           37              /**< 白色 */
#define     CSI_BLACK_L         90              /**< 亮黑 */
#define     CSI_RED_L           91              /**< 亮红 */
#define     CSI_GREEN_L         92              /**< 亮绿 */
#define     CSI_YELLOW_L        93              /**< 亮黄 */
#define     CSI_BLUE_L          94              /**< 亮蓝 */
#define     CSI_FUCHSIN_L       95              /**< 亮品红 */
#define     CSI_CYAN_L          96              /**< 亮青 */
#define     CSI_WHITE_L         97              /**< 亮白 */
#define     CSI_DEFAULT         39              /**< 默认 */

#define     CSI(code)           "\033[" #code "m"   /**< ANSI CSI指令 */
#define     CSI_RST             "\033[0m"           /**< ANSI CSI指令重置 */

/**
 * log级别字符(包含颜色)
 */
#if FC_LOG_USING_COLOR == 1
    #define ERROR_TEXT      CSI(31) "E" FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 错误标签 */
    #define WARNING_TEXT    CSI(33) "W" FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 警告标签 */
    #define INFO_TEXT       CSI(32) "I" FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 信息标签 */
    #define DEBUG_TEXT      CSI(34) "D" FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 调试标签 */
    #define VERBOSE_TEXT    CSI(36) "V" FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 冗余信息标签 */
#else
    #define ERROR_TEXT      "E" FC_LOG_PREFIX_FMT
    #define WARNING_TEXT    "W" FC_LOG_PREFIX_FMT
    #define INFO_TEXT       "I" FC_LOG_PREFIX_FMT
    #define DEBUG_TEXT      "D" FC_LOG_PREFIX_FMT
    #define VERBOSE_TEXT    "V" FC_LOG_PREFIX_FMT
#endif

// clang-format on

typedef enum
{
    FC_LOG_NONE = 0,    /**< 屏蔽所有 */
    FC_LOG_ERROR = 1,   /**< 错误 */
    FC_LOG_WRANING = 2, /**< 警告 */
    FC_LOG_INFO = 3,    /**< 消息 */
    FC_LOG_DEBUG = 4,   /**< 调试 */
    FC_LOG_VERBOSE = 5, /**< 冗余 */
    FC_LOG_ALL = 6,     /**< 所有日志 */
} fc_log_level_t;

//> C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif
    //+********************************* 面向对象 **********************************/
    typedef struct _fc_log_t fc_log_t;
    struct _fc_log_t
    {
        fc_log_level_t level;
        bool           active;
        bool           buff_busy;               // 自身的缓冲区是否被占用
        char           buff[FC_LOG_LINE_SIZE];  // 行缓冲,每个log独立拥有自己的行缓冲
        // 每个log拥有自己独立的行缓冲是为了提高性能,避免频繁栈内存创建和销毁

        void* user;  // 预留用户个人数据

        size_t (*write)(fc_log_t* log, const void* buff, size_t len);  // 写入数据
    };

    extern void fc_log_set_active(fc_log_t* log, bool active);
    extern void fc_log_set_level(fc_log_t* log, fc_log_level_t level);
    extern void fc_log_printf(fc_log_t* log, fc_log_level_t level, const char* fmt, ...);
    extern void fc_log_write(fc_log_t* log, fc_log_level_t level, const void* buff, size_t len);

    //+********************************* 实例化 **********************************/

    extern fc_log_t default_log;  // 默认log对象

#define fc_log_write_catch(write_func) \
    do                                 \
    {                                  \
        default_log.write = write_func \
    } while (0)

#ifndef FC_LOG_OBJ
    #define FC_LOG_OBJ ((fc_log_t*)&default_log)
#endif

#ifndef FC_LOG_LOSE_HOOK
    extern size_t fc_log_write_lose_hook(fc_log_t* log, const void* buff, size_t len);
    #if 0
        #define FC_LOG_LOSE_HOOK(exp, log, buf, len)       \
            if (!(exp))                                    \
            {                                              \
                do                                         \
                {                                          \
                    fc_log_write_lose_hook(log, buf, len); \
                } while (0);                               \
            }
    #else
        #define FC_LOG_LOSE_HOOK(exp, log, buf, len) (void)(0)
    #endif
#endif

    //+********************************* 宏API **********************************/

#if FC_LOG_ENABLE

    #define log_format(text, level, fmt, ...)                                                    \
        do                                                                                       \
        {                                                                                        \
            fc_log_printf(FC_LOG_OBJ, level, text "" fmt, FC_LOG_PREFIX_CONTENT, ##__VA_ARGS__); \
        } while (0)

    //+********************************* 期望使用 **********************************/
    #define log_error(fmt, ...) \
        log_format(ERROR_TEXT, FC_LOG_ERROR, fmt, ##__VA_ARGS__)

    #define log_warning(fmt, ...) \
        log_format(WARNING_TEXT, FC_LOG_WRANING, fmt, ##__VA_ARGS__)

    #define log_info(fmt, ...) \
        log_format(INFO_TEXT, FC_LOG_INFO, fmt, ##__VA_ARGS__)

    #define log_debug(fmt, ...) \
        log_format(DEBUG_TEXT, FC_LOG_DEBUG, fmt, ##__VA_ARGS__)

    #define log_verbose(fmt, ...) \
        log_format(VERBOSE_TEXT, FC_LOG_VERBOSE, fmt, ##__VA_ARGS__)

    #define log_assert(expr, ...)                                                                   \
        if (!(expr))                                                                                \
        {                                                                                           \
            log_error("\"" #expr "\" assert failed at file: %s, line: %d\r\n", __FILE__, __LINE__); \
            __VA_ARGS__;                                                                            \
        }

#else

    #define log_format(text, level, fmt, ...) (void)(0)
    #define log_error(fmt, ...) (void)(0)
    #define log_warning(fmt, ...) (void)(0)
    #define log_info(fmt, ...) (void)(0)
    #define log_debug(fmt, ...) (void)(0)
    #define log_verbose(fmt, ...) (void)(0)
    #define log_assert(expr, ...) (void)(0)

#endif

#ifdef __cplusplus
}
#endif

#endif  // __FC_LOG_H__
