/**
 * @file fc_log.h
 * @author fool-cat (2696652257@qq.com)
 * @brief 参考letter_shell的log组件,log组件核心为行缓冲,[ulog](https://github.com/rdpoor/ulog)
 * 使用的宏API设计为可重入模式,可以重新include此文件进行重定义
 * @version 1.0
 * @date 2025-01-31
 *
 * @copyright Copyright (c) 2025
 *
 */

//> 单次包含宏定义
#ifndef __FC_LOG_H__
    #define __FC_LOG_H__

    #include <stddef.h>
    #include <stdbool.h>
    #include <stdint.h>

    #include "fc_config.h"
    #include "fc_helper.h"

    #ifndef FC_LOG_NOPREFIX_API
        #define FC_LOG_NOPREFIX_API 1 /**< 提供不带(fc_)前缀的log宏API */
    #endif

    #ifndef FC_LOG_ENABLE
        #define FC_LOG_ENABLE 1 /**< 使能log */
    #endif

    #ifndef FC_LOG_LINE_SIZE
        #define FC_LOG_LINE_SIZE 128 /**< log行缓冲大小 */
    #endif

    #ifndef FC_LOG_STACK_LINE_SIZE
        #define FC_LOG_STACK_LINE_SIZE (FC_LOG_LINE_SIZE) /**< log使用栈时在栈上使用的缓冲大小 */
    #endif

    #ifndef FC_LOG_USING_COLOR
        #define FC_LOG_CSI_END ""
        #define FC_LOG_USING_COLOR 1 /**< 是否使用颜色 */
    #endif

    // 宏展开辅助宏
    #undef __FC_LOG_MACRO_EXPANDING
    #define __FC_LOG_MACRO_EXPANDING(...) __VA_ARGS__

    //! 格式需要转义的数量和格式内容的数量/类型必须匹配
    #ifndef FC_LOG_PREFIX_FMT
        #define FC_LOG_PREFIX_FMT "(%d)%s:" /**< 默认输出时间和当前函数名 */
    #endif

    #ifndef FC_LOG_PREFIX_CONTENT
        #define FC_LOG_PREFIX_CONTENT __FC_LOG_MACRO_EXPANDING((uint32_t)666, __FUNCTION__)

    // #include <stdint.h>
    // extern uint32_t HAL_GetTick(void);
    //     #define FC_LOG_PREFIX_CONTENT __FC_LOG_MACRO_EXPANDING((uint32_t)HAL_GetTick(), __FUNCTION__)
    #endif

    #ifndef FC_LOG_END
        #define FC_LOG_END ""
    // #define FC_LOG_END "\r\n"  // 每句log自带换行
    #endif

    // 不同等级的log前缀
    #ifndef FC_LOG_ERROR_HEAD
        #define FC_LOG_ERROR_HEAD "E"
    #endif

    #ifndef FC_LOG_WARNING_HEAD
        #define FC_LOG_WARNING_HEAD "W"
    #endif

    #ifndef FC_LOG_INFO_HEAD
        #define FC_LOG_INFO_HEAD "I"
    #endif

    #ifndef FC_LOG_DEBUG_HEAD
        #define FC_LOG_DEBUG_HEAD "D"
    #endif

    #ifndef FC_LOG_VERBOSE_HEAD
        #define FC_LOG_VERBOSE_HEAD "V"
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
        #define FC_ERROR_TEXT      CSI(31) FC_LOG_ERROR_HEAD    FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 错误标签 */
        #define FC_WARNING_TEXT    CSI(33) FC_LOG_WARNING_HEAD  FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 警告标签 */
        #define FC_INFO_TEXT       CSI(32) FC_LOG_INFO_HEAD     FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 信息标签 */
        #define FC_DEBUG_TEXT      CSI(34) FC_LOG_DEBUG_HEAD    FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 调试标签 */
        #define FC_VERBOSE_TEXT    CSI(36) FC_LOG_VERBOSE_HEAD  FC_LOG_PREFIX_FMT CSI(39) FC_LOG_CSI_END    /**< 冗余信息标签 */
    #else
        #define FC_ERROR_TEXT       FC_LOG_ERROR_HEAD       FC_LOG_PREFIX_FMT
        #define FC_WARNING_TEXT     FC_LOG_WARNING_HEAD     FC_LOG_PREFIX_FMT
        #define FC_INFO_TEXT        FC_LOG_INFO_HEAD        FC_LOG_PREFIX_FMT
        #define FC_DEBUG_TEXT       FC_LOG_DEBUG_HEAD       FC_LOG_PREFIX_FMT
        #define FC_VERBOSE_TEXT     FC_LOG_VERBOSE_HEAD     FC_LOG_PREFIX_FMT
    #endif

    typedef enum
    {
        FC_LOG_NONE     = 0,    /**< 屏蔽所有 */
        FC_LOG_ERROR    = 1,    /**< 错误 */
        FC_LOG_WRANING  = 2,    /**< 警告 */
        FC_LOG_INFO     = 3,    /**< 消息 */
        FC_LOG_DEBUG    = 4,    /**< 调试 */
        FC_LOG_VERBOSE  = 5,    /**< 冗余 */
        FC_LOG_ALL      = 6,    /**< 所有日志 */
    } fc_log_level_t;

