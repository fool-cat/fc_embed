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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "fc_compiler.h"
#include "fc_helper.h"
#include "fc_port.h"

//+********************************* 宏配置项 **********************************/

#ifndef EOF
    #define EOF (-1)
#endif

// 等待一会儿,有操作系统建议使用操作系统的延时函数
#ifndef FC_WAIT_MOMENT
    #define FC_WAIT_MOMENT() ((void)0)
#endif

// 运行时断言
#ifndef fc_stdio_assert
    #define fc_stdio_assert(x) fc_assert(x)
#endif

#ifndef FIFO_TX_LOG2_SIZE
    /* 输出环形队列大小,2^n */
    // 4K Byte
    #define FIFO_TX_LOG2_SIZE 12
#endif

#ifndef STDOUT_TX_SINGLE_MAX_SHIFT
    // 单次发送最大字节数为缓冲区的1/(2^n),多段发送可以尽快空出部分缓冲区
    #define STDOUT_TX_SINGLE_MAX_SHIFT 2
#endif

// 连续发送
#ifndef PHY_SERIAL_TX_ENABLE
    #define PHY_SERIAL_TX_ENABLE 0
#endif

#ifndef FIFO_RX_LOG2_SIZE
    /* 输入环形队列大小,2^n */
    // 256 Byte
    #define FIFO_RX_LOG2_SIZE 8
#endif

#ifndef STDIN_RX_SINGLE_MAX_SHIFT
    // 单次接收最大字节数为缓冲区的1/(2^n),多段接收可以防止连续接收满了之后来不及处理
    #define STDIN_RX_SINGLE_MAX_SHIFT 1
#endif

// 连续接收
#ifndef PHY_SERIAL_RX_ENABLE
    #define PHY_SERIAL_RX_ENABLE 0
#endif

//+*********************************  **********************************/

/**
 * @brief 丢失数据处理
 *
 * @param port
 * @param buf
 * @param len
 * @return 后面的长度设为0可以获取丢失的长度
 */
fc_weak size_t fc_port_lose_hook(fc_port_t *port, const void *buf, size_t len)
{
    (void)port;
    (void)buf;
    (void)len;
    size_t lose = 0;

    if (port == &fc_stdout)
    {
        static volatile size_t _fc_stdout_lose = 0;
        _fc_stdout_lose += len;
        lose = _fc_stdout_lose;
    }
    else if (port == &fc_stdin)
    {
        static volatile size_t _fc_stdin_lose = 0;
        _fc_stdin_lose += len;
        lose = _fc_stdin_lose;
    }
    else
    {
        fc_stdio_assert(0);
    }

    return lose;
}

//+********************************* 面向对象 **********************************/
/**
 * @brief
 *
 * @param port
 * @param ch
 * @return int 成功返回ch,失败返回EOF
 */
int fc_port_putc(fc_port_t *port, int ch)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    int ret = ch;

    if (1 != fc_fifo_write(port->rb, (void *)&ch, 1))
    {
        ch = EOF;
    }

    if (EOF == ch)
    {
        (void)ret;  // 防止未使用警告
        FC_PORT_LOSE_HOOK(port, (void *)&ret, 1);
    }

    return ch;
}

/**
 * @brief
 *
 * @param port
 * @param str
 * @return int
 */
int fc_port_puts(fc_port_t *port, const char *str)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    size_t len = strlen(str);
    int    write_size = EOF;

    if (fc_fifo_get_free(port->rb) >= len)
    {
        write_size = fc_fifo_write(port->rb, (void *)str, len);
    }

    if (write_size < len)
    {
        FC_PORT_LOSE_HOOK(port, (void *)str, len);
    }

    return (int)write_size;
}

/**
 * @brief
 *
 * @param port
 * @param buf
 * @param len
 * @return int
 */
int fc_port_write(fc_port_t *port, const void *buf, size_t len)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    int write_size = EOF;

    if (fc_fifo_get_free(port->rb) >= len)
    {
        write_size = fc_fifo_write(port->rb, (void *)buf, len);
    }

    if (write_size < len)
    {
        FC_PORT_LOSE_HOOK(port, (void *)buf, len);
    }

    return write_size;
}

