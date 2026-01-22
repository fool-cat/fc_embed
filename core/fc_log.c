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

#include "fc_arch.h"

#ifndef fc_assert
    #define fc_assert(x) ((void)(0))
#endif  //\ fc_assert

#ifndef FC_ATOMIC_SCOPE
    #define FC_ATOMIC_SCOPE
#endif

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

/**
 * @brief 能到这个函数表明一定出现了内存分配失败,后续所有的记录都没有意义,记录丢失就好
 *
 * @param f
 * @param buf
 * @param len 每次只会写入1字节,结尾会调用一次0长度的写入
 * @return int
 */
static int __fc_log_fail_record_write(FC_FILE *f, const void *buf, int len)
{
    if (len >= FC_IO_SWAP)
    {
        fc_log_file_user_t *user = (fc_log_file_user_t *)f->user;
        // 调用钩子记录丢失的日志长度
        FC_LOG_LOSE_HOOK(false, user->log, len);
    }

    return len;
}

/**
 * @brief 具体查看vfprintf.c中_write_ch函数
 *
 * @param f
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

        fc_log_t     *log = user->log;
        fc_log_mem_t *mem = &(user->mem);
        user->block_write += mem->size;
        if (false == log->alloc(FC_LOG_ALLOC_NEW, mem, FC_LOG_LINE_SIZE))
        {
            f->io.write = __fc_log_fail_record_write;  // 改变下一次进来的函数,后续只做丢失记录
            return len;
        }

        // 切换到新的内存块
        f->p_now = mem->buff;
        f->p_start = mem->buff;
        f->p_end = (char *)((uint8_t *)(mem->buff) + mem->size);
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

    if (log->merge)
    {
        log->last_level = level;  // 记录临时等级
    }

    if (log->level >= level)
    {
        FC_FILE            temp_f = {0};
        fc_log_file_user_t temp_user = {0};

        FC_FILE            *f = &temp_f;
        fc_log_file_user_t *user = &temp_user;

        if (log->merge)  // 推迟输出,使用对象自身的f和user
        {
            f = &log->f;
            user = &log->file_user;
        }

        if (!user->mem.buff && NULL == f->io.write)  // 还没有分配过内存并设置write指针(第一次进入)
        {
            // 分配内存池
            if (false == log->alloc(FC_LOG_ALLOC_NEW, &(user->mem), FC_LOG_LINE_SIZE))
            {
                va_list vargs;
                va_start(vargs, fmt);
                user->total_write = fc_vsnprintf(NULL, 0, fmt, vargs);
                va_end(vargs);

                // 调用钩子记录丢失的日志长度
                FC_LOG_LOSE_HOOK(0 == user->total_write, log, user->total_write);

                f->io.write = __fc_log_fail_record_write;  // 改变下一次进来的函数,后续只做丢失记录,一次失败后续连续失败,避免一个段内的日志存在中间丢失两边存在的情况

                return;
            }

            user->log = (log->file_user.log) ? log->file_user.log : log;  // 始终指向其根对象,如果根对象没有赋值的话就指向当前log对象(代价是日志丢失没有记录)
            user->mem_chain = user->mem.buff;
            // user->block_write = 0;

            f->p_now = user->mem.buff;
            f->p_start = user->mem.buff;
            f->p_end = user->mem.buff + user->mem.size;
            f->user = (void *)user;
            f->io.write = __fc_log_alloc_write;
        }

        {
            va_list vargs;
            va_start(vargs, fmt);
            user->total_write = fc_vfprintf(f, fmt, vargs);
            va_end(vargs);
        }

        if (!log->merge)
        {
            // 这里必须展开调用,使用的是栈对象
            //  fc_log_fflush(log);

            // 写入数据,数据早已写入,这里是将内存块链入fifo
            log->write(user);

            // 释放内存池
            log->alloc(FC_LOG_ALLOC_FREE, &(user->mem), 0);
        }
    }
}

/**
 * @brief 从fc_vfprintf中提取出来的写入单个字符函数
 *
 * @param f
 * @param ch
 * @return true
 * @return false
 */
