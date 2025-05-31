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

    // overlay的方式覆盖默认配置
    #ifdef FC_CONFIG_HEADER
        #if defined(FC_USE_STRINGFY)
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

    #include "fc_helper.h"

    #ifndef FC_LOG_ENABLE
        #define FC_LOG_ENABLE 1 /**< 使能log */
    #endif

    #ifndef FC_LOG_DEFAULT_CREATE
        #define FC_LOG_DEFAULT_CREATE 1 /**< 是否创建默认的log对象,如果不创建则需要自己创建一个 */
    #endif

    // 需要支持嵌套
    #ifndef FC_LOG_ATOMIC
        #define FC_LOG_ATOMIC
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

    //! 格式需要转义的数量和格式内容的数量/类型必须匹配
    #ifndef FC_LOG_PREFIX_FMT
        #define FC_LOG_PREFIX_FMT "(%d)%s:" /**< 默认输出时间和当前函数名 */
    #endif

    #ifndef FC_LOG_PREFIX_CONTENT
        #undef __MACRO_EXPANDING
        #define __MACRO_EXPANDING(...) __VA_ARGS__
        #define FC_LOG_PREFIX_CONTENT __MACRO_EXPANDING(666, __FUNCTION__)

    // #include <stdint.h>
    // extern uint32_t HAL_GetTick(void);
    //     #define FC_LOG_PREFIX_CONTENT __MACRO_EXPANDING(HAL_GetTick(), __FUNCTION__)
    #endif

    #ifndef FC_LOG_END
        #define FC_LOG_END ""
    // #define FC_LOG_END "\r\n"  // 每句log自带换行
    #endif

    #define FC_LOG_MEM_SELF 0   // 使用自身的行缓冲区,线程不安全
    #define FC_LOG_MEM_STACK 1  // 使用栈缓冲区,线程安全-适用于中断等场景

    #ifndef FC_LOG_MEM_TYPE
        #define FC_LOG_MEM_TYPE FC_LOG_MEM_SELF /**< log缓冲区类型,默认是自己分配内存 */
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

        int buff_size;  // 行缓冲区大小
        // 每个log拥有自己独立的行缓冲是为了提高性能,避免频繁栈内存创建和销毁
        char *buff;                              // 行缓冲区
        int (*write)(const char *buf, int len);  // 写入数据

        // void* user;  // 预留用户个人数据
    };

    extern void fc_log_set_level(fc_log_t *log, fc_log_level_t level);
    extern void fc_log_printf(fc_log_t *log, fc_log_level_t level, const char *fmt, ...);
    extern void fc_log_printf_stack(fc_log_t *log, fc_log_level_t level, char *stack_buf, int stack_size, const char *fmt, ...);
    extern void fc_log_write(fc_log_t *log, fc_log_level_t level, const void *buff, int len);  // 未使用

    // 提供一份默认的弱函数log写丢失数据钩子,可以在外面重写
    extern int fc_log_write_lose_hook(fc_log_t *log, const void *buff, int len);

    // 提供两个默认的write函数,一个是输出到fc_stdout,一个是输出到fc_transport的0端口
    extern int log_write_stdout(const char *buf, int len);
    extern int log_write_transport(const char *buf, int len);

    //+********************************* 实例化 **********************************/

    #ifndef FC_LOG_OBJ
        #if FC_LOG_DEFAULT_CREATE != 1
            #error "不创建默认log对象情况下,必须提供自定义的fc_log_t对象,并定义FC_LOG_OBJ宏"
        #endif
    extern fc_log_t default_log;  // 默认log对象
        #define FC_LOG_OBJ (&default_log)
    #endif

    #ifndef FC_LOG_DEFAULT_WRITE
        #define FC_LOG_DEFAULT_WRITE log_write_stdout
    #endif

    #define FC_LOG_IMPL_FULL(obj_name, _level, _func, _size)        \
        static char SAFE_NAME(buff)[_size]; /* 缓冲区内存 */        \
        fc_log_t    obj_name = {                                    \
               .level = _level,                  /* 日志级别 */     \
               .buff_size = _size,               /* 行缓冲区大小 */ \
               .buff = (char *)&SAFE_NAME(buff), /* 行缓冲区 */     \
               .write = _func,                   /* 写入函数 */     \
        }

    #define _FC_LOG_IMPL_FULL_1(obj_name) \
        FC_LOG_IMPL_FULL(obj_name, FC_LOG_ALL, FC_LOG_DEFAULT_WRITE, FC_LOG_LINE_SIZE)

    #define _FC_LOG_IMPL_FULL_2(obj_name, _level) \
        FC_LOG_IMPL_FULL(obj_name, FC_LOG_ALL, _func, FC_LOG_LINE_SIZE)

    #define _FC_LOG_IMPL_FULL_3(obj_name, _level, _func) \
        FC_LOG_IMPL_FULL(obj_name, _level, _func, FC_LOG_LINE_SIZE)

    #define _FC_LOG_IMPL_FULL_4(obj_name, _level, _func, _size) \
        FC_LOG_IMPL_FULL(obj_name, _level, _func, _size)

    /**
     * 这个宏用于快速定义一个fc_log_t对象,禁止在头文件中使用,否则会导致多重定义
     * @brief 实例化log对象,至少一个参数,最大支持4个参数
     *参数必须按照指定顺序给出,允许缺省(从后面开始缺省)
     *<1>对象名称
     *<2>写入函数
     *<3>日志级别
     *<4>行缓冲区大小
     */
    #define FC_LOG_IMPL(...) \
        CONNECT2(_FC_LOG_IMPL_FULL_, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

    #ifndef FC_LOG_LOSE_HOOK
        #if 1
            #define FC_LOG_LOSE_HOOK(exp, log, buf, len)   \
                if (!(exp))                                \
                {                                          \
                    fc_log_write_lose_hook(log, buf, len); \
                }
        #else
            #define FC_LOG_LOSE_HOOK(exp, log, buf, len) (void)(0)
        #endif
    #endif

    #ifdef __cplusplus
}
    #endif

