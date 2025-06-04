/**
 * @file fc_transport.c
 * @author fool_cat (2696652257@qq.com)
 * @brief 默认"\033[?;" "<num>" "m"作为分页信息,长度定为10个字符其能支持的端口号范围为4位数字(0 ~ 9999)
 * @version 1.0
 * @date 2025-04-24
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <string.h>
#include <stdlib.h>
#include "fc_transport.h"

// 自定义的分页信息
#ifndef FC_DIVISION_NUM_MAX_LEN
    #define FC_DIVISION_NUM_MAX_LEN 4  // 分页信息中端口号的最大长度("9999")
#endif

#ifndef FC_DIVISION_NUM_MAX
    #define FC_DIVISION_NUM_MAX (9999)  // 分页信息中端口号的最大值,即4位数字的最大值
#endif

#define FC_DIVISION_HEAD "\033[?;"
#define FC_DIVISION_TAIL "m"
#define FC_DIVISION_DEFAULT "\033[?;0m"                                                    // 默认窗口0
#define FC_DIVISION_POSITION (sizeof(FC_DIVISION_HEAD) - 1)                                // 分页信息的索引位置
#define FC_DIVISION_MIN_BUF_LEN (sizeof(FC_DIVISION_DEFAULT))                              // 分页信息最小长度,包括前缀和后缀个一个字节的端口号
#define FC_DIVISION_MAX_BUF_LEN (FC_DIVISION_MIN_BUF_LEN + (FC_DIVISION_NUM_MAX_LEN - 1))  // 包含'\0'的分页信息最大长度,即"\033[?;9999m"的长度,方便使用字符串API

#ifndef EOF
    #define EOF (-1)
#endif

// 判断字符数组是否全为数字
static bool is_all_digits(const char *arr, int length)
{
    if (length <= 0)
    {
        return false;
    }

    for (int i = 0; i < length; i++)
    {
        if (arr[i] < '0' || arr[i] > '9')
        {
            return false;
        }
    }
    return true;
}

//+********************************* receiver **********************************/

/**
 * @brief 初始化接收器
 *
 * @param receiver  接收器对象
 * @param port      一般建议是输入方向的port,不做强制
 * @param out 分发函数
 * @param end 结束函数,返回true表示本次结束,返回false表示还在接收
 */
void fc_receiver_init(fc_receiver_t *receiver, fc_port_t *port, fc_receiver_out_t out, fc_receiver_end_t end)
{
    fc_stdio_assert(NULL != receiver);
    fc_stdio_assert(NULL != port);

    memset(receiver, 0, sizeof(fc_receiver_t));
    receiver->port = port;
    receiver->index = 0;  // 默认窗口0
    receiver->out = out;
    receiver->end = end;
}

/**
 * @brief 接收器监控函数,需要保证线程安全
 *
 * @param receiver
 */
