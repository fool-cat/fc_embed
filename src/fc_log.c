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
#include "fc_log.h"
#include "fc_port.h"

#ifndef fc_assert
    #define fc_assert(x) ((void)(0))
#endif  //\ fc_assert

#if FC_LOG_USE_FC_POOL
    #include "fc_stdio.h"
    #define LOG_VSNPRINTF fc_vsnprintf
#else
    #include <stdio.h>
    #define LOG_VSNPRINTF vsnprintf
#endif  //\ LOG_USE_VSNPRINTF

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

#if (0 != FC_LOG_USE_FC_POOL)

    #define LOG_FILE_LINEAR_WRITE 1

// 传递给write函数的用户数据
typedef struct _fc_log_file_user_t fc_log_file_user_t;
struct _fc_log_file_user_t
{
    fc_log_t      *log;         // 关联的log对象
    fc_log_pool_t *pool;        // 第一块内存池
    void          *start_pool;  // 记录第一块内存地址
    size_t         write_size;  // 已写入的大小

    #if !(LOG_FILE_LINEAR_WRITE)
    size_t offset;  // 偏移
    #endif
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
    #if LOG_FILE_LINEAR_WRITE

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

    #else

    // 分配内存(FC_IO_SWAP == 0)和结束(FC_IO_EOF == -1)和写入数据(len > 0,实际每次仅写入1字节)都会调用
    if (len >= FC_IO_SWAP)
    {
        fc_log_file_user_t *user = (fc_log_file_user_t *)f->user;
        if (user->offset >= user->pool->size)
        {
            fc_log_t      *log = user->log;
            fc_log_pool_t *pool = user->pool;
            user->write_size += pool->size;
            if (false == log->alloc(FC_LOG_ALLOC_NEW, pool, FC_LOG_LINE_SIZE))
            {
                // 结束序列化
                return -1;  // 这里丢失由内存池分配函数alloc记录
            }
            user->offset = 0;
        }

        *((char *)(user->pool->buff) + user->offset) = *((char *)buf);  // 直接写入当前内存块
        user->offset++;
    }

    #endif

    return len;
}

#endif  //\ FC_LOG_USE_FC_POOL

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
        int           len = 0;
        fc_log_pool_t pool = {0};
        if (false == log->alloc(FC_LOG_ALLOC_NEW, &pool, FC_LOG_LINE_SIZE))  // 分配内存池
        {
            return;  // 这里丢失由内存池分配函数alloc记录
        }

#if (0 == FC_LOG_USE_FC_POOL)

        {
            va_list vargs;
            va_start(vargs, fmt);
            len = LOG_VSNPRINTF(pool.buff, pool.size, fmt, vargs);
            va_end(vargs);
        }

        if (len > pool.size)
        {
            if (false == log->alloc(FC_LOG_ALLOC_REALLOC, &pool, len + 1))  // 重新分配内存池
            {
                // 这一次丢失由LOSE_HOOK记录
                FC_LOG_LOSE_HOOK(false, log, NULL, len - pool.size);
                len = pool.size;  // 截断
            }
            else
            {
                va_list vargs;
                va_start(vargs, fmt);
                len = LOG_VSNPRINTF(pool.buff, pool.size, fmt, vargs);
                va_end(vargs);
            }
        }

#else

        fc_log_file_user_t user = {0};
        {
            user.log = log;
            user.pool = &pool;
            user.start_pool = pool.buff;

            FC_FILE f = {0};
    #if LOG_FILE_LINEAR_WRITE
            f.p_now = pool.buff;
            f.p_start = pool.buff;
            f.p_end = pool.buff + pool.size;
    #endif
            f.user = (void *)&user;
            f.io.write = __fc_log_alloc_write;

            va_list vargs;
            va_start(vargs, fmt);
            len = fc_vfprintf(&f, fmt, vargs);
            va_end(vargs);
        }

#endif  //\ FC_LOG_USE_FC_POOL

        len -= log->write((void *)&user, user.start_pool, len);  // 写入数据
        FC_LOG_LOSE_HOOK(0 == len, log, pool.buff, len);
        log->alloc(FC_LOG_ALLOC_FREE, &pool, 0);  // 释放内存池
    }
}