// clang-format on

// 存在多次获取,但是释放仅释放一次
typedef enum
{
    FC_LOG_ALLOC_FREE = 0, /**< 释放内存池 */
    FC_LOG_ALLOC_NEW,      /**< 分配新内存池 */
    FC_LOG_ALLOC_REALLOC,  /**< 重新分配内存池 */
} fc_log_alloc_type_t;

//+********************************* 面向对象 **********************************/
typedef struct _fc_log_t      fc_log_t;
typedef struct _fc_log_pool_t fc_log_pool_t;

typedef int (*fc_log_write_t)(void *user, const char *buf, int len);                                   // 写入数据
typedef bool (*fc_log_alloc_t)(fc_log_alloc_type_t alloc_type, fc_log_pool_t *pool, int advice_size);  // log内存池处理函数,返回是否成功

struct _fc_log_pool_t
{
    size_t size; /**< 内存池大小 */
    char  *buff; /**< 内存池 */
    // void *user; /**< 用户自定义数据 */
};

struct _fc_log_t
{
    fc_log_level_t level;

    fc_log_write_t write;
    fc_log_alloc_t alloc;

    // void *user;  // 自定义数据
};

    #ifdef __cplusplus
extern "C"
{
    #endif

    // clang-format off

    extern void fc_log_catch            (fc_log_t *log, fc_log_write_t write, fc_log_alloc_t alloc);
    extern void fc_log_set_level        (fc_log_t *log, fc_log_level_t level);
    extern void fc_log_fprintf          (fc_log_t *log, fc_log_level_t level, const char *fmt, ...);
    // extern void fc_log_write            (fc_log_t *log, fc_log_level_t level, const void *buff, int len); // 废弃

    // clang-format on

    // 提供一份默认的弱函数log写丢失数据钩子,可以在外面重写
    extern int fc_log_write_lose_hook(fc_log_t *log, const void *buff, int len);

    // 提供一份默认的log写函数,可以在外面重写
    extern int log_write_default(void *user, const char *buf, int len);

    // 提供一份默认的内存池获取函数,可以在外面重写
    extern bool log_alloc_default(fc_log_alloc_type_t alloc_type, fc_log_pool_t *pool, int advice_size);

    #ifdef __cplusplus
}
    #endif

//+********************************* 实例化 **********************************/

    #ifndef FC_LOG_OBJ
extern fc_log_t default_log;  // 默认log对象
        #define FC_LOG_OBJ (&default_log)
    #endif

    #define FC_LOG_IMPL(obj_name, _level, _write, _alloc) \
        fc_log_t obj_name = {                             \
            .level = _level, /* 日志级别 */               \
            .write = _write, /* 写入函数 */               \
            .alloc = _alloc, /* 内存池获取函数 */         \
        }

    #ifndef FC_LOG_LOSE_HOOK
        /* #define FC_LOG_LOSE_HOOK(exp, log, buf, len) (void)(0) */
        #define FC_LOG_LOSE_HOOK(exp, log, buf, len)   \
            if (!(exp))                                \
            {                                          \
                fc_log_write_lose_hook(log, buf, len); \
            }
    #endif

    #include "fc_pool.h"

