#include <string.h>
#include <stdlib.h>
#include "fc_transport.h"

// 自定义的分页信息
#define FC_DIVISION_HEAD "\033[?;"
#define FC_DIVISION_TAIL "m"
#define FC_DIVISION_DEFAULT "\033[?;0m"                        // 默认窗口0
#define FC_DIVISION_POSITION (sizeof(FC_DIVISION_HEAD) - 1)    // 分页信息的索引位置
#define FC_DIVISION_MIN_LEN (sizeof(FC_DIVISION_DEFAULT) - 1)  // 分页信息最小长度,包括前缀和后缀
#define FC_DIVISION_MAX_LEN (16)                               // 分页信息最大长度+1,包括前缀和后缀

#ifndef EOF
    #define EOF (-1)
#endif

// 判断字符数组是否全为数字
static bool is_all_digits(const char* arr, int length)
{
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
 * @param port      端口对象
 * @param out 分发函数
 * @param end 结束函数,返回true表示本次结束,返回false表示还在接收
 */
void fc_receiver_init(fc_receiver_t* receiver, fc_port_t* port, fc_receiver_out_t out, fc_receiver_end_t end)
{
    fc_stdio_assert(NULL != receiver);
    fc_stdio_assert(NULL != port);

    memset(receiver, 0, sizeof(fc_receiver_t));  // 清空
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
void fc_receiver_monitor(fc_receiver_t* receiver)
{
    fc_stdio_assert(NULL != receiver->port);
    fc_stdio_assert(NULL != receiver->out);

    size_t len_total = fc_port_available(receiver->port);
    if (len_total <= 0)
    {
        return;  // 没有数据直接返回
    }

    size_t      len = 0;
    char*       p_start = NULL;
    char*       p_end = NULL;
    const char* head = FC_DIVISION_HEAD;  // 分页信息头部

    do
    {
        len_total = fc_port_available(receiver->port);  // 更新现存数据量
        p_start = (char*)fc_fifo_linear_read_setup(receiver->port, &len);
        p_end = memchr(p_start, head[0], len);  // 查找是否可能存在分页信息

        if (NULL == p_end)  // 不存在分页信息
        {
            // 分页信息不存在,直接分发数据
            receiver->out(receiver->index, p_start, len);
            fc_fifo_linear_read_done(receiver->port, len);
        }
        else  // 可能存在分页信息
        {
            // 先把分页信息之前的数据分发
            receiver->out(receiver->index, p_start, p_end - p_start);
            fc_fifo_linear_read_done(receiver->port, p_end - p_start);

            len_total = fc_port_available(receiver->port);  // 更新现存数据量
            char buff[FC_DIVISION_MAX_LEN] = {0};
            len = fc_fifo_peek(receiver->port, buff, (sizeof(buff) - 1) > len_total ? len_total : (sizeof(buff) - 1));

            p_start = &buff[0];                                    // 分页信息开始位置
            if (0 != memcmp(p_start, head, FC_DIVISION_POSITION))  // 分页信息头部
            {
                receiver->out(receiver->index, buff, 1);  // 将这个可能为头的字符分发出去
                fc_fifo_read(receiver->port, 1);          // 弹出这个字符
                continue;                                 // 继续查找分页信息
            }

            p_end = strstr(buff + FC_DIVISION_POSITION, FC_DIVISION_TAIL);  // 找到分页尾部
            if (p_end)                                                      // 找到分页信息头部和尾部
            {
                if ((p_end - p_start) <= FC_DIVISION_POSITION)
                {
                    // 根本没有端口号信息,直接当做数据分发
                    receiver->out(receiver->index, p_start, p_end - p_start + strlen(FC_DIVISION_TAIL));
                    fc_fifo_read(receiver->port, p_end - p_start + strlen(FC_DIVISION_TAIL));  // 弹出数据
                    continue;                                                                  // 继续查找分页信息
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
                    receiver->out(receiver->index, p_start, p_end - p_start + strlen(FC_DIVISION_TAIL));
                }

                fc_fifo_read(receiver->port, buff, p_end - p_start + strlen(FC_DIVISION_TAIL));  // 不论分页码信息解析是否成功,都要弹出帧尾(包含)之前的数据
            }
            else if (!p_end && len < (sizeof(buff) - 1))  // 可能数据不完整导致的未找到分页信息尾部
            {
                if (receiver->end)
                {
                    if (receiver->end(receiver))
                    {
                        // 已经完成数据接收,表明仅仅是数据刚好存在分页信息头
                        receiver->out(receiver->index, p_start, len);  // 当做数据直接分发
                        fc_fifo_read(receiver->port, len);             // 弹出数据
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
                    fc_fifo_read(receiver->port, len);
                }
            }
            else  // 数据长度足够,但是没有匹配到结束信息,直接当数据分发
            {
                receiver->out(receiver->index, p_start, len);
                fc_fifo_read(receiver->port, len);
            }
        }
    } while (fc_port_available(receiver->port) > 0);  // 继续处理数据
}

//+********************************* sender **********************************/

/**
 * @brief 初始化发送器
 *
 * @param sender
 * @param port
 */
void fc_sender_init(fc_sender_t* sender, fc_port_t* port)
{
    fc_stdio_assert(NULL != sender);
    fc_stdio_assert(NULL != port);

    memset(sender, 0, sizeof(fc_sender_t));  // 清空
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
bool fc_sender_switch(fc_sender_t* sender, size_t index)
{
    fc_stdio_assert(NULL != sender);
    fc_stdio_assert(NULL != sender->port);

    if (sender->index != index)
    {
        sender->index = index;
        char buff[FC_DIVISION_MAX_LEN] = {0};
        strcpy(buff, FC_DIVISION_HEAD);                // 分页信息前缀
        itoa(buff + FC_DIVISION_POSITION, index, 10);  // 转换为字符串
        strcat(buff, FC_DIVISION_TAIL);                // 分页信息后缀
        size_t len = strlen(buff);

        return (fc_port_write(sender->port, buff, len) == len);  // 写入分页信息
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
int fc_sender_putc(fc_sender_t* sender, size_t index, int ch)
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
int fc_sender_puts(fc_sender_t* sender, size_t index, const char* str)
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
int fc_sender_write(fc_sender_t* sender, size_t index, const void* buf, size_t len)
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
int fc_sender_printf(fc_sender_t* sender, size_t index, const char* fmt, ...)
{
    int ret = EOF;
    if (fc_sender_switch(sender, index))
    {
        va_list arp;
        va_start(arp, fmt);
        FC_STDIO_ATOMIC
        {
            ret = fc_port_vprintf(port, fmt, arp);
        }
        va_end(arp);
    }
    return ret;
}
