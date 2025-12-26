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

#include "fc_compiler.h"
#include "fc_config.h"
#include "fc_stdio.h"
#include "fc_log.h"
#include "fc_port.h"

#ifndef fc_assert
    #define fc_assert(x) ((void)(0))
#endif  //\ fc_assert

//+*********************************  **********************************/
/**
 * @brief 绑定日志组件的写函数和内存池分配函数
 *
 * @param log
 * @param write
 * @param alloc
 */
void fc_log_catch(fc_log_t *log, fc_log_write_t write, fc_log_alloc_t alloc)
{
    fc_assert(log != NULL);
    fc_assert(write != NULL);
    fc_assert(alloc != NULL);

    log->write = write;
    log->alloc = alloc;
}

/**
 * @brief 设置日志等级
 *
 * @param log
 * @param level
 */
void fc_log_set_level(fc_log_t *log, fc_log_level_t level)
{
    fc_assert(log != NULL);

    log->level = level;
}

// 传递给write函数的用户数据
typedef struct _fc_log_file_user_t fc_log_file_user_t;
struct _fc_log_file_user_t
{
    fc_log_t      *log;         // 关联的log对象
    fc_log_pool_t *pool;        // 第一块内存池
    void          *start_pool;  // 记录第一块内存地址
    size_t         write_size;  // 已写入的大小
};

/**
 * @brief 具体查看vfprintf.c中_write_ch函数
 *
 *
 * @param buf
 * @param len 每次只会写入1字节,结尾会调用一次0长度的写入
 * @return int
 */
static int __fc_log_alloc_write(FC_FILE *f, const void *buf, int len)
{
    // 只会在需要分配内存(FC_IO_SWAP == 0)和结束(FC_IO_EOF == -1)的时候调用
    if (len >= FC_IO_SWAP)
    {
        fc_log_file_user_t *user = (fc_log_file_user_t *)f->user;

        fc_log_t      *log = user->log;
        fc_log_pool_t *pool = user->pool;
        user->write_size += pool->size;
        if (false == log->alloc(FC_LOG_ALLOC_NEW, pool, FC_LOG_LINE_SIZE))
        {
            // 结束序列化
            return -1;  // 这里丢失由内存池分配函数alloc记录
        }

        // 切换到新的内存块
        f->p_now = pool->buff;
        f->p_start = pool->buff;
        f->p_end = (char *)((uint8_t *)(pool->buff) + pool->size);
    }

    return len;
}

/**
 * @brief
 *
 * @param log
 * @param level
 * @param fmt
 * @param ...
 */
fc_weak void fc_log_fprintf(fc_log_t *log, fc_log_level_t level, const char *fmt, ...)
{
    fc_assert(log != NULL);
    fc_assert(log->alloc != NULL);

    if (log->level >= level)
    {
        fc_log_pool_t pool = {0};
        int           len = 0;

        // 分配内存池
        if (false == log->alloc(FC_LOG_ALLOC_NEW, &pool, FC_LOG_LINE_SIZE))
        {
#if FC_LOG_ENABLE_ALLOC_FAIL_HANDLE
            va_list vargs;
            va_start(vargs, fmt);
            len = fc_vsnprintf(NULL, 0, fmt, vargs);
            va_end(vargs);

            // 调用钩子记录丢失的日志长度
            FC_LOG_LOSE_HOOK(0 == len, log, len);
#else
            (void)fmt;  // 避免未使用参数警告
#endif
            return;
        }

        fc_log_file_user_t user = {
            .log = log,
            .pool = &pool,
            .start_pool = pool.buff,
            .write_size = 0};

        // 直接在栈上构造FC_FILE，避免额外的初始化开销
        FC_FILE f = {
            .p_now = pool.buff,
            .p_start = pool.buff,
            .p_end = pool.buff + pool.size,
            .user = (void *)&user,
            .io.write = __fc_log_alloc_write};

        va_list vargs;
        va_start(vargs, fmt);
        len = fc_vfprintf(&f, fmt, vargs);
        va_end(vargs);

        // 写入数据并处理丢失
        len -= log->write((void *)&user, user.start_pool, len);
        FC_LOG_LOSE_HOOK(0 == len, log, len);

        // 释放内存池
        log->alloc(FC_LOG_ALLOC_FREE, &pool, 0);
    }
}

#if 0
// 废弃
/**
 * @brief
 *
 * @param log
 * @param level
 * @param buff
 * @param len
 */