static inline bool _write_ch(FC_FILE *f, char ch)
{
    if (f->p_now)
    {
        *f->p_now++ = (char)ch;
        f->n++;
        if ((size_t)f->p_now >= (size_t)f->p_end)
        {
            f->p_now = NULL;
            if (f->io.write)
            {
                // 准备交换(重新设置)缓冲区
                if ((int)FC_IO_SWAP != f->io.write(f, f->p_now, (int)FC_IO_SWAP))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }
    else if (f->io.write)
    {
        if (1 != f->io.write(f, &ch, 1))
        {
            return false;
        }
        f->n++;
    }
    else
    {
        return false;
    }

    return true;
}

/**
 * @brief
 *
 * @param log
 * @param level
 * @param buff
 * @param len
 * @return int
 */
int fc_log_fwrite(fc_log_t *log, fc_log_level_t level, const void *buff, int len)
{
    fc_assert(log != NULL);

    if (log->merge)
    {
        log->last_level = level;  // 记录临时等级
    }

    if (log->level >= level)
    {
        FC_FILE            temp_f = {0};
        fc_log_file_user_t temp_user = {0};

        FC_FILE            *f = &temp_f;
        fc_log_file_user_t *user = &temp_user;

        if (log->merge)  // 推迟输出,使用对象自身的f和user
        {
            f = &log->f;
            user = &log->file_user;
        }

        user->total_write = len;

        if (!user->mem.buff && NULL == f->io.write)  // 还没有分配过内存并设置write指针(第一次进入)
        {
            // 分配内存池
            if (false == log->alloc(FC_LOG_ALLOC_NEW, &(user->mem), FC_LOG_LINE_SIZE))
            {
                // 调用钩子记录丢失的日志长度
                FC_LOG_LOSE_HOOK(false, log, len);

                f->io.write = __fc_log_fail_record_write;  // 改变下一次进来的函数,后续只做丢失记录,一次失败后续连续失败,避免一个段内的日志存在中间丢失两边存在的情况

                return 0;
            }

            user->log = log;
            user->mem_chain = user->mem.buff;
            // user->block_write = 0;

            // 保持跟fprintf一样
            f->p_now = user->mem.buff;
            f->p_start = user->mem.buff;
            f->p_end = user->mem.buff + user->mem.size;
            f->user = (void *)user;
            f->io.write = __fc_log_alloc_write;
        }

        {
            char *this_buff = (char *)buff;
            do
            {
                if (!_write_ch(f, *this_buff))
                    break;
                this_buff++;
                len--;
            } while (len > 0);

            if (f->io.write)
            {
                f->io.write(f, f->p_now, (int)FC_IO_EOF);  // 结束
            }
        }

        if (!log->merge)
        {
            // 这里必须展开调用,使用的是栈对象
            //  fc_log_fflush(log);

            // 写入数据,数据早已写入,这里是将内存块链入fifo
            log->write(user);

            // 释放内存池
            log->alloc(FC_LOG_ALLOC_FREE, &(user->mem), 0);
        }

        return (int)(user->total_write - len);
    }

    return 0;
}

/**
 * @brief 缓冲区输出
 *
 * @param log
 */
void fc_log_fflush(fc_log_t *log)
{
    fc_assert(log != NULL);

    if (log->merge)
    {
        fc_log_file_user_t *user = &log->file_user;

        // 写入数据,数据早已写入,这里是将内存块链入fifo
        log->write(user);

        // 释放内存池
        log->alloc(FC_LOG_ALLOC_FREE, &(user->mem), 0);
    }
}

//+********************************* 写入丢失记录 **********************************/

/**
 * @brief log写入丢失钩子,弱函数
 *  记得在宏FC_LOG_LOSE_HOOK里面去开启,默认是关闭了的
 * @param log
 * @param len buff为NULL时表示内存池分配失败截断丢弃的长度
 * @return size_t 弱函数,可以在外面重写
 */
fc_weak size_t fc_log_write_lose_hook(fc_log_t *log, int len)
{
    (void)log;
    (void)len;

    size_t lose_count = 0;

    if (log == &default_log)
    {
        static size_t default_lose_count = 0;
        FC_ATOMIC_SCOPE
        {
            default_lose_count += len;
        }
        lose_count = default_lose_count;
    }
    else
    {
    }

    return lose_count;
}

//+********************************* 提供一份默认log对象 **********************************/

// 默认实例化对象
fc_log_t default_log = {
    .write = log_write_default,
    .alloc = log_alloc_default,
    .file_user = {0},  // 临时对象中才会用到这个内存,其他都不用
    .f = {0},
    .level = FC_LOG_ALL,
    .last_level = FC_LOG_NONE,  // 不限制
    .merge = false,             // 默认不需要推迟输出
};

fc_log_t const *scope_log_ptr = NULL;  // 设置为空指针!!!

//+********************************* 使用fc_pool进行管理 **********************************/

fc_pool_t fc_log_pool;  // log组件使用的内存池

// log组件可使用的内存池大小(字节),建议向上取整到sizeof(size_t)
#ifndef FC_LOG_POOL_TOTAL_SIZE
    #define FC_LOG_POOL_TOTAL_SIZE (4 * 1024) /* 默认4KB */
#endif

// 每块pool用户可使用大小(字节),内部会向上取整到sizeof(size_t)
#ifndef FC_LOG_ALLOC_BLOCK_SIZE
    #define FC_LOG_ALLOC_BLOCK_SIZE FC_LOG_LINE_SIZE /* 128的话满足大多数情况下日志需求,减少内存分配次数 */
#endif

void fc_log_init(void)
{
    default_log.file_user.log = &default_log;  // 根对象初始化的时候必须将此指针指向自身!!!

    // static size_t log_pool_mem[FC_CALC_POOL_MEM_SIZE(FC_LOG_ALLOC_BLOCK_SIZE, 64) / sizeof(size_t)];                         // 内存池,64块内存,每块FC_LOG_ALLOC_BLOCK_SIZE字节
    static size_t log_pool_mem[FC_CALC_POOL_USABLE_SIZE(FC_LOG_ALLOC_BLOCK_SIZE, FC_LOG_POOL_TOTAL_SIZE) / sizeof(size_t)];  // 内存池,每块FC_LOG_ALLOC_BLOCK_SIZE字节,至少包含FC_LOG_POOL_TOTAL_SIZE字节的内存
    fc_pool_init(&fc_log_pool, log_pool_mem, sizeof(log_pool_mem), FC_LOG_ALLOC_BLOCK_SIZE);
}

/**
 * @brief
 *
 * @param alloc_type
 * @param mem
 * @param advice_size
 * @return fc_weak
 */
fc_weak bool log_alloc_default(fc_log_alloc_type_t alloc_type, fc_log_mem_t *mem, int advice_size)
{
    if (FC_LOG_ALLOC_NEW == alloc_type)
    {
        void *new_buff = fc_pool_alloc(&fc_log_pool, &(mem->size));

        if (new_buff)
        {
            if (mem->buff)  // 不是第一次分配
            {
                fc_pool_link(mem->buff, new_buff);  // 链接起来
            }
            mem->buff = new_buff;  // 切换到新内存块
            return true;
        }
    }
    else if (FC_LOG_ALLOC_FREE == alloc_type)
    {
        // 交给write的时候push,可以知道实际用到了多少内存
        // push到used_list尾部
        // fc_mempool_fifo_push(&g_mempool, mem->buff);
    }

    return false;
}

/**
 * @brief
 *
 * @param file_user
 * @return fc_weak
 */
fc_weak size_t log_write_default(fc_log_file_user_t *file_user)
{
    if (file_user == NULL)
    {
        return 0;
    }

    // 标记最后一块已使用大小
    fc_pool_mark_used((void *)(file_user->mem.buff), file_user->total_write - file_user->block_write);

    fc_pool_fifo_push(&fc_log_pool, (void *)file_user->mem_chain);  // 压入fifo

    return file_user->total_write;
}

//+********************************* 注册到ENV段中 **********************************/
#include "fc_auto_init.h"
INIT_EXPORT_ENV(fc_log_init, FC_LOG_INIT_ORDER);
