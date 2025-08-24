/**
 * @file fc_trans.h
 * @author fool_cat (2696652257@qq.com)
 * @brief 提供一种简单的分页机制实现单个通道聚合传输解析,接收器和发送器需要配合使用
 * @version 1.0
 * @date 2025-04-05
 *
 * @copyright Copyright (c) 2025
 *
 */

// > 单次包含宏定义
#ifndef _FC_TRANS_H_
#define _FC_TRANS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "fc_fifo.h"

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

    //+********************************* receiver **********************************/

    typedef struct _fc_receiver_t fc_receiver_t;  // 前置声明

    typedef size_t (*fc_receiver_out_t)(size_t index, const char *buf, size_t len);  // 返回值仅做保留
    typedef bool (*fc_receiver_end_t)(fc_receiver_t *receiver);                      // 接收结束回调函数,返回true表示结束,返回false表示继续接收

    struct _fc_receiver_t
    {
        fc_fifo_t        *fifo;   // fifo指针
        size_t            index;  // 当前窗口索引
        fc_receiver_out_t out;    // 数据分发处理函数
        fc_receiver_end_t end;    // 如果出现不完整的分页符,根据此函数返回值决定是否将不完整的分页符当数据处理

        // void *user;  // 预留用户个人数据
    };

    extern void fc_receiver_init(fc_receiver_t *receiver, fc_fifo_t *fifo);
    extern void fc_receiver_catch(fc_receiver_t *receiver, fc_receiver_out_t out, fc_receiver_end_t end);  // 设置数据分发和结束函数
    extern void fc_receiver_monitor(fc_receiver_t *receiver);                                              // 需要保证线程安全

    //+********************************* sender **********************************/

    typedef struct _fc_sender_t fc_sender_t;
    struct _fc_sender_t
    {
        fc_fifo_t *fifo;   // fifo指针
        size_t     index;  // 当前窗口索引

        // void *user;  // 预留用户个人数据
    };

    extern void fc_sender_init(fc_sender_t *sender, fc_fifo_t *fifo);
    extern bool fc_sender_switch(fc_sender_t *sender, size_t index);  // 切换窗口,"0~9"端口非常高效

    extern int fc_sender_putc(fc_sender_t *sender, size_t index, int ch);
    extern int fc_sender_puts(fc_sender_t *sender, size_t index, const char *str);
    extern int fc_sender_write(fc_sender_t *sender, size_t index, const void *buf, size_t len);
    extern int fc_sender_printf(fc_sender_t *sender, size_t index, const char *fmt, ...);
    extern int fc_sender_vprintf(fc_sender_t *sender, size_t index, const char *fmt, va_list arp);

    //+********************************* 默认实例化对象 **********************************/
    extern fc_receiver_t fc_receiver;  // 接收器对象,绑定fc_stdin
    extern fc_sender_t   fc_sender;    // 发送器对象,绑定fc_stdout

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_TRANS_H_
