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

#include "fc_log.h"

#include "fc_port.h"

#ifndef FC_ASSERT
    #define FC_ASSERT(x) ((void)0)
#endif  //\ FC_ASSERT

static size_t _log_buff_write(fc_log_t* log, const void* buff, size_t len)
{
    FC_ASSERT(log != NULL);

    return fc_write(buff, len);
}

fc_log_t default_log = {
    .level = FC_LOG_ALL,
    .active = true,
    .buff = {0},
    .user = NULL,
    .write = _log_buff_write,
};

void fc_log_set_active(fc_log_t* log, bool active)
{
    FC_ASSERT(log != NULL);

    log->active = active;
}

void fc_log_set_level(fc_log_t* log, fc_log_level_t level)
{
    FC_ASSERT(log != NULL);

    log->level = level;
}

extern int fc_vsnprintf(char* s, size_t n, const char* fmt, va_list ap);

void fc_log_printf(fc_log_t* log, fc_log_level_t level, const char* fmt, ...)
{
    FC_ASSERT(log != NULL);

    if (log->active && log->level >= level)
    {
        va_list vargs;
        va_start(vargs, fmt);

        if (log->buff_busy)
        {
            log->buff_busy = true;

            int len = fc_vsnprintf(log->buff, FC_LOG_LINE_SIZE, fmt, vargs);
            len -= log->write(log, log->buff, len);
            FC_LOG_LOSE_HOOK(0 == len, log, log->buff, len);

            log->buff_busy = false;
        }
        else
        {
            char buff[FC_LOG_STACK_LINE_SIZE];
            int  len = fc_vsnprintf(buff, FC_LOG_STACK_LINE_SIZE, fmt, vargs);
            len -= log->write(log, buff, len);
            FC_LOG_LOSE_HOOK(0 == len, log, buff, len);
        }

        va_end(vargs);
    }
}

void fc_log_write(fc_log_t* log, fc_log_level_t level, const void* buff, size_t len)
{
    FC_ASSERT(log != NULL);

    if (log->active && log->level >= level)
    {
        len -= log->write(log, buff, len);
        FC_LOG_LOSE_HOOK(0 == len, log, log->buff, len);
    }
}

#include "fc_compiler.h"
fc_weak size_t fc_log_write_lose_hook(fc_log_t* log, const void* buff, size_t len)
{
    (void)log;
    (void)buff;
    (void)len;

    size_t count = 0;

    if (log == &default_log)
    {
        static volatile size_t lose_count = 0;
        lose_count += len;
        count = lose_count;
    }
    else
    {
        FC_ASSERT(log != NULL);
    }

    return count;
}