/**
 * @brief
 *
 * @param port
 * @param fmt
 * @param ...
 * @return int
 */
int fc_port_printf(fc_port_t *port, const char *fmt, ...)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    int ret = EOF;

    va_list arp;
    va_start(arp, fmt);
    ret = fc_port_vprintf(port, fmt, arp);
    va_end(arp);

    return ret;
}

/**
 * @brief
 *
 * @param port
 * @return int
 */
int fc_port_getc(fc_port_t *port)
{
    int ch;
    while (1 != fc_fifo_read(port->rb, (void *)&ch, 1))
    {
        FC_WAIT_MOMENT();
    }
    return ch;
}

/**
 * @brief
 *
 * @param port
 * @param buf
 * @param n
 * @return char*
 */
char *fc_port_gets(fc_port_t *port, char *buf, size_t n)
{
    int c;
    int i = 0;
    n--;

    for (;;)
    {
        c = fc_port_getc(port); /* Get a char from the incoming stream */
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
 * @param buf
 * @param len
 * @return int
 */
int fc_port_read(fc_port_t *port, void *buf, size_t len)
{
    int read_size = 0;
    read_size = fc_fifo_read(port->rb, buf, len);
    return read_size;
}

/**
 * @brief 从port中读取数据到buf中,但不删除数据
 *
 * @param port
 * @param buf
 * @param len
 * @return int
 */
int fc_port_peek(fc_port_t *port, void *buf, size_t len)
{
    int read_size = 0;
    read_size = fc_fifo_peek(port->rb, buf, len);
    return read_size;
}

//+********************************* port **********************************/
/**
 * @brief
 *
 * @param port
 */
void fc_port_trigger(fc_port_t *port)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    bool   busy;
    size_t size;
    void  *buf;

    if (port->dir == FC_PORT_DIR_OUT)
    {
        busy = fc_fifo_linear_read_busy(port->rb);

        if (!busy)
        {
            if (port->single_max_shift)
            {
                fc_stdio_assert(fc_fifo_get_size(port->rb) > (1 << port->single_max_shift));
                buf = fc_fifo_linear_read_setup_limit(port->rb, &size, port->single_max_shift);
            }
            else
            {
                buf = fc_fifo_linear_read_setup(port->rb, &size);
            }
        }
    }
    else
    {
        busy = fc_fifo_linear_write_busy(port->rb);

        if (!busy)
        {
            if (port->single_max_shift)
            {
                fc_stdio_assert(fc_fifo_get_size(port->rb) > (1 << port->single_max_shift));
                buf = fc_fifo_linear_write_setup_limit(port->rb, &size, port->single_max_shift);
            }
            else
            {
                buf = fc_fifo_linear_write_setup(port->rb, &size);
            }
        }
    }

    if (busy)
    {
        return;
    }

    fc_stdio_assert(port->phy != NULL);
    port->phy(buf, size);
}

/**
 * @brief
 *
 * @param port
 * @param size
 */
void fc_port_end(fc_port_t *port, int size)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    if (size < 0)
    {
        if (port->dir == FC_PORT_DIR_OUT)
        {
            fc_fifo_linear_read_done(port->rb, fc_fifo_linear_read_get_size(port->rb));
        }
        else
        {
            fc_fifo_linear_write_done(port->rb, fc_fifo_linear_write_get_size(port->rb));
        }
    }
    else
    {
        if (port->dir == FC_PORT_DIR_OUT)
        {
            fc_fifo_linear_read_done(port->rb, size);
        }
        else
        {
            fc_fifo_linear_write_done(port->rb, size);
        }
    }

    if (port->trigger_serial)
    {
        return fc_port_trigger(port);
    }
}

/**
 * @brief
 *
 * @param port
 * @return int
 */
