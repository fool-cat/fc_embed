/**
 * @file fc_port.h
 * @author fool-cat (2696652257@qq.com)
 * @brief 提供带环形缓冲区的端口对象,提供了标准IO的重定向到端口对象的接口,不加锁,由使用者自行确保线程安全
 * @version 1.0
 * @date 2025-01-30
 *
 * @copyright Copyright (c) 2025
 *
 */

// > 单次包含宏定义
#ifndef _FC_PORT_H_
#define _FC_PORT_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fc_config.h"
#include "fc_fifo.h"

#ifdef __cplusplus
extern "C"
{
#endif

// 类似SEGGER RTT,一个端口可以有多个缓冲区
#ifndef PORT_RB_NUM
    #define PORT_RB_NUM 8
#endif

#ifndef FC_PORT_LOCK
    #define FC_PORT_LOCK(port, rb_index, dir) ((void)0)
#endif

#ifndef FC_PORT_UNLOCK
    #define FC_PORT_UNLOCK(port, rb_index, dir) ((void)0)
#endif

    // 方向是从内核视角来说(高速部分)
    typedef enum
    {
        FC_PORT_DIR_IN = 0,                  // 慢速IO(通常是物理层)->环形缓冲->高速IO(通常是内核)
        FC_PORT_DIR_OUT = 1,                 // 高速IO(通常是内核)->环形缓冲->慢速IO(通常是物理层)
        FC_PORT_DIR_WRITE = FC_PORT_DIR_IN,  // 往缓冲区写数据
        FC_PORT_DIR_READ = FC_PORT_DIR_OUT,  // 从缓冲区读数据
    } fc_port_dir_t;

    typedef size_t (*fc_phy_io_t)(size_t rb_index, void *buf, size_t len);  // 返回值仅做保留,回调时保证从buf地址开始len长度一定是连续内存

    typedef struct _fc_port_t fc_port_t;
    struct _fc_port_t
    {
        // void *user;  // 自定义数据
        fc_fifo_t  *rb[PORT_RB_NUM];               // 环形缓冲
        const char *rb_name[PORT_RB_NUM];          // 每个rb缓冲区的名字
        fc_phy_io_t phy;                           // 物理IO接口
        uint8_t     rb_single_limit[PORT_RB_NUM];  // 单次读写限制(针对慢速IO而言),限制大小为缓冲区空间的1/(2^n),n=rb_single_limit

        uint8_t dir;  // 方向,用uint8_t而不是枚举(fc_port_dir_t)是为了明确空间大小
    };

    typedef struct _fc_port_rtt_t fc_port_rtt_t;
    struct _fc_port_rtt_t
    {
        char       id[16];         // 端口ID,用于RTT定位
        size_t     rb_array_size;  // rb指针数组的大小,恒等于PORT_RB_NUM
        fc_port_t *port_in;        // 输入端口
        fc_port_t *port_out;       // 输出端口
    };

    // 默认提供的写端口丢失钩子函数,提供默认弱实现可以在外面重写
    extern size_t fc_port_lose_hook(fc_port_t *port, size_t rb_index, const void *buf, size_t len);

    //+********************************* 面向对象 **********************************/
    // clang-format off
    extern void fc_port_init        (fc_port_t *port, fc_port_dir_t dir);
    extern void fc_port_catch_fifo  (fc_port_t *port, size_t rb_index, fc_fifo_t *fifo, const char *name, uint8_t single_limit);
    extern void fc_port_catch_phy   (fc_port_t *port, fc_phy_io_t phy);

    // 静态内存初始化一个port的环形缓冲区,包括静态内存分配构造,单次读写限制设置等
    #define fc_port_static_alloc_rb(port, rb_index, log2_size, name, single_limit)           \
        do                                                                                   \
        {                                                                                    \
            fc_fifo_t *__temp_fifo_ptr = NULL;                                               \
            fc_fifo_static_new_at(__temp_fifo_ptr, log2_size);                               \
            fc_port_catch_fifo((port), (rb_index), __temp_fifo_ptr, (name), (single_limit)); \
        } while (0)

    extern int fc_port_putc   (fc_port_t *port, size_t rb_index, int ch);
    extern int fc_port_puts   (fc_port_t *port, size_t rb_index, const char *str);
    extern int fc_port_write  (fc_port_t *port, size_t rb_index, const void *buf, size_t len);
    extern int fc_port_printf (fc_port_t *port, size_t rb_index, const char *fmt, ...);
    extern int fc_port_vprintf(fc_port_t *port, size_t rb_index, const char *fmt, va_list arp);  // fc_port_printf核心实现

    extern int   fc_port_getc(fc_port_t *port, size_t rb_index);                         // 阻塞式API
    extern char *fc_port_gets(fc_port_t *port, size_t rb_index, char *buf, size_t n);    // 不建议使用
    extern int   fc_port_read(fc_port_t *port, size_t rb_index, void *buf, size_t len);  // 读取数据并删除
    extern int   fc_port_peek(fc_port_t *port, size_t rb_index, void *buf, size_t len);  // 读取数据但不删除

    extern int  fc_port_used    (fc_port_t *port, size_t rb_index);            // 获取指定缓冲区的已用空间大小
    extern int  fc_port_free    (fc_port_t *port, size_t rb_index);            // 获取指定缓冲区的剩余空间大小
    // 以下API的行为取决于port的方向port->dir
    extern void fc_port_trigger (fc_port_t *port, size_t rb_index);            // 触发慢速IO
    extern void fc_port_end     (fc_port_t *port, size_t rb_index, int size);  // 慢速IO完成回调

    // clang-format on

    //+********************************* 提供一份格式化输出到fifo的API **********************************/

    // 这两个API使用都需要自行保证fifo的写操作线程安全
    extern int fc_fifo_printf(fc_fifo_t *fifo, const char *fmt, ...);
    extern int fc_fifo_vprintf(fc_fifo_t *fifo, const char *fmt, va_list arp);  // fc_fifo_printf核心实现,在utils/fc_fifo_vprintf.c中实现

    //+********************************* 默认实例化对象 **********************************/
    // 初始化标准输入输出
    extern void fc_default_port_init(void);

    // 声明输入输出对象
    extern fc_port_t fc_stdin;
    extern fc_port_t fc_stdout;

#ifndef FC_STDOUT_OBJ
    #define FC_STDOUT_OBJ (&fc_stdout)
#endif

#ifndef FC_STDOUT_RB_INDEX
    #define FC_STDOUT_RB_INDEX (0)
#endif

#ifndef FC_STDIN_OBJ
    #define FC_STDIN_OBJ (&fc_stdin)
#endif

#ifndef FC_STDIN_RB_INDEX
    #define FC_STDIN_RB_INDEX (0)
#endif

#define fc_stdin_phy_catch(func) fc_port_catch_phy(FC_STDIN_OBJ, (fc_phy_io_t)func)
#define fc_stdout_phy_catch(func) fc_port_catch_phy(FC_STDOUT_OBJ, (fc_phy_io_t)func)

    // clang-format off

    // 输出到fc_stdout
    #define fc_putchar(ch)       fc_port_putc   (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX, ch)
    #define fc_putc(ch)          fc_port_putc   (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX, ch)
    #define fc_puts(str)         fc_port_puts   (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX, str)
    #define fc_write(buf, len)   fc_port_write  (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX, buf, len)  // 要么全部写入,要么返回0
    #define fc_printf(fmt, ...)  fc_port_printf (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX, fmt, ##__VA_ARGS__)
    #define fc_vprintf(fmt, arp) fc_port_vprintf(FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX, fmt, arp)

    #define fc_out_trigger()    fc_port_trigger (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX)         // 触发发送
    #define fc_out_end(size)    fc_port_end     (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX, size)   // 发送完成处理
    #define fc_out_used()       fc_port_used    (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX)         // 缓冲区已用空间
    #define fc_out_free()       fc_port_free    (FC_STDOUT_OBJ, FC_STDOUT_RB_INDEX)         // 缓冲区剩余空间

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
   #define fc_getchar()        fc_port_getc(FC_STDIN_OBJ, FC_STDIN_RB_INDEX)            // 阻塞式API
   #define fc_getc()           fc_port_getc(FC_STDIN_OBJ, FC_STDIN_RB_INDEX)            // 阻塞式API
   #define fc_gets(buf, n)     fc_port_gets(FC_STDIN_OBJ, FC_STDIN_RB_INDEX, buf, n)    // 不建议使用
   #define fc_read(buf, len)   fc_port_read(FC_STDIN_OBJ, FC_STDIN_RB_INDEX, buf, len)  // 阻塞式API

   #define fc_in_trigger()  fc_port_trigger (FC_STDIN_OBJ, FC_STDIN_RB_INDEX)       // 触发接收
   #define fc_in_end(size)  fc_port_end     (FC_STDIN_OBJ, FC_STDIN_RB_INDEX, size) // 接收完成处理
   #define fc_in_used()     fc_port_used    (FC_STDIN_OBJ, FC_STDIN_RB_INDEX)       // 缓冲区已用空间
   #define fc_in_free()     fc_port_free    (FC_STDIN_OBJ, FC_STDIN_RB_INDEX)       // 缓冲区剩余空间

    // clang-format on

#ifdef __cplusplus
}
#endif

#endif  //\ _FC_PORT_H_