#endif  // __FC_LOG_H__

//+********************************* 以下部分允许重入 **********************************/
#undef log_format

#undef log_error
#undef log_warning
#undef log_info
#undef log_debug
#undef log_verbose
#undef log_assert

#undef fc_log_level
#undef fc_log_switch

//+********************************* 宏API **********************************/

// 切换log等级,使用宏API,无需显示指定对象名称
#define fc_log_level(_level)                  \
    do                                        \
    {                                         \
        fc_log_set_level(FC_LOG_OBJ, _level); \
    } while (0)

// 切换log等级,使用宏API,无需显示指定对象名称
#define fc_log_switch(_level)                 \
    do                                        \
    {                                         \
        fc_log_set_level(FC_LOG_OBJ, _level); \
    } while (0)

#if FC_LOG_ENABLE

    #if (FC_LOG_MEM_TYPE == FC_LOG_MEM_STACK)

        #if (FC_LOG_STACK_LINE_SIZE < 0)
            #error "FC_LOG_STACK_LINE_SIZE must be greater than 0"
            #error "FC_LOG_STACK_LINE_SIZE 必须大于0"
        #endif

        #define log_format(text, level, fmt, ...)                                                                                                                 \
            do                                                                                                                                                    \
            {                                                                                                                                                     \
                char SAFE_NAME(buff)[FC_LOG_STACK_LINE_SIZE]; /* 使用栈内存 */                                                                                    \
                fc_log_printf_stack(FC_LOG_OBJ, level, SAFE_NAME(buff), FC_LOG_STACK_LINE_SIZE, text "" fmt "" FC_LOG_END, FC_LOG_PREFIX_CONTENT, ##__VA_ARGS__); \
            } while (0)

    #elif (FC_LOG_MEM_TYPE == FC_LOG_MEM_SELF)

        #define log_format(text, level, fmt, ...)                                                                  \
            do                                                                                                     \
            {                                                                                                      \
                fc_log_printf(FC_LOG_OBJ, level, text "" fmt "" FC_LOG_END, FC_LOG_PREFIX_CONTENT, ##__VA_ARGS__); \
            } while (0)

    #else
        #error "FC_LOG_MEM_TYPE unkown"
        #error "FC_LOG_MEM_TYPE 未知,请检查配置"
    // 后续可以提供直接输出到fc_stdout的方式,不使用自身或者栈的缓冲区

    #endif

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

    #ifndef log_assert
        #define log_assert(expr, ...)                                                                   \
            if (!(expr))                                                                                \
            {                                                                                           \
                log_error("\"" #expr "\" assert failed at file: %s, line: %d\r\n", __FILE__, __LINE__); \
                __VA_ARGS__;                                                                            \
            }
    #endif

#else

// clang-format off

    #define log_format(text, level, fmt, ...) do {} while(0)
    #define log_error(fmt, ...) do {} while(0)
    #define log_warning(fmt, ...) do {} while(0)
    #define log_info(fmt, ...) do {} while(0)
    #define log_debug(fmt, ...) do {} while(0)
    #define log_verbose(fmt, ...) do {} while(0)

// clang-format on

    #ifndef log_assert
        #define log_assert(expr, ...) \
            if (!(expr))              \
            {                         \
                (void)0;              \
                __VA_ARGS__;          \
            }
    #endif

#endif
