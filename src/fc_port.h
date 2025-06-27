/**
 * @file fc_port.h
 * @author fool-cat (2696652257@qq.com)
 * @brief 提供带环形缓冲区的端口对象,提供了标准IO的重定向到端口对象的接口
 * @version 1.0
 * @date 2025-01-30
 *
 * @copyright Copyright (c) 2025
 *
 */

//> 单次包含宏定义
#ifndef __FC_STDIO_H__
#define __FC_STDIO_H__

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
#include <stddef.h>
#include <stdint.h>

#include "fc_fifo.h"

#include "fc_helper.h"

//> C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef FC_PORT_USE_LOCK
    #define FC_PORT_USE_LOCK 0 /**< 添加lock,unlock指针 */
#endif

#ifndef FC_PORT_USER_DATA
    #define FC_PORT_USER_DATA 0 /**< 添加user指针 */
#endif

// 建议支持嵌套
#ifndef FC_STDIO_ATOMIC
    #define FC_STDIO_ATOMIC
#endif

// 建议支持嵌套
#ifndef FC_PORT_ATOMIC
    #define FC_PORT_ATOMIC
#endif

// 等待一会儿,有操作系统建议使用操作系统的延时函数
#ifndef FC_WAIT_MOMENT
    #define FC_WAIT_MOMENT() ((void)0)
#endif

// 运行时断言
#ifndef fc_stdio_assert
    #define fc_stdio_assert(x) fc_assert(x)
#endif

    // 方向是对于内核来说(高速部分)
    typedef enum
    {
        FC_PORT_DIR_IN = 0,   // 慢速IO(通常是物理层)->环形缓冲->高速IO(通常是内核)
        FC_PORT_DIR_OUT = 1,  // 高速IO(通常是内核)->环形缓冲->慢速IO(通常是物理层)
    } fc_port_dir_t;

    typedef size_t (*fc_phy_io_t)(void *buf, size_t len);  // 返回值仅做保留

    typedef struct _fc_port_t fc_port_t;
    struct _fc_port_t
    {
        char        id[16];  // 端口ID,用于调试输出
        fc_fifo_t  *rb;      // 环形缓冲
        fc_phy_io_t phy;     // 物理IO接口

#if FC_PORT_USE_LOCK
        void (*lock)(fc_port_t *port);    // 加锁,必须使用递归锁
        void (*unlock)(fc_port_t *port);  // 解锁
#endif

#if FC_PORT_USER_DATA
        void *user;  // 自定义数据
#endif

        uint8_t       single_max_shift;  // 单次读写限制(针对慢速IO而言),限制大小为缓冲区空间的1/(2^n)
        fc_port_dir_t dir;               // 方向
        bool          trigger_serial;    // 连续触发
    };

    // 端口写数据丢失处理
#ifndef FC_PORT_LOSE_HOOK
    extern size_t fc_port_lose_hook(fc_port_t *port, const void *buf, size_t len);
    #define FC_PORT_LOSE_HOOK(p_port, buf, len) fc_port_lose_hook(p_port, buf, len)
#endif

    //+********************************* 面向对象 **********************************/
    extern int fc_port_putc(fc_port_t *port, int ch);
    extern int fc_port_puts(fc_port_t *port, const char *str);
    extern int fc_port_write(fc_port_t *port, const void *buf, size_t len);
    extern int fc_port_printf(fc_port_t *port, const char *fmt, ...);
    extern int fc_port_vprintf(fc_port_t *port, const char *fmt, va_list arp);  // fc_port_printf核心实现

    extern int   fc_port_getc(fc_port_t *port);                       // 阻塞式API,非线程安全
    extern char *fc_port_gets(fc_port_t *port, char *buf, size_t n);  // 不建议使用
    extern int   fc_port_read(fc_port_t *port, void *buf, size_t len);
    extern int   fc_port_peek(fc_port_t *port, void *buf, size_t len);           // 读取数据但不删除
    extern int   fc_port_scanf(fc_port_t *port, const char *fmt, ...);           // TODO:待实现
    extern int   fc_port_vscanf(fc_port_t *port, const char *fmt, va_list arp);  // fc_port_scanf核心实现

    // 以下API的行为取决于fc_port_t的方向(fc_port_dir_t)
    extern void fc_port_trigger(fc_port_t *port);        // 触发慢速IO
    extern void fc_port_end(fc_port_t *port, int size);  // 慢速IO完成回调
    extern int  fc_port_available(fc_port_t *port);      // 缓冲区可用字节数
    extern int  fc_port_free(fc_port_t *port);           // 获取缓冲区剩余空间

    //+********************************* 默认实例化对象 **********************************/

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
    #define PHY_SERIAL_TX_ENABLE 1
