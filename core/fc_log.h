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
    #include "fc_pool.h"
    #include "fc_stdio.h"

    #ifndef FC_LOG_ENABLE
        #define FC_LOG_ENABLE 1 /**< 使能log */
    #endif

// clang-format off

    #undef FC_LOG_LEVEL_NONE
    #undef FC_LOG_LEVEL_ERROR
    #undef FC_LOG_LEVEL_WARNING
    #undef FC_LOG_LEVEL_INFO
    #undef FC_LOG_LEVEL_DEBUG
    #undef FC_LOG_LEVEL_VERBOSE
    #undef FC_LOG_LEVEL_ALL

    #define FC_LOG_LEVEL_NONE       0  /**< 屏蔽所有/无视等级 */
    #define FC_LOG_LEVEL_ERROR      1  /**< 错误 */
    #define FC_LOG_LEVEL_WARNING    2  /**< 警告 */
    #define FC_LOG_LEVEL_INFO       3  /**< 消息 */
    #define FC_LOG_LEVEL_DEBUG      4  /**< 调试 */
    #define FC_LOG_LEVEL_VERBOSE    5  /**< 冗余 */
    #define FC_LOG_LEVEL_ALL        6  /**< 所有日志 */

// clang-format on

    #ifndef FC_LOG_FILE_LEVEL
        #define FC_LOG_FILE_LEVEL FC_LOG_LEVEL_ALL /**< 所在文件允许输出log等级 */
    #endif

    #ifndef FC_LOG_NOPREFIX_API
        #define FC_LOG_NOPREFIX_API 1 /**< 提供不带(fc_)前缀的log宏API */
    #endif

    #ifndef FC_LOG_LINE_SIZE
        #define FC_LOG_LINE_SIZE 128 /**< log行缓冲大小 */
    #endif

    #ifndef FC_LOG_USING_COLOR
        #define FC_LOG_USING_COLOR 1 /**< 是否使用颜色 */
    #endif

    #ifndef FC_LOG_FMT_END
        #define FC_LOG_FMT_END "" /**< 每句log格式结尾部分 */
    #endif

    // 宏展开辅助宏
    #undef __FC_LOG_MACRO_EXPANDING
    #define __FC_LOG_MACRO_EXPANDING(...) __VA_ARGS__

    //! 格式需要转义的数量和格式内容的数量/类型必须匹配
    #ifndef FC_LOG_PREFIX_FMT
        #define FC_LOG_PREFIX_FMT ":%s->%d:\t" /**< 默认设置为函数名和行号 */
    #endif

    #ifndef FC_LOG_PREFIX_CONTENT
        #define FC_LOG_PREFIX_CONTENT __FC_LOG_MACRO_EXPANDING(__FUNCTION__, (uint32_t)__LINE__)

    // #include <stdint.h>
    // extern uint32_t HAL_GetTick(void);  //"[时间]函数->行号:"([%d]%s->%d)这种格式的log比较常见
    // #define FC_LOG_PREFIX_CONTENT __FC_LOG_MACRO_EXPANDING(__FUNCTION__, (uint32_t)HAL_GetTick())
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
        #define FC_ERROR_TEXT      CSI(31) FC_LOG_ERROR_HEAD    FC_LOG_PREFIX_FMT CSI(39)   FC_LOG_FMT_END    /**< 错误标签 */
        #define FC_WARNING_TEXT    CSI(33) FC_LOG_WARNING_HEAD  FC_LOG_PREFIX_FMT CSI(39)   FC_LOG_FMT_END    /**< 警告标签 */
        #define FC_INFO_TEXT       CSI(32) FC_LOG_INFO_HEAD     FC_LOG_PREFIX_FMT CSI(39)   FC_LOG_FMT_END    /**< 信息标签 */
        #define FC_DEBUG_TEXT      CSI(34) FC_LOG_DEBUG_HEAD    FC_LOG_PREFIX_FMT CSI(39)   FC_LOG_FMT_END    /**< 调试标签 */
        #define FC_VERBOSE_TEXT    CSI(36) FC_LOG_VERBOSE_HEAD  FC_LOG_PREFIX_FMT CSI(39)   FC_LOG_FMT_END    /**< 冗余信息标签 */
    #else
        #define FC_ERROR_TEXT       FC_LOG_ERROR_HEAD       FC_LOG_PREFIX_FMT   FC_LOG_FMT_END
        #define FC_WARNING_TEXT     FC_LOG_WARNING_HEAD     FC_LOG_PREFIX_FMT   FC_LOG_FMT_END
        #define FC_INFO_TEXT        FC_LOG_INFO_HEAD        FC_LOG_PREFIX_FMT   FC_LOG_FMT_END
        #define FC_DEBUG_TEXT       FC_LOG_DEBUG_HEAD       FC_LOG_PREFIX_FMT   FC_LOG_FMT_END
        #define FC_VERBOSE_TEXT     FC_LOG_VERBOSE_HEAD     FC_LOG_PREFIX_FMT   FC_LOG_FMT_END
    #endif