void fc_log_write(fc_log_t *log, fc_log_level_t level, const void *buff, int len)
{
    fc_assert(log != NULL);

    if (log->level >= level)
    {
        len -= log->write(buff, len);

        FC_LOG_LOSE_HOOK(0 == len, log, len);
    }
}
#endif

//+********************************* 写入丢失记录 **********************************/

/**
 * @brief log写入丢失钩子,弱函数,定义了自己的fc_log对象可以重写
 *  记得在宏FC_LOG_LOSE_HOOK里面去开启,默认是关闭了的
 * @param log
 * @param len buff为NULL时表示内存池分配失败截断丢弃的长度
 * @return int 弱函数,可以在外面重写
 */
fc_weak int fc_log_write_lose_hook(fc_log_t *log, int len)
{
    (void)log;
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
        fc_assert(log != NULL);
    }

    return count;
}

//+********************************* 提供一份默认log对象 **********************************/

// 默认实例化对象
FC_LOG_IMPL(default_log, FC_LOG_ALL, log_write_default, log_alloc_default);

//+********************************* 使用fc_pool进行管理 **********************************/

#include "fc_auto_init.h"
#include "fc_pool.h"

fc_pool_t fc_log_pool;  // log组件使用的内存池

// log组件可使用的内存池大小(字节),建议向上取整到sizeof(size_t)
#ifndef FC_LOG_POOL_TOTAL_SIZE
    #define FC_LOG_POOL_TOTAL_SIZE (4 * 1024) /* 默认4KB */
#endif

// 每块pool用户可使用大小(字节),内部会向上取整到sizeof(size_t)
#ifndef FC_LOG_ALLOC_BLOCK_SIZE
    #define FC_LOG_ALLOC_BLOCK_SIZE FC_LOG_LINE_SIZE /* 128的话满足大多数情况下日志需求,减少内存分配次数 */
#endif

void fc_log_pool_init(void)
{
    // static size_t log_pool_mem[FC_LOG_POOL_TOTAL_SIZE / sizeof(size_t)];  // 内存池
    // static size_t log_pool_mem[FC_CALC_POOL_MEM_SIZE(FC_LOG_ALLOC_BLOCK_SIZE, 64) / sizeof(size_t)];                         // 内存池,64块内存,每块FC_LOG_ALLOC_BLOCK_SIZE字节
    static size_t log_pool_mem[FC_CALC_POOL_USABLE_SIZE(FC_LOG_ALLOC_BLOCK_SIZE, FC_LOG_POOL_TOTAL_SIZE) / sizeof(size_t)];  // 内存池,每块FC_LOG_ALLOC_BLOCK_SIZE字节,至少包含FC_LOG_POOL_TOTAL_SIZE字节的内存
    fc_pool_init(&fc_log_pool, log_pool_mem, sizeof(log_pool_mem), FC_LOG_ALLOC_BLOCK_SIZE);
}
INIT_EXPORT_ENV(fc_log_pool_init, FC_LOG_INIT_ORDER - 1);  // 确保在log_init之前初始化

/**
 * @brief
 *
 * @param alloc_type
 * @param pool
 * @param advice_size
 * @return fc_weak
 */
fc_weak bool log_alloc_default(fc_log_alloc_type_t alloc_type, fc_log_pool_t *pool, int advice_size)
{
    if (FC_LOG_ALLOC_NEW == alloc_type)
    {
        void *new_buff = fc_pool_alloc(&fc_log_pool, &(pool->size));

        if (new_buff)
        {
            if (pool->buff)  // 不是第一次分配
            {
                fc_pool_link(pool->buff, new_buff);  // 链接起来
            }
            pool->buff = new_buff;  // 切换到新内存块
            return true;
        }
    }
    else if (FC_LOG_ALLOC_FREE == alloc_type)
    {
        // 交给write的时候push,可以知道实际用到了多少内存
        // push到used_list尾部
        // fc_mempool_fifo_push(&g_mempool, pool->buff);
    }

    return false;
}

/**
 * @brief
 *
 * @param user
 * @param buf
 * @param len
 * @return fc_weak
 */
fc_weak int log_write_default(void *user, const char *buf, int len)
{
    if (buf == NULL || len <= 0)
    {
        return 0;
    }

    fc_log_file_user_t *file_user = (fc_log_file_user_t *)user;

    // 标记最后一块已使用大小
    fc_pool_mark_used((void *)(file_user->pool->buff), len - file_user->write_size);

    fc_pool_fifo_push(&fc_log_pool, (void *)buf);  // 压入fifo

    return len;
}
