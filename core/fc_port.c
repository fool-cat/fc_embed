/**
 * @file retarget_stdio.c
 * @author fool-cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-01-30
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "fc_compiler.h"
#include "fc_helper.h"

#include "fc_config.h"
#include "fc_port.h"

//+********************************* 宏配置项 **********************************/

#ifndef EOF
    #define EOF (-1)
#endif

// 等待一会儿,有操作系统建议使用操作系统的延时函数
#ifndef FC_WAIT_MOMENT
    #define FC_WAIT_MOMENT() ((void)0)
#endif

#ifndef STDOUT_RB0_LOG2_SIZE
    /* 输出环形队列大小,2^n */
    // 4K Byte
    #define STDOUT_RB0_LOG2_SIZE 12
#endif

#ifndef STDOUT_RB0_TX_SINGLE_MAX_SHIFT
    // 单次发送最大字节数为缓冲区的1/(2^n),多段发送可以尽快空出部分缓冲区
    #define STDOUT_RB0_TX_SINGLE_MAX_SHIFT 2
#endif

#ifndef STDIN_RB0_LOG2_SIZE
    /* 输入环形队列大小,2^n */
    // 256 Byte
    #define STDIN_RB0_LOG2_SIZE 8
#endif

#ifndef STDIN_RB0_RX_SINGLE_MAX_SHIFT
    // 单次接收最大字节数为缓冲区的1/(2^n),多段接收可以防止连续接收满了之后来不及处理
    #define STDIN_RB0_RX_SINGLE_MAX_SHIFT 1
#endif

//+********************************* 提供的默认数据丢失处理钩子函数 **********************************/

// 端口写数据丢失处理
#ifndef FC_PORT_LOSE_HOOK
    #define FC_PORT_LOSE_HOOK(p_port, rb_index, buf, len) fc_port_lose_hook(p_port, rb_index, buf, len)
#endif

/**
 * @brief 丢失数据处理
 *
 * @param port
 * @param buf
 * @param len
 * @return 后面的长度设为0可以获取丢失的长度
 */
fc_weak size_t fc_port_lose_hook(fc_port_t *port, size_t rb_index, const void *buf, size_t len)
{
    (void)port;
    (void)buf;
    (void)len;
    size_t lose = 0;

    if (port == &fc_stdout)
    {
        static volatile size_t _fc_stdout_lose[PORT_RB_NUM] = {0};
        _fc_stdout_lose[rb_index] += len;
        lose = _fc_stdout_lose[rb_index];
    }
    else if (port == &fc_stdin)
    {
        static volatile size_t _fc_stdin_lose[PORT_RB_NUM] = {0};
        _fc_stdin_lose[rb_index] += len;
        lose = _fc_stdin_lose[rb_index];
    }
    else
    {
        fc_assert(0);
    }

    return lose;
}

//+********************************* 面向对象 **********************************/
/**
 * @brief 初始化port,其实就是设置方向
 *
 * @param port
 * @param dir
 */
void fc_port_init(fc_port_t *port, fc_port_dir_t dir)
{
    fc_assert(port != NULL);
    memset(port, 0, sizeof(fc_port_t));
    port->dir = (uint8_t)dir;
}

/**
 * @brief port绑定环形缓冲区
 *
 * @param port
 * @param rb_index 环形缓冲区索引
 * @param fifo 环形缓冲区指针
 * @param name 缓冲区名字
 * @param single_limit 单次读写限制(针对慢速IO而言),限制大小为缓冲区空间的1/(2^n),n=single_limit
 */
void fc_port_catch_fifo(fc_port_t *port, size_t rb_index, fc_fifo_t *fifo, const char *name, uint8_t single_limit)
{
    fc_assert(port != NULL);
    fc_assert(rb_index < PORT_RB_NUM);
    fc_assert(fifo != NULL);

    port->rb[rb_index] = fifo;
    port->rb_name[rb_index] = name;
    port->rb_single_limit[rb_index] = single_limit;
}

/**
 * @brief 绑定物理IO
 *
 * @param port
 * @param phy
 */