#endif

#ifndef FIFO_RX_LOG2_SIZE
    /* 输入环形队列大小,2^n */
    // 256 Byte
    #define FIFO_RX_LOG2_SIZE 8
#endif

#ifndef STDIN_RX_SINGLE_MAX_SHIFT
    // 单次接收最大字节数为缓冲区的1/(2^n),多段接收可以防止连续接收满了之后来不及处理
    #define STDIN_RX_SINGLE_MAX_SHIFT 2
#endif

// 连续接收
#ifndef PHY_SERIAL_RX_ENABLE
    #define PHY_SERIAL_RX_ENABLE 1
#endif

    // 声明输入输出对象
    extern fc_port_t fc_stdin;
    extern fc_port_t fc_stdout;
#define fc_stdin_phy_catch(func) (fc_stdin.phy = (fc_phy_io_t)func)
#define fc_stdout_phy_catch(func) (fc_stdout.phy = (fc_phy_io_t)func)

    // 为了提升效率单独实现IO触发与回调
    extern void fc_stdio_init(void);  // 初始化标准输入输出

#ifndef FC_STDIO_DEFINE_API
    #define FC_STDIO_DEFINE_API 0
#endif

#if (0 == FC_STDIO_DEFINE_API)

    // 输出到fc_stdout
    extern int fc_putchar(int ch);
    extern int fc_putc(int ch);
    extern int fc_puts(const char *str);
    extern int fc_write(const void *buf, size_t len);  // 要么全部写入,要么返回0
    extern int fc_printf(const char *fmt, ...);

    extern void fc_out_trigger(void);    // 触发发送
    extern void fc_out_end(int size);    // 发送完成处理
    extern int  fc_out_available(void);  // 缓冲区可用字节数
    extern int  fc_out_free(void);       // 缓冲区剩余空间

    /*----------------------------------------------*/
    /* Formatted string output                      */
    /*----------------------------------------------*/
    /*  fc_printf("%d",         1234);			"1234"
        fc_printf("%6d,%3d%%",  -200, 5);	    "  -200,  5%"
        fc_printf("%-6u",       100);			"100   "
        fc_printf("%ld",        12345678);		"12345678"
        fc_printf("%llu",       0x100000000);	"4294967296"    <XF_USE_LLI>
        fc_printf("%lld",       -1LL);			"-1"			<XF_USE_LLI>
        fc_printf("%04x",       0xA3);			"00a3"
        fc_printf("%08lX",      0x123ABC);		"00123ABC"
        fc_printf("%016b",      0x550F);	    "0101010100001111"
        fc_printf("%*d",        6, 100);		"   100"
        fc_printf("%s",         "String");		"String"
        fc_printf("%5s",        "abc");			"  abc"
        fc_printf("%-5s",       "abc");			"abc  "
        fc_printf("%-5s",       "abcdefg");		"abcdefg"
        fc_printf("%-5.5s",     "abcdefg");	    "abcde"
        fc_printf("%-.5s",      "abcdefg");	    "abcde"
        fc_printf("%-5.5s",     "abc");		    "abc  "
        fc_printf("%c",         'a');			"a"
        fc_printf("%12f",       10.0);			"   10.000000"	<XF_USE_FP>
        fc_printf("%.4E",       123.45678);		"1.2346E+02"	<XF_USE_FP>
    */

    // stdin
    extern int   fc_getchar(void);              // 阻塞式API,非线程安全
    extern int   fc_getc(void);                 // 阻塞式API,非线程安全
    extern char *fc_gets(char *buf, size_t n);  // 不建议使用
    extern int   fc_read(void *buf, size_t len);
    extern int   fc_scanf(const char *fmt, ...);

    extern void fc_in_trigger(void);    // 触发接收
    extern void fc_in_end(int size);    // 接收完成处理
    extern int  fc_in_available(void);  // 缓冲区可用字节数
    extern int  fc_in_free(void);       // 缓冲区剩余空间