//+********************************* 提供一份默认的实现给log组件 **********************************/
extern fc_pool_t fc_log_pool;  // log组件使用的内存池声明,在fc_log.c中定义

//! 使用fc_pool之后需要在其他地方使用以下方式将内存池中的数据写出去
// void fc_log_walker(bool end, void *ptr, size_t used, void *user)
// {
//     (void)user;
//     (void)end;
//     fc_write(ptr, used);
// }

// if (!fc_pool_fifo_empty(&fc_log_pool))
// {
//     fc_pool_fifo_walk(&fc_log_pool, fc_log_walker, NULL);
// }

#endif  // __FC_LOG_H__

//+********************************* 以下部分允许重入 **********************************/
#undef fc_log_format

#undef fc_log_error
#undef fc_log_warning
#undef fc_log_info
#undef fc_log_debug
#undef fc_log_verbose
#undef fc_log_printf
#undef fc_log_assert

#undef fc_log_level

//+********************************* 宏API **********************************/

// 切换log等级,使用宏API,无需显示指定对象名称
#define fc_log_level(_level)                  \
    do                                        \
    {                                         \
        fc_log_set_level(FC_LOG_OBJ, _level); \
    } while (0)

#if FC_LOG_ENABLE

    #define fc_log_format(text, _level, fmt, ...)                                                                \
        do                                                                                                       \
        {                                                                                                        \
            fc_log_fprintf(FC_LOG_OBJ, _level, text "" fmt "" FC_LOG_END, FC_LOG_PREFIX_CONTENT, ##__VA_ARGS__); \
        } while (0)

    //+********************************* 期望使用 **********************************/
    #define fc_log_error(fmt, ...) \
        fc_log_format(FC_ERROR_TEXT, FC_LOG_ERROR, fmt, ##__VA_ARGS__)

    #define fc_log_warning(fmt, ...) \
        fc_log_format(FC_WARNING_TEXT, FC_LOG_WRANING, fmt, ##__VA_ARGS__)

    #define fc_log_info(fmt, ...) \
        fc_log_format(FC_INFO_TEXT, FC_LOG_INFO, fmt, ##__VA_ARGS__)

    #define fc_log_debug(fmt, ...) \
        fc_log_format(FC_DEBUG_TEXT, FC_LOG_DEBUG, fmt, ##__VA_ARGS__)

    #define fc_log_verbose(fmt, ...) \
        fc_log_format(FC_VERBOSE_TEXT, FC_LOG_VERBOSE, fmt, ##__VA_ARGS__)

    #define fc_log_printf(fmt, ...) \
        fc_log_format("", FC_LOG_NONE, fmt, ##__VA_ARGS__)

    #define fc_log_assert(expr, ...)                                                                   \
        if (!(expr))                                                                                   \
        {                                                                                              \
            fc_log_error("\"" #expr "\" assert failed at file: %s, line: %d\r\n", __FILE__, __LINE__); \
            __VA_ARGS__;                                                                               \
        }

#else

// clang-format off

    #define fc_log_format   (text, level, fmt, ...) do {} while(0)
    #define fc_log_error    (fmt, ...)              do {} while(0)
    #define fc_log_warning  (fmt, ...)              do {} while(0)
    #define fc_log_info     (fmt, ...)              do {} while(0)
    #define fc_log_debug    (fmt, ...)              do {} while(0)
    #define fc_log_verbose  (fmt, ...)              do {} while(0)

    #define fc_log_printf   (fmt, ...)              do {} while(0)
// clang-format on

// 一般在断言中只进行变量比较等操作,通常来说取消断言后效果需要等效完全注释掉
    #define fc_log_assert(expr, ...) \
        if (!(expr))                 \
        {                            \
            (void)0;                 \
            __VA_ARGS__;             \
        }

// #define fc_log_assert(expr, ...) do {} while(0)

#endif

// clang-format off

// 去掉fc_前缀的log宏API
#if FC_LOG_NOPREFIX_API
    #define log_level       fc_log_level
    #define log_error       fc_log_error
    #define log_warning     fc_log_warning
    #define log_info        fc_log_info
    #define log_debug       fc_log_debug
    #define log_verbose     fc_log_verbose

    #define log_printf      fc_log_printf

    #define log_assert      fc_log_assert
#endif

// clang-format on