// clang-format on

// 存在多次获取,但是释放仅释放一次
typedef enum
{
    FC_LOG_ALLOC_FREE = 0, /**< 释放内存池 */
    FC_LOG_ALLOC_NEW,      /**< 分配新内存池 */
    FC_LOG_ALLOC_REALLOC,  /**< 重新分配内存池 */
} fc_log_alloc_type_t;

//+********************************* 面向对象 **********************************/
typedef uint8_t                    fc_log_level_t;  // log等级类型
typedef struct _fc_log_t           fc_log_t;
typedef struct _fc_log_mem_t       fc_log_mem_t;
typedef struct _fc_log_file_user_t fc_log_file_user_t;  // 传递给write函数的用户数据

typedef size_t (*fc_log_write_t)(fc_log_file_user_t *file_user);                                     // 写入数据
typedef bool (*fc_log_alloc_t)(fc_log_alloc_type_t alloc_type, fc_log_mem_t *mem, int advice_size);  // log内存管理函数,返回是否成功

struct _fc_log_mem_t
{
    size_t size; /**< 内存大小 */
    char  *buff; /**< 内存地址 */
    // void *user; /**< 用户自定义数据 */
};

struct _fc_log_file_user_t
{
    fc_log_t    *log;          // 指向根对象,根对象初始化的时候必须赋值为自身
    fc_log_mem_t mem;          // 正在使用的内存块
    size_t       block_write;  // 完整内存块写入的大小
    size_t       total_write;  // 总共写入的大小

    void *mem_chain;  // 内存链,使用自定义的方式保证内存能够链式的管理,这里用到了fc_pool_t来管理,只需要记录第一块内存地址即可
};

struct _fc_log_t
{
    fc_log_alloc_t alloc;
    fc_log_write_t write;

    fc_log_file_user_t file_user;   // 缓冲输出的临时对象中才会使用
    FC_FILE            f;           // 输出对象,同样只在临时对象中才会使用
    fc_log_level_t     last_level;  // 记录上次临时log对象输出等级

    fc_log_level_t level;
    bool           merge; /**< 是否合并日志一并输出 */

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
    extern int  fc_log_fwrite           (fc_log_t *log, fc_log_level_t level, const void *buff, int len);

    // clang-format on

    void fc_log_fflush(fc_log_t *log);  // 输出缓冲区
    // extern size_t fc_log_lose(fc_log_t *log, int len);  // 获取丢失日志长度

    // 提供一份默认的弱函数log写丢失数据钩子,可以在外面重写
    extern size_t fc_log_write_lose_hook(fc_log_t *log, int len);

    // 提供一份默认的log写函数,可以在外面重写
    extern size_t log_write_default(fc_log_file_user_t *file_user);

    // 提供一份默认的内存池获取函数,可以在外面重写
    extern bool log_alloc_default(fc_log_alloc_type_t alloc_type, fc_log_mem_t *mem, int advice_size);

    // 使用log之前需要调用一次默认初始化,已注册到ENV中
    extern void fc_log_init(void);

    #ifdef __cplusplus
}
    #endif

//+********************************* 实例化 **********************************/
extern fc_log_t default_log;  // 默认log对象

    #ifndef FC_LOG_OBJ
        #define FC_LOG_OBJ (default_log)
    #endif

static fc_log_t const *const scope_log_ptr = NULL;  // 强制空指针!!!,强烈建议O1及以上优化可以省非常多空间

    #ifndef FC_LOG_LOSE_HOOK
        /* #define FC_LOG_LOSE_HOOK(exp, log, len) (void)(0) */
        #define FC_LOG_LOSE_HOOK(exp, log, len)   \
            if (!(exp))                           \
            {                                     \
                fc_log_write_lose_hook(log, len); \
            }
    #endif

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
#undef fc_log_level

#undef fc_log_merge_2
#undef fc_log_merge_1
#undef fc_log_merge_0

#undef fc_log_merge
#undef fc_log_format

#undef fc_log_error
#undef fc_log_warning
#undef fc_log_info
#undef fc_log_debug
#undef fc_log_verbose

#undef fc_log_printf
#undef fc_log_printf_lv

#undef fc_log_assert

#undef fc_log_write
#undef fc_log_write_lv

//+********************************* 宏API **********************************/

// 切换log等级,使用宏API,无需显示指定对象名称,注意使用的时候作用域对象
#define fc_log_level(_level)                                                                 \
    do                                                                                       \
    {                                                                                        \
        fc_log_set_level((fc_log_t *)(scope_log_ptr ? scope_log_ptr : &FC_LOG_OBJ), _level); \
    } while (0)