void fc_receiver_monitor(fc_receiver_t *receiver)
{
    fc_stdio_assert(NULL != receiver->port);
    fc_stdio_assert(NULL != receiver->out);  // 先绑定了分发函数才能调用

    fc_fifo_t *rb = receiver->port->rb;  // 环形缓冲区
    size_t     len_total = fc_fifo_get_used(rb);
    if (len_total <= 0)
    {
        return;  // 没有数据直接返回
    }

    size_t      len = 0;
    char       *p_start = NULL;
    char       *p_end = NULL;
    const char *head = FC_DIVISION_HEAD;  // 分页信息头部

    do
    {
        len_total = fc_fifo_get_used(rb);  // 更新现存数据量
        p_start = (char *)fc_fifo_linear_read_setup(rb, &len);
        p_end = memchr(p_start, head[0], len);  // 查找是否可能存在分页信息

        if (NULL == p_end)  // 不存在分页信息
        {
            // 分页信息不存在,直接分发数据
            receiver->out(receiver->index, p_start, len);
            fc_fifo_linear_read_done(rb, len);
        }
        else  // 可能存在分页信息
        {
            // 先把分页信息之前的数据分发
            if (p_end != p_start)
            {
                receiver->out(receiver->index, p_start, p_end - p_start);
            }
            fc_fifo_linear_read_done(rb, p_end - p_start);

            len_total = fc_fifo_get_used(rb);          // 更新现存数据量
            char buff[FC_DIVISION_MAX_BUF_LEN] = {0};  // 多一个'\0'字符,方便字符串处理
            len = fc_fifo_peek(rb, buff, (sizeof(buff) - 1) > len_total ? len_total : (sizeof(buff) - 1));

            p_start = &buff[0];                                    // 分页信息开始位置
            if (0 != memcmp(p_start, head, FC_DIVISION_POSITION))  // 分页信息头部
            {
                receiver->out(receiver->index, buff, 1);  // 将这个可能为头的字符分发出去
                fc_fifo_drop(rb, 1);                      // 弹出这个字符
                continue;                                 // 继续查找分页信息
            }

            p_end = strstr(buff + FC_DIVISION_POSITION, FC_DIVISION_TAIL);  // 找到分页尾部
            if (p_end)                                                      // 找到分页信息头部和尾部
            {
                if ((p_end - p_start) <= FC_DIVISION_POSITION)
                {
                    // 根本没有端口号信息,直接当做数据分发
                    receiver->out(receiver->index, p_start, p_end - p_start + (sizeof(FC_DIVISION_TAIL) - 1));
                    fc_fifo_drop(rb, p_end - p_start + (sizeof(FC_DIVISION_TAIL) - 1));  // 弹出数据
                    continue;                                                            // 继续查找分页信息
                }

                // 判断索引开始到尾部中间的所有字符是否全为数字
                if (is_all_digits(p_start + FC_DIVISION_POSITION, p_end - p_start - FC_DIVISION_POSITION))
                {
                    // 更新当前端口号
                    *p_end = '\0';  // 将分页信息结束符替换为'\0'方便后续字符串处理
                    receiver->index = atoi(p_start + FC_DIVISION_POSITION);
                }
                else
                {  // 不完全符合分页格式,当数据分发
                    receiver->out(receiver->index, p_start, p_end - p_start + (sizeof(FC_DIVISION_TAIL) - 1));
                }

                fc_fifo_drop(rb, p_end - p_start + (sizeof(FC_DIVISION_TAIL) - 1));  // 不论分页码信息解析是否成功,都要弹出帧尾(包含)之前的数据
            }
            else if (len < (FC_DIVISION_MAX_BUF_LEN - 1))  // 可能数据不完整导致的未找到分页信息尾部
            {
                if (receiver->end)
                {
                    if (receiver->end(receiver))
                    {
                        // 已经完成数据接收,表明仅仅是数据刚好存在分页信息头
                        receiver->out(receiver->index, p_start, len);  // 当做数据直接分发
                        fc_fifo_drop(rb, len);                         // 弹出数据
                    }
                    else
                    {
                        break;  // 数据还没接收完,退出这次分发
                    }
                }
                else
                {
                    // 没有绑定结束函数当数据已经接收完成,没有匹配到结束信息,直接当做数据分发
                    receiver->out(receiver->index, p_start, len);
                    fc_fifo_drop(rb, len);
                }
            }
            else  // 数据长度足够,但是没有匹配到结束信息,直接当数据分发
            {
                receiver->out(receiver->index, p_start, len);
                fc_fifo_drop(rb, len);
            }
        }
    } while (fc_fifo_get_used(rb) > 0);  // 继续处理数据
}

//+********************************* sender **********************************/

/**
 * @brief 初始化发送器
 *
 * @param sender
 * @param port // 一般建议是输出方向的port,不做强制
 */
void fc_sender_init(fc_sender_t *sender, fc_port_t *port)
{
    fc_stdio_assert(NULL != sender);
    fc_stdio_assert(NULL != port);

    memset(sender, 0, sizeof(fc_sender_t));
    sender->port = port;
    sender->index = 0;  // 默认窗口0

    fc_port_write(sender->port, FC_DIVISION_DEFAULT, sizeof(FC_DIVISION_DEFAULT) - 1);  // 写入默认窗口
}

/**
 * @brief 切换窗口
 *
 * @param sender
 * @param index  窗口号
 * @return true
 * @return false
 */