void fc_port_catch_phy(fc_port_t *port, fc_phy_io_t phy)
{
    fc_assert(port != NULL);
    port->phy = phy;
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @param ch
 * @return int 成功返回ch,失败返回EOF
 */
int fc_port_putc(fc_port_t *port, size_t rb_index, int ch)
{
    fc_assert(port != NULL);
    fc_assert(port->rb[rb_index] != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];
    int        ret = ch;

    FC_PORT_LOCK(port, rb_index, FC_PORT_DIR_WRITE);
    if (1 != fc_fifo_write(fifo, (void *)&ch, 1))
    {
        ch = EOF;
    }
    FC_PORT_UNLOCK(port, rb_index, FC_PORT_DIR_WRITE);

    if (EOF == ch)
    {
        (void)ret;  // 防止未使用警告
        FC_PORT_LOSE_HOOK(port, rb_index, (void *)&ret, 1);
    }

    return ch;
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @param str
 * @return int
 */
int fc_port_puts(fc_port_t *port, size_t rb_index, const char *str)
{
    fc_assert(port != NULL);
    fc_assert(port->rb[rb_index] != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];
    size_t     len = strlen(str);
    int        write_size = EOF;

    FC_PORT_LOCK(port, rb_index, FC_PORT_DIR_WRITE);
    if (fc_fifo_get_free(fifo) >= len)
    {
        write_size = fc_fifo_write(fifo, (void *)str, len);
    }
    FC_PORT_UNLOCK(port, rb_index, FC_PORT_DIR_WRITE);

    if (write_size < len)
    {
        FC_PORT_LOSE_HOOK(port, rb_index, (void *)str, len);
    }

    return (int)write_size;
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @param buf
 * @param len
 * @return int
 */
int fc_port_write(fc_port_t *port, size_t rb_index, const void *buf, size_t len)
{
    fc_assert(port != NULL);
    fc_assert(port->rb[rb_index] != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];
    int        write_size = EOF;

    FC_PORT_LOCK(port, rb_index, FC_PORT_DIR_WRITE);
    if (fc_fifo_get_free(fifo) >= len)
    {
        write_size = fc_fifo_write(fifo, (void *)buf, len);
    }
    FC_PORT_UNLOCK(port, rb_index, FC_PORT_DIR_WRITE);

    if (write_size < len)
    {
        FC_PORT_LOSE_HOOK(port, rb_index, (void *)buf, len);
    }

    return write_size;
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @param fmt
 * @param ...
 * @return int
 */
int fc_port_printf(fc_port_t *port, size_t rb_index, const char *fmt, ...)
{
    fc_assert(port != NULL);
    fc_assert(port->rb[rb_index] != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];
    int        ret = EOF;

    va_list arp;
    va_start(arp, fmt);

    FC_PORT_LOCK(port, rb_index, FC_PORT_DIR_WRITE);
    ret = fc_fifo_vprintf(fifo, fmt, arp);
    FC_PORT_UNLOCK(port, rb_index, FC_PORT_DIR_WRITE);

    va_end(arp);

    return ret;
}

/**
 * @brief fc_port_vprintf的核心实现,将格式化字符串写入到fc_port_t的环形缓冲区中
 *
 * @param port
 * @param fmt
 * @param arp
 * @return int
 */
int fc_port_vprintf(fc_port_t *port, size_t rb_index, const char *fmt, va_list arp)
{
    return fc_fifo_vprintf(port->rb[rb_index], fmt, arp);
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @return int
 */
int fc_port_getc(fc_port_t *port, size_t rb_index)
{
    fc_assert(port != NULL);
    fc_assert(port->rb[rb_index] != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];
    int        ch;

    FC_PORT_LOCK(port, rb_index, FC_PORT_DIR_READ);

    while (1 != fc_fifo_read(fifo, (void *)&ch, 1))
    {
        FC_WAIT_MOMENT();
    }

    FC_PORT_UNLOCK(port, rb_index, FC_PORT_DIR_READ);

    return ch;
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @param buf
 * @param n
 * @return char*
 */
char *fc_port_gets(fc_port_t *port, size_t rb_index, char *buf, size_t n)
{
    fc_assert(port != NULL);

    int c;
    int i = 0;
    n--;

    for (;;)
    {
        c = fc_port_getc(port, rb_index); /* Get a char from the incoming stream */
        if (c < 0 || c == '\n')
            break; /* End of stream or CR? */
        if (c == '\b' && i)
        { /* BS? */
            i--;
            continue;
        }
        if (c >= ' ' && i < n)
        { /* Visible chars? */
            buf[i++] = c;
        }
    }

    buf[i] = 0; /* Terminate with a \0 */
    return i ? buf : NULL;
}

/**
 * @brief 从port中读取数据到buf中,并删除数据
 *
 * @param port
 * @param rb_index
 * @param buf
 * @param len
 * @return int
 */
int fc_port_read(fc_port_t *port, size_t rb_index, void *buf, size_t len)
{
    fc_assert(port != NULL);
    fc_assert(port->rb[rb_index] != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];
    int        read_size = 0;

    FC_PORT_LOCK(port, rb_index, FC_PORT_DIR_READ);
    read_size = fc_fifo_read(fifo, buf, len);
    FC_PORT_UNLOCK(port, rb_index, FC_PORT_DIR_READ);

    return read_size;
}

/**
 * @brief 从port中读取数据到buf中,但不删除数据
 *
 * @param port
 * @param rb_index
 * @param buf
 * @param len
 * @return int
 */
int fc_port_peek(fc_port_t *port, size_t rb_index, void *buf, size_t len)
{
    fc_assert(port != NULL);
    fc_assert(port->rb[rb_index] != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];
    int        read_size = 0;

    FC_PORT_LOCK(port, rb_index, FC_PORT_DIR_READ);
    read_size = fc_fifo_peek(fifo, buf, len);
    FC_PORT_UNLOCK(port, rb_index, FC_PORT_DIR_READ);

    return read_size;
}

//+********************************* port **********************************/
/**
 * @brief
 *
 * @param port
 * @param rb_index
 */
void fc_port_trigger(fc_port_t *port, size_t rb_index)
{
    fc_assert(port != NULL);
    fc_assert(port->rb != NULL);

    size_t     size;
    void      *buf;
    fc_fifo_t *fifo = port->rb[rb_index];

    if ((uint8_t)FC_PORT_DIR_OUT == port->dir)
    {
        if (0 >= fc_port_used(port, rb_index))
        {
            return;  // 没有数据需要发送
        }

        if (fc_fifo_linear_read_busy(fifo))
        {
            return;  // 正在传输中,不响应
        }

        if (port->rb_single_limit[rb_index])
        {
            fc_assert(fc_fifo_get_size(fifo) > (1 << port->rb_single_limit[rb_index]));
            buf = fc_fifo_linear_read_setup_limit(fifo, &size, port->rb_single_limit[rb_index]);
        }
        else
        {
            buf = fc_fifo_linear_read_setup(fifo, &size);
        }
    }
    else
    {
        if (0 >= fc_port_free(port, rb_index))
        {
            return;  // 没有空间可以接收
        }

        if (fc_fifo_linear_write_busy(fifo))
        {
            return;  // 正在传输中,不响应
        }

        if (port->rb_single_limit[rb_index])
        {
            fc_assert(fc_fifo_get_size(fifo) > (1 << port->rb_single_limit[rb_index]));
            buf = fc_fifo_linear_write_setup_limit(fifo, &size, port->rb_single_limit[rb_index]);
        }
        else
        {
            buf = fc_fifo_linear_write_setup(fifo, &size);
        }
    }

    fc_assert(port->phy != NULL);
    port->phy(rb_index, buf, size);
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @param size
 */
void fc_port_end(fc_port_t *port, size_t rb_index, int size)
{
    fc_assert(port != NULL);
    fc_assert(port->rb != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];

    if (size < 0)
    {
        if ((uint8_t)FC_PORT_DIR_OUT == port->dir)
        {
            fc_fifo_linear_read_done(fifo, fc_fifo_linear_read_get_size(fifo));
        }
        else
        {
            fc_fifo_linear_write_done(fifo, fc_fifo_linear_write_get_size(fifo));
        }
    }
    else
    {
        if ((uint8_t)FC_PORT_DIR_OUT == port->dir)
        {
            fc_fifo_linear_read_done(fifo, size);
        }
        else
        {
            fc_fifo_linear_write_done(fifo, size);
        }
    }
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @return int
 */
int fc_port_used(fc_port_t *port, size_t rb_index)
{
    fc_assert(port != NULL);
    fc_assert(port->rb != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];

    return (int)fc_fifo_get_used(fifo);
}

/**
 * @brief
 *
 * @param port
 * @param rb_index
 * @return int
 */
int fc_port_free(fc_port_t *port, size_t rb_index)
{
    fc_assert(port != NULL);
    fc_assert(port->rb != NULL);

    fc_fifo_t *fifo = port->rb[rb_index];

    return (int)fc_fifo_get_free(fifo);
}

/**
 * @brief
 *
 * @param fifo
 * @param fmt
 * @param ...
 * @return int
 */
int fc_fifo_printf(fc_fifo_t *fifo, const char *fmt, ...)
{
    fc_assert(fifo != NULL);
    fc_assert(fmt != NULL);

    int ret = EOF;

    va_list arp;
    va_start(arp, fmt);
    ret = fc_fifo_vprintf(fifo, fmt, arp);
    va_end(arp);

    return ret;
}

//+********************************* 默认实例化对象 **********************************/

fc_port_t fc_stdin = {0};  // 对象创建
fc_port_t fc_stdout = {0};

static fc_port_rtt_t fc_port_rtt = {0};  // RTT支持

/**
 * @brief
 *
 */
void fc_default_port_init(void)
{
    static bool init = false;
    if (init)
    {
        return;  // 防止重复初始化
    }
    init = true;

#if !(STDOUT_RB0_TX_SINGLE_MAX_SHIFT < STDOUT_RB0_LOG2_SIZE && STDOUT_RB0_TX_SINGLE_MAX_SHIFT >= 0 && STDOUT_RB0_LOG2_SIZE > 0)
    #error "STDOUT_RB0_TX_SINGLE_MAX_SHIFT must less than STDOUT_RB0_LOG2_SIZE,please check it"
    #error "单次发送位移必须小于等于缓冲区的log2大小,请检查配置"
#endif

#if !(STDIN_RB0_RX_SINGLE_MAX_SHIFT < STDIN_RB0_LOG2_SIZE && STDIN_RB0_RX_SINGLE_MAX_SHIFT >= 0 && STDIN_RB0_LOG2_SIZE > 0)
    #error "STDIN_RB0_RX_SINGLE_MAX_SHIFT must less than STDIN_RB0_LOG2_SIZE,please check it"
    #error "单次接收位移必须小于等于缓冲区的log2大小,请检查配置"
#endif

    {
        fc_port_init(&fc_stdout, FC_PORT_DIR_OUT);

        // 初始化环形队列,静态内存构造,默认端口只给一个环形缓冲区分配内存
        fc_port_static_alloc_rb(&fc_stdout, 0, STDOUT_RB0_LOG2_SIZE, "fc_stdout_rb0", STDOUT_RB0_TX_SINGLE_MAX_SHIFT);
    }

    {
        fc_port_init(&fc_stdin, FC_PORT_DIR_IN);

        // 初始化环形队列,静态内存构造,默认端口只给一个环形缓冲区分配内存
        fc_port_static_alloc_rb(&fc_stdin, 0, STDIN_RB0_LOG2_SIZE, "fc_stdin_rb0", STDIN_RB0_RX_SINGLE_MAX_SHIFT);
    }

    {
        static const char mark_str[] = "\0\0\0\0KRAM TTR CF";  // "FC RTT MARK"内存标记点
        // static_assert(sizeof(mark_str) !=sizeof(fc_port_rtt.id), "mark_str size must equal fc_port_rtt.id size");
        for (size_t i = 0; i < sizeof(fc_port_rtt.id); i++)
        {
            fc_port_rtt.id[i] = mark_str[i];
        }
        fc_port_rtt.rb_array_size = PORT_RB_NUM;
        fc_port_rtt.port_in = &fc_stdin;
        fc_port_rtt.port_out = &fc_stdout;
    }

    (void)fc_port_rtt;  // 未使用也不要警告
}

//+********************************* 注册到ENV段中 **********************************/
#include "fc_auto_init.h"
// 等级比默认的1000优先级更高,纯数据结构无外部依赖
INIT_EXPORT_ENV(fc_default_port_init, FC_PORT_INIT_ORDER);