#else

    #ifndef FC_STDOUT_OBJ
        #define FC_STDOUT_OBJ (&fc_stdout)
    #endif

    #ifndef FC_STDIN_OBJ
        #define FC_STDIN_OBJ (&fc_stdin)
    #endif

    // clang-format off

    // 输出到fc_stdout
    #define fc_putchar(ch)      fc_port_putc(FC_STDOUT_OBJ, ch)
    #define fc_putc(ch)         fc_port_putc(FC_STDOUT_OBJ, ch)
    #define fc_puts(str)        fc_port_puts(FC_STDOUT_OBJ, str)
    #define fc_write(buf, len)  fc_port_write(FC_STDOUT_OBJ, buf, len)  // 要么全部写入,要么返回0
    #define fc_printf(fmt, ...) fc_port_printf(FC_STDOUT_OBJ, fmt, ##__VA_ARGS__)

    #define fc_out_trigger()    fc_port_trigger(FC_STDOUT_OBJ)      // 触发发送
    #define fc_out_end(size)    fc_port_end(FC_STDOUT_OBJ, size)    // 发送完成处理
    #define fc_out_available()  fc_port_available(FC_STDOUT_OBJ)    // 缓冲区可用字节数
    #define fc_out_free()       fc_port_free(FC_STDOUT_OBJ)         // 缓冲区剩余空间

    /*----------------------------------------------*/
    /* Formatted string output                      */
    /*----------------------------------------------*/
    /*  fc_printf("%d",         1234);			"1234"
        fc_printf("%6d,%3d%%",  -200, 5);	    "  -200,  5%"
        fc_printf("%-6u",       100);			"100   "
        fc_printf("%ld",        12345678);		"12345678"
        fc_printf("%llu",       0x100000000);	"4294967296"    <XF_USE_LLI>
        fc_printf("%lld",       -1LL);			"-1"			<XF_USE_LLI>
        fc_printf("%04x",       0xA3);			"00a3"
        fc_printf("%08lX",      0x123ABC);		"00123ABC"
        fc_printf("%016b",      0x550F);	    "0101010100001111"
        fc_printf("%*d",        6, 100);		"   100"
        fc_printf("%s",         "String");		"String"
        fc_printf("%5s",        "abc");			"  abc"
        fc_printf("%-5s",       "abc");			"abc  "
        fc_printf("%-5s",       "abcdefg");		"abcdefg"
        fc_printf("%-5.5s",     "abcdefg");	    "abcde"
        fc_printf("%-.5s",      "abcdefg");	    "abcde"
        fc_printf("%-5.5s",     "abc");		    "abc  "
        fc_printf("%c",         'a');			"a"
        fc_printf("%12f",       10.0);			"   10.000000"	<XF_USE_FP>
        fc_printf("%.4E",       123.45678);		"1.2346E+02"	<XF_USE_FP>
    */

    // stdin
    #define fc_getchar()        fc_port_getc(FC_STDIN_OBJ)              // 阻塞式API,非线程安全
    #define fc_getc()           fc_port_getc(FC_STDIN_OBJ)              // 阻塞式API,非线程安全
    #define fc_gets(buf, n)     fc_port_gets(FC_STDIN_OBJ, buf, n)      // 不建议使用
    #define fc_read(buf, len)   fc_port_read(FC_STDIN_OBJ, buf, len)    // 阻塞式API,非线程安全
    #define fc_scanf(fmt, ...)  fc_port_scanf(FC_STDIN_OBJ, fmt, ##__VA_ARGS__)

    #define fc_in_trigger()     fc_port_trigger(FC_STDIN_OBJ)       // 触发接收
    #define fc_in_end(size)     fc_port_end(FC_STDIN_OBJ, size)     // 接收完成处理
    #define fc_in_available()   fc_port_available(FC_STDIN_OBJ)     // 缓冲区可用字节数
    #define fc_in_free()        fc_port_free(FC_STDIN_OBJ)          // 缓冲区剩余空间

    // clang-format on

#endif  // FC_STDIO_DEFINE_API

#ifdef __cplusplus
}
#endif

#endif  // __FC_STDIO_H__