#if 0
// 废弃
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
void fc_log_fprintf_stack(fc_log_t *log, fc_log_level_t level, char *stack_buf, int stack_size, const char *fmt, ...)
{
    fc_assert(log != NULL);
    fc_assert(stack_buf != NULL);
    fc_assert(stack_size > 0);

    if (log->level >= level)
    {
        int                len = 0;
        fc_log_file_user_t user = {0};
        {
            user.log = log;
            user.pool = NULL;
            user.start_pool = NULL;
        }

        {
            va_list vargs;
            va_start(vargs, fmt);
            len = LOG_VSNPRINTF(stack_buf, stack_size, fmt, vargs);
            va_end(vargs);
        }

        len -= log->write((void *)&user, stack_buf, len);  // 写入数据
        FC_LOG_LOSE_HOOK(0 == len, log, stack_buf, len);
    }
}
#endif

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

        FC_LOG_LOSE_HOOK(0 == len, log, buff, len);
    }
}
#endif

//+********************************* 写入丢失记录 **********************************/

/**
 * @brief log写入丢失钩子,弱函数,定义了自己的fc_log对象可以重写
 *  记得在宏FC_LOG_LOSE_HOOK里面去开启,默认是关闭了的
 * @param log
 * @param buff 有可能是NULL
 * @param len buff为NULL时表示内存池分配失败截断丢弃的长度
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
        fc_assert(log != NULL);
    }

    return count;
}

//+********************************* 提供一份默认log对象 **********************************/

// 默认实例化对象
FC_LOG_IMPL(default_log, FC_LOG_ALL, log_write_default, log_alloc_default);

#if (0 == FC_LOG_USE_FC_POOL)

//+********************************* 默认自带的写函数 **********************************/

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
    (void)user;
    // 提供两个默认的write函数,一个是输出到fc_stdout的0号缓冲区,一个是输出到fc_trans的0号虚拟页
    extern int log_write_stdout(const char *buf, int len);  // 放在fc_port.c中实现,提供一份弱实现
    extern int log_write_trans(const char *buf, int len);   // 放在fc_trans.c中实现,提供一份弱实现

    return log_write_stdout(buf, len);
}

//+********************************* 默认自带的内存池分配函数(无'并行'支持) **********************************/

/**
 * @brief  弱函数建议重写一份,用于调用日志组件的时候分配缓冲区,如果是操作系统可以提供多份固定大小的内存池或者动态内存
 *
 * @param alloc_type
 * @param pool
 * @param advice_size
 * @return bool 返回值表示是否成功分配内存池
 */
fc_weak bool log_alloc_default(fc_alloc_type_t alloc_type, fc_log_pool_t *pool, int advice_size)
{
    (void)alloc_type;
    (void)advice_size;

    fc_assert(pool != NULL);

    static char __buff[FC_LOG_LINE_SIZE] = {0};  // 默认内存池大小

    bool ret = false;

    if (FC_LOG_ALLOC_NEW == alloc_type)
    {
        // 默认为单线程且不考虑中断等情况
        pool->buff = __buff;          // 内存池缓冲区
        pool->size = sizeof(__buff);  // 内存池大小

        ret = true;
    }
    else
    {
        // 仅仅用于表示有丢失(数量不准确)
        FC_LOG_LOSE_HOOK(false, FC_LOG_OBJ, &__buff, sizeof(__buff));
    }

    return ret;
}

#else

//+********************************* 使用fc_pool进行管理 **********************************/

    #include "fc_auto_init.h"
    #include "fc_pool.h"

fc_pool_t fc_log_pool;  // log组件使用的内存池

    // log组件使用的内存池总大小(字节),建议向上取整到sizeof(size_t)
    #ifndef FC_LOG_POOL_TOTAL_SIZE
        #define FC_LOG_POOL_TOTAL_SIZE (4 * 1024) /* 默认4KB */
    #endif

    // 每块pool用户可使用大小(字节),内部会向上取整到sizeof(size_t)
    #ifndef FC_LOG_ALLOC_BLOCK_SIZE
        #define FC_LOG_ALLOC_BLOCK_SIZE FC_LOG_LINE_SIZE /* 128的话满足大多数情况下日志需求,减少内存分配次数 */
    #endif

void fc_log_pool_init(void)
{
    static size_t log_pool_mem[FC_LOG_POOL_TOTAL_SIZE / sizeof(size_t)];  // 内存池
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
fc_weak bool log_alloc_default(fc_alloc_type_t alloc_type, fc_log_pool_t *pool, int advice_size)
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

#endif  //\ FC_LOG_USE_FC_POOL
