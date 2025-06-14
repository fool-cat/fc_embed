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
    #include "./fc_stdio/fc_stdio.h"
    #define LOG_VSNPRINTF fc_vsnprintf
#else
    #include <stdio.h>
    #define LOG_VSNPRINTF vsnprintf
#endif  //\ LOG_USE_VSNPRINTF

#ifndef fc_log_assert
    #define fc_log_assert(x) fc_assert(x)
#endif  //\ fc_log_assert

//+*********************************  **********************************/

#if FC_LOG_USE_LOCK
void fc_log_catch_lock(fc_log_t *log, void (*lock)(fc_log_t *log), void (*unlock)(fc_log_t *log))
{
    fc_log_assert(log != NULL);
    log->lock = lock;
    log->unlock = unlock;
}
#endif

void fc_log_set_level(fc_log_t *log, fc_log_level_t level)
{
    fc_log_assert(log != NULL);

    log->level = level;
}

void fc_log_printf(fc_log_t *log, fc_log_level_t level, const char *fmt, ...)
{
    fc_log_assert(log != NULL);
    fc_log_assert(log->buff != NULL);
    fc_log_assert(log->buff_size > 0);

    if (log->level >= level)
    {
        va_list vargs;
        va_start(vargs, fmt);
        int len = 0;

#if FC_LOG_USE_LOCK
        if (log->lock)
        {
            log->lock(log);
        }
#endif

        FC_LOG_ATOMIC
        {
            len = LOG_VSNPRINTF(log->buff, log->buff_size, fmt, vargs);
            len -= log->write(log->buff, len);
        }

#if FC_LOG_USE_LOCK
        if (log->unlock)
        {
            log->unlock(log);
        }
#endif

        FC_LOG_LOSE_HOOK(0 == len, log, log->buff, len);

        va_end(vargs);
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
        va_list vargs;
        va_start(vargs, fmt);
        int len = 0;

        len = LOG_VSNPRINTF(stack_buf, stack_size, fmt, vargs);
        len -= log->write(log->buff, len);

        FC_LOG_LOSE_HOOK(0 == len, log, log->buff, len);

        va_end(vargs);
    }
}

void fc_log_write(fc_log_t *log, fc_log_level_t level, const void *buff, int len)
{
    fc_log_assert(log != NULL);

    if (log->level >= level)
    {
        len -= log->write(buff, len);
        FC_LOG_LOSE_HOOK(0 == len, log, log->buff, len);
    }
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
#include "fc_port.h"
/**
 * @brief 默认log对象的write函数,写入到fc_stdout
 *
 * @param log
 * @param buff
 * @param len
 * @return int
 */
int log_write_stdout(const char *buf, int len)
{
    return fc_port_write(&fc_stdout, buf, len);
}

#include "fc_transport.h"
/**
 * @brief 默认log对象的write函数,写入到fc_transport的0端口
 *
 * @param buf
 * @param len
 * @return int
 */
int log_write_transport(const char *buf, int len)
{
    return fc_sender_write(&fc_sender, 0, buf, len);
}

#if FC_LOG_DEFAULT_CREATE

// 默认实例化对象
FC_LOG_IMPL(default_log);

#endif