int fc_port_available(fc_port_t *port)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    if (port->dir == FC_PORT_DIR_OUT)
    {
        return (int)fc_fifo_get_used(port->rb);
    }
    else
    {
        return (int)fc_fifo_get_free(port->rb);
    }
}

/**
 * @brief
 *
 * @param port
 * @return int
 */
int fc_port_free(fc_port_t *port)
{
    fc_stdio_assert(port != NULL);
    fc_stdio_assert(port->rb != NULL);

    if (port->dir == FC_PORT_DIR_OUT)
    {
        return (int)fc_fifo_get_free(port->rb);
    }
    else
    {
        return (int)fc_fifo_get_used(port->rb);
    }
}

int fc_fifo_printf(fc_fifo_t *fifo, const char *fmt, ...)
{
    fc_stdio_assert(fifo != NULL);
    fc_stdio_assert(fmt != NULL);

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

/**
 * @brief
 *
 */
void fc_stdio_init(void)
{
    static bool init = false;
    if (init)
    {
        return;  // 防止重复初始化
    }
    init = true;

#if !(STDOUT_TX_SINGLE_MAX_SHIFT < FIFO_TX_LOG2_SIZE && STDOUT_TX_SINGLE_MAX_SHIFT >= 0 && FIFO_TX_LOG2_SIZE >= 0)
    #error "STDOUT_TX_SINGLE_MAX_SHIFT must less than FIFO_TX_LOG2_SIZE,please check it"
    #error "单次发送位移必须小于缓冲区的log2大小,且两者必须同时大于等于0"
#endif

#if !(STDIN_RX_SINGLE_MAX_SHIFT < FIFO_RX_LOG2_SIZE && STDIN_RX_SINGLE_MAX_SHIFT >= 0 && FIFO_RX_LOG2_SIZE >= 0)
    #error "STDIN_RX_SINGLE_MAX_SHIFT must less than FIFO_RX_LOG2_SIZE,please check it"
    #error "单次接收位移必须小于缓冲区的log2大小,且两者必须同时大于等于0"
#endif

    {
        static const char str[] = "FC_OUT_RTT_MARK";  // "FC_OUT_RTT_MARK"
        memset(fc_stdout.id, 0, sizeof(fc_stdout.id));
        strncpy(fc_stdout.id, str, sizeof(fc_stdout.id) - 1);

        fc_stdout.single_max_shift = STDOUT_TX_SINGLE_MAX_SHIFT;
        fc_stdout.dir = FC_PORT_DIR_OUT;
        fc_stdout.trigger_serial = PHY_SERIAL_TX_ENABLE ? true : false;

        // 初始化环形队列,静态内存构造
        fc_fifo_static_new_at(fc_stdout.rb, FIFO_TX_LOG2_SIZE);
    }

    {
        static const char str[] = "FC_IN_RTT_MARK";  // "FC_IN_RTT_MARK"
        memset(fc_stdin.id, 0, sizeof(fc_stdin.id));
        strncpy(fc_stdin.id, str, sizeof(fc_stdin.id) - 1);

        fc_stdin.single_max_shift = STDIN_RX_SINGLE_MAX_SHIFT;
        fc_stdin.dir = FC_PORT_DIR_IN;
        fc_stdin.trigger_serial = PHY_SERIAL_RX_ENABLE ? true : false;

        // 初始化环形队列,静态内存构造
        fc_fifo_static_new_at(fc_stdin.rb, FIFO_RX_LOG2_SIZE);
    }
}

//+********************************* log组件write函数 **********************************/

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

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

#ifdef __cplusplus
}
#endif  //\ __cplusplus

//+********************************* 自动注册初始化 **********************************/
#include "fc_auto_init.h"
#if USE_FC_AUTO_INIT
static void _fc_port_auto_init(void)
{
    fc_stdio_init();  // 纯内存结构初始化,可以放在constructor的时候就初始化
}
INIT_EXPORT_ENV(_fc_port_auto_init, 100);  // 等级比默认的1000优先级更高,纯数据结构无外部依赖
#endif