bool fc_sender_switch(fc_sender_t *sender, size_t index)
{
    fc_stdio_assert(NULL != sender);
    fc_stdio_assert(NULL != sender->port);
    fc_stdio_assert(index <= FC_DIVISION_NUM_MAX);  // 确保索引在范围内

    if (sender->index != index)
    {
        sender->index = index;
        size_t num = index;
        char   buff[FC_DIVISION_MAX_BUF_LEN] = {0};
        int    pos = FC_DIVISION_POSITION;

        // 1. 拷贝前缀
        for (int i = 0; i < FC_DIVISION_POSITION; i++)
        {
            buff[i] = FC_DIVISION_HEAD[i];
        }

        // 2. 计算数字位数并写入数字部分
        if (num < 10)
        {
            buff[pos++] = '0' + num;  // 只有一位数字
        }
        else
        {
            // 从最高位开始计算，避免前导零
            size_t divisor = 10;
            // 计算最大除数
            for (size_t i = 0; i < (FC_DIVISION_NUM_MAX_LEN - 1); i++)
            {
                // if (num >= divisor && num < (divisor * 10))
                if (num < (divisor * 10))  // 优化条件
                {
                    break;
                }
                divisor *= 10;
            }

            int digit = 0;
            // 逐位写入数字
            do
            {
                digit = (size_t)num / (size_t)divisor;  // 整数除法,丢掉余数部分
                buff[pos++] = '0' + digit;
                num -= digit * divisor;
                if (num < 10)
                {
                    while (divisor > 10)
                    {
                        divisor /= 10;      // 除数逐渐减小,直到只剩下个位
                        buff[pos++] = '0';  // 写入中间0
                    }
                    buff[pos++] = '0' + num;  // 写入最后一位数字
                    break;
                }
                divisor /= 10;
            } while (divisor > 0);
        }

        // 3. 拷贝后缀
        for (int i = 0; i < (sizeof(FC_DIVISION_TAIL) - 1); i++)
        {
            buff[pos++] = FC_DIVISION_TAIL[i];
        }

        return (fc_port_write(sender->port, buff, pos) == pos);
    }

    return true;
}

/**
 * @brief 发送字符
 *
 * @param sender
 * @param index
 * @param ch
 * @return int 发送成功返回ch,失败返回负值
 */
int fc_sender_putc(fc_sender_t *sender, size_t index, int ch)
{
    int ret = EOF;
    if (fc_sender_switch(sender, index))
    {
        ret = fc_port_putc(sender->port, ch);
    }
    return ret;
}

/**
 * @brief 发送字符串
 *
 * @param sender
 * @param index
 * @param str
 * @return int 发送成功返回发送的字节数,失败返回负值
 */
int fc_sender_puts(fc_sender_t *sender, size_t index, const char *str)
{
    int ret = EOF;
    if (fc_sender_switch(sender, index))
    {
        ret = fc_port_puts(sender->port, str);
    }
    return ret;
}

/**
 * @brief 从指定地址发送指定长度的数据
 *
 * @param sender
 * @param index
 * @param buf
 * @param len
 * @return size_t 发送成功返回发送的字节数,失败返回负值
 */
int fc_sender_write(fc_sender_t *sender, size_t index, const void *buf, size_t len)
{
    int write_size = EOF;
    if (fc_sender_switch(sender, index))
    {
        write_size = fc_port_write(sender->port, buf, len);
    }
    return write_size;
}

/**
 * @brief 发送格式化字符串
 *
 * @param sender
 * @param index
 * @param fmt
 * @param ...
 * @return int 发送成功返回发送的字节数,失败返回负值
 */
int fc_sender_printf(fc_sender_t *sender, size_t index, const char *fmt, ...)
{
    int ret = EOF;
    if (fc_sender_switch(sender, index))
    {
        va_list arp;
        va_start(arp, fmt);
        FC_STDIO_ATOMIC
        {
            ret = fc_port_vprintf(sender->port, fmt, arp);
        }
        va_end(arp);
    }
    return ret;
}

//+*********************************  **********************************/

fc_receiver_t fc_receiver;  // 接收器对象
fc_sender_t   fc_sender;    // 发送器对象

//+********************************* 自动注册初始化 **********************************/
#if USE_FC_AUTO_INIT
    #include "fc_auto_init.h"
static void _fc_transport_auto_init(void)
{
    fc_receiver_init(&fc_receiver, &fc_stdin, NULL, NULL);  // 初始化接收器
    fc_sender_init(&fc_sender, &fc_stdout);                 // 初始化发送器
}
INIT_EXPORT_ENV(_fc_transport_auto_init, 110);  // 等级比默认的1000优先级更高,但是低于fc_stdio_init,纯数据结构无外部依赖
#endif
