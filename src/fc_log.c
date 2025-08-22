/**
 * @file fc_log.c
 * @author fool-cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-02-06
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>
#include <string.h>

#include "fc_log.h"

#include "fc_port.h"

#ifndef USE_FC_VSNPRINTF
    #define USE_FC_VSNPRINTF 1 /**< 是否使用fc_vsnprintf进行格式化 */
#endif

#if USE_FC_VSNPRINTF
    #include "fc_stdio.h"
    #define LOG_VSNPRINTF fc_vsnprintf
#else
    #include <stdio.h>
    #define LOG_VSNPRINTF vsnprintf
#endif  //\ LOG_USE_VSNPRINTF

#ifndef fc_log_assert
    #define fc_log_assert(x) void(0)
#endif  //\ fc_log_assert

//+*********************************  **********************************/

void fc_log_set_level(fc_log_t *log, fc_log_level_t level)
{
    fc_log_assert(log != NULL);

    log->level = level;
}

void fc_log_printf(fc_log_t *log, fc_log_level_t level, int advice_size, const char *fmt, ...)
{
    fc_log_assert(log != NULL);
    // fc_log_assert(log->alloc != NULL);

    if (log->level >= level)
    {
        int           len = 0;
        fc_log_pool_t pool = {0};
        log->alloc(FC_LOG_ALLOC_NEW, &pool, advice_size);  // 分配内存池

        {
            va_list vargs;
            va_start(vargs, fmt);
            len = LOG_VSNPRINTF(pool.buff, pool.size, fmt, vargs);
            va_end(vargs);
        }

        len -= log->write(pool.buff, len);
        FC_LOG_LOSE_HOOK(0 == len, log, pool.buff, len);
        log->alloc(FC_LOG_ALLOC_FREE, &pool, 0);  // 释放内存池
    }
}

/**
 * @brief 使用者自己去保证write的线程安全性
 *
 * @param log
 * @param level
 * @param stack_buf
 * @param stack_size
 * @param fmt
 * @param ...
 */
void fc_log_printf_stack(fc_log_t *log, fc_log_level_t level, char *stack_buf, int stack_size, const char *fmt, ...)
{
    fc_log_assert(log != NULL);
    fc_log_assert(stack_buf != NULL);
    fc_log_assert(stack_size > 0);

    if (log->level >= level)
    {
        int len = 0;

        {
            va_list vargs;
            va_start(vargs, fmt);
            len = LOG_VSNPRINTF(stack_buf, stack_size, fmt, vargs);
            va_end(vargs);
        }

        len -= log->write(stack_buf, len);
        FC_LOG_LOSE_HOOK(0 == len, log, stack_buf, len);
    }
}

void fc_log_write(fc_log_t *log, fc_log_level_t level, const void *buff, int len)
{
    fc_log_assert(log != NULL);

    if (log->level >= level)
    {
        len -= log->write(buff, len);

        FC_LOG_LOSE_HOOK(0 == len, log, buff, len);
    }
}

//+********************************* 默认自带的内存池分配函数 **********************************/

/**
 * @brief  弱函数建议重写一份,用于调用日志组件的时候分配缓冲区,如果是操作系统可以提供多份固定大小的内存池或者动态内存
 *
 * @param alloc_type
 * @param pool
 * @param advice_size
 * @return int 返回值目前未使用
 */
fc_weak int log_alloc_default(fc_alloc_type_t alloc_type, fc_log_pool_t *pool, int advice_size)
{
    (void)alloc_type;
    (void)advice_size;

    fc_log_assert(pool != NULL);

    static char __buff[FC_LOG_LINE_SIZE] = {0};  // 默认内存池大小

    // 默认为单线程且不考虑中断等情况
    pool->buff = __buff;          // 内存池缓冲区
    pool->size = sizeof(__buff);  // 内存池大小
    return sizeof(__buff);
}

//+********************************* 写入丢失记录 **********************************/

#include "fc_compiler.h"
/**
 * @brief log写入丢失钩子,弱函数,定义了自己的fc_log对象可以重写
 *  记得在宏FC_LOG_LOSE_HOOK里面去开启,默认是关闭了的
 * @param log
 * @param buff
 * @param len
 * @return int 弱函数,可以在外面重写
 */
fc_weak int fc_log_write_lose_hook(fc_log_t *log, const void *buff, int len)
{
    (void)log;
    (void)buff;
    (void)len;

    int count = 0;

    if (log == &default_log)
    {
        static volatile int lose_count = 0;
        lose_count += len;
        count = lose_count;
    }
    else
    {
        fc_log_assert(log != NULL);
    }

    return count;
}

//+********************************* 提供一份默认log对象 **********************************/

// 默认实例化对象
FC_LOG_IMPL(default_log);