#if FC_LOG_ENABLE

    #define fc_log_merge_2(enter_expr, leave_expr) \
        fc_using(fc_log_t SAFE_NAME(log_obj) = FC_LOG_OBJ, *scope_log_ptr = &SAFE_NAME(log_obj), { SAFE_NAME(log_obj).merge = true; enter_expr; }, {leave_expr; fc_log_fflush(&SAFE_NAME(log_obj)); })

    #define fc_log_merge_1(enter_expr) \
        fc_log_merge_2(enter_expr, { (void)0; })

    #define fc_log_merge_0() \
        fc_log_merge_1({ (void)0; })

    #define fc_log_merge(...) \
        FC_CONNECT2(fc_log_merge_, __PLOOC_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

    #define fc_log_format(text, _level, fmt, ...)                                                                                                                   \
        do                                                                                                                                                          \
        {                                                                                                                                                           \
            if (_level <= FC_LOG_FILE_LEVEL)                                                                                                                        \
                fc_log_fprintf((fc_log_t *)(scope_log_ptr ? scope_log_ptr : &FC_LOG_OBJ), _level, text "" fmt "" FC_LOG_END, FC_LOG_PREFIX_CONTENT, ##__VA_ARGS__); \
        } while (0)

    //+********************************* 期望使用 **********************************/
    #if FC_LOG_FILE_LEVEL >= FC_LOG_LEVEL_ERROR
        #define fc_log_error(fmt, ...) \
            fc_log_format(FC_ERROR_TEXT, FC_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
    #else
        #define fc_log_error(fmt, ...)                                                                       \
            do                                                                                               \
            {                                                                                                \
                if (scope_log_ptr)                                                                           \
                {                                                                                            \
                    fc_log_t *SAFE_NAME(log_temp_ptr) = (fc_log_t *)scope_log_ptr;                           \
                    SAFE_NAME(log_temp_ptr)->last_level = FC_LOG_LEVEL_ERROR; /* 临时对象实体记录临时等级 */ \
                }                                                                                            \
            } while (0);
    #endif

    #if FC_LOG_FILE_LEVEL >= FC_LOG_LEVEL_WARNING
        #define fc_log_warning(fmt, ...) \
            fc_log_format(FC_WARNING_TEXT, FC_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
    #else
        #define fc_log_warning(fmt, ...)                                                                       \
            do                                                                                                 \
            {                                                                                                  \
                if (scope_log_ptr)                                                                             \
                {                                                                                              \
                    fc_log_t *SAFE_NAME(log_temp_ptr) = (fc_log_t *)scope_log_ptr;                             \
                    SAFE_NAME(log_temp_ptr)->last_level = FC_LOG_LEVEL_WARNING; /* 临时对象实体记录临时等级 */ \
                }                                                                                              \
            } while (0);
    #endif

    #if FC_LOG_FILE_LEVEL >= FC_LOG_LEVEL_INFO
        #define fc_log_info(fmt, ...) \
            fc_log_format(FC_INFO_TEXT, FC_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
    #else
        #define fc_log_info(fmt, ...)                                                                       \
            do                                                                                              \
            {                                                                                               \
                if (scope_log_ptr)                                                                          \
                {                                                                                           \
                    fc_log_t *SAFE_NAME(log_temp_ptr) = (fc_log_t *)scope_log_ptr;                          \
                    SAFE_NAME(log_temp_ptr)->last_level = FC_LOG_LEVEL_INFO; /* 临时对象实体记录临时等级 */ \
                }                                                                                           \
            } while (0);
    #endif

    #if FC_LOG_FILE_LEVEL >= FC_LOG_LEVEL_DEBUG
        #define fc_log_debug(fmt, ...) \
            fc_log_format(FC_DEBUG_TEXT, FC_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
    #else
        #define fc_log_debug(fmt, ...)                                                                       \
            do                                                                                               \
            {                                                                                                \
                if (scope_log_ptr)                                                                           \
                {                                                                                            \
                    fc_log_t *SAFE_NAME(log_temp_ptr) = (fc_log_t *)scope_log_ptr;                           \
                    SAFE_NAME(log_temp_ptr)->last_level = FC_LOG_LEVEL_DEBUG; /* 临时对象实体记录临时等级 */ \
                }                                                                                            \
            } while (0);
    #endif

    #if FC_LOG_FILE_LEVEL >= FC_LOG_LEVEL_VERBOSE
        #define fc_log_verbose(fmt, ...) \
            fc_log_format(FC_VERBOSE_TEXT, FC_LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
    #else
        #define fc_log_verbose(fmt, ...)                                                                       \
            do                                                                                                 \
            {                                                                                                  \
                if (scope_log_ptr)                                                                             \
                {                                                                                              \
                    fc_log_t *SAFE_NAME(log_temp_ptr) = (fc_log_t *)scope_log_ptr;                             \
                    SAFE_NAME(log_temp_ptr)->last_level = FC_LOG_LEVEL_VERBOSE; /* 临时对象实体记录临时等级 */ \
                }                                                                                              \
            } while (0);
    #endif

    #define fc_log_printf(fmt, ...)                                                                        \
        do                                                                                                 \
        {                                                                                                  \
            fc_log_t *SAFE_NAME(log_temp_ptr) = (fc_log_t *)(scope_log_ptr ? scope_log_ptr : &FC_LOG_OBJ); \
            if (SAFE_NAME(log_temp_ptr)->last_level <= FC_LOG_FILE_LEVEL)                                  \
                fc_log_format("", SAFE_NAME(log_temp_ptr)->last_level, fmt, ##__VA_ARGS__);                \
        } while (0)

    #define fc_log_printf_lv(_level, fmt, ...)                 \
        do                                                     \
        {                                                      \
            if (_level <= FC_LOG_FILE_LEVEL)                   \
                fc_log_format("", _level, fmt, ##__VA_ARGS__); \
        } while (0)

    #define fc_log_assert(expr, ...)                                                                   \
        if (!(expr))                                                                                   \
        {                                                                                              \
            fc_log_error("\"" #expr "\" assert failed at file: %s, line: %d\r\n", __FILE__, __LINE__); \
            __VA_ARGS__;                                                                               \
        }

    /**
     * @brief fc_log_write如果是全局的则强制以最高等级输出,如果是fc_log_merge作用域内,则上一条log什么等级,接下来的write就以什么等级输出
     *
     */
    #define fc_log_write(buf, len)                                                                         \
        do                                                                                                 \
        {                                                                                                  \
            fc_log_t *SAFE_NAME(log_temp_ptr) = (fc_log_t *)(scope_log_ptr ? scope_log_ptr : &FC_LOG_OBJ); \
            fc_log_fwrite(SAFE_NAME(log_temp_ptr), SAFE_NAME(log_temp_ptr)->last_level, buf, len);         \
        } while (0)

    #define fc_log_write_lv(_level, buf, len)                                                               \
        do                                                                                                  \
        {                                                                                                   \
            if (_level <= FC_LOG_FILE_LEVEL)                                                                \
                fc_log_fwrite((fc_log_t *)(scope_log_ptr ? scope_log_ptr : &FC_LOG_OBJ), _level, buf, len); \
        } while (0)

#else
    #define fc_log_merge_2(enter_expr, leave_expr)
    #define fc_log_merge_1(enter_expr)
    #define fc_log_merge_0()

    #define fc_log_merge(...)  // 空定义即可

// clang-format off

    #define fc_log_format(text, level, fmt, ...)    ((void)0)
    #define fc_log_error(fmt, ...)                  ((void)0)
    #define fc_log_warning(fmt, ...)                ((void)0)
    #define fc_log_info(fmt, ...)                   ((void)0)
    #define fc_log_debug(fmt, ...)                  ((void)0)
    #define fc_log_verbose(fmt, ...)                ((void)0) 

    #define fc_log_printf(fmt, ...)                 ((void)0)
    #define fc_log_printf_lv(_level, fmt, ...)      ((void)0)

    #define fc_log_write(buf, len)                  ((void)0)
    #define fc_log_write_lv(_level, buf, len)       ((void)0)
// clang-format on

// 一般在断言中只进行变量比较等操作,通常来说取消断言后效果需要等效完全注释掉
    #define fc_log_assert(expr, ...) \
        if (!(expr))                 \
        {                            \
            (void)0;                 \
            __VA_ARGS__;             \
        }

// #define fc_log_assert(expr, ...)     ((void)0)

#endif

// clang-format off

// 去掉fc_前缀的log宏API
#if FC_LOG_NOPREFIX_API
    #define log_level       fc_log_level

    #define log_merge       fc_log_merge

    #define log_error       fc_log_error
    #define log_warning     fc_log_warning
    #define log_info        fc_log_info
    #define log_debug       fc_log_debug
    #define log_verbose     fc_log_verbose

    #define log_printf      fc_log_printf
    #define log_printf_lv   fc_log_printf_lv

    #define log_assert      fc_log_assert

    #define log_write       fc_log_write
    #define log_write_lv    fc_log_write_lv
#endif

// clang-format on

// 除了 FC_LOG_FMT_END 定义结尾外,还可以使用如下宏在每句log后面添加特定内容如换行
// #define user_printf(fmt, ...) fc_log_printf(fmt "\r\n", ##__VA_ARGS__)
// #define info_printf(fmt, ...) fc_log_info(fmt "\r\n", ##__VA_ARGS__)
