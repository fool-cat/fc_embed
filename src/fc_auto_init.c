/**
 * @file fc_auto_init.c
 * @author fool_cat (2696652257@qq.com)
 * @brief 自带的自动初始化函数
 * @version 1.0
 * @date 2025-02-23
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdbool.h>

#include "fc_auto_init.h"

// 防止默认的空段警告
static void __void__func(void)
{
    (void)0;
}
INIT_EXPORT_ENV(__void__func);

// 已经隐式包含到了fc_foreach中
// FC_EXTERN(fc_section_0);
// FC_EXTERN(fc_section_1);
// FC_EXTERN(fc_section_2);
// FC_EXTERN(fc_section_3);

/**
 * @brief 保证函数只执行一次
 *
 */
#define ONCE_RUNNING()        \
    static bool init = false; \
    if (init)                 \
    {                         \
        return;               \
    }                         \
    init = true;

/**
 * @brief 根据优先级顺序执行
 *
 */
#define ORDER_SECTION_RUN(section_name)                                \
    do                                                                 \
    {                                                                  \
        size_t               order_min = 0;                            \
        size_t               order_max = 0;                            \
        size_t               count_total = 0;                          \
        fc_auto_init_elem_t* ptr = NULL;                               \
        fc_foreach(section_name, ptr)                                  \
        {                                                              \
            if (count_total == 0)                                      \
            {                                                          \
                order_min = ptr->order;                                \
                order_max = ptr->order;                                \
            }                                                          \
            if (ptr->order < order_min)                                \
            {                                                          \
                order_min = ptr->order;                                \
            }                                                          \
            if (ptr->order > order_max)                                \
            {                                                          \
                order_max = ptr->order;                                \
            }                                                          \
            count_total++;                                             \
        }                                                              \
        if (count_total == 0)                                          \
        {                                                              \
            break;                                                     \
        }                                                              \
        size_t order_now = order_min;                                  \
        size_t count_now = 0;                                          \
        for (;;)                                                       \
        {                                                              \
            fc_foreach(section_name, ptr)                              \
            {                                                          \
                if (ptr->order == order_now)                           \
                {                                                      \
                    ptr->func();                                       \
                    count_now++;                                       \
                }                                                      \
            }                                                          \
            if (count_now >= count_total)                              \
            {                                                          \
                break;                                                 \
            }                                                          \
            size_t order_next = order_max;                             \
            fc_foreach(section_name, ptr)                              \
            {                                                          \
                if (ptr->order > order_now && ptr->order < order_next) \
                {                                                      \
                    order_next = ptr->order;                           \
                }                                                      \
            }                                                          \
            order_now = order_next;                                    \
        }                                                              \
    } while (0)

// 可以无需显式调用
/**
 * @brief main 函数之前自动初始化
 *
 */
__attribute__((constructor)) fc_used void fc_section_init_env(void)
{
    ONCE_RUNNING();

    ORDER_SECTION_RUN(fc_section_0);
}

//+*********************************  **********************************/
// 以下需要显式调用
/**
 * @brief main函数配置完时钟之后自动初始化
 *
 */
void fc_section_init_clock(void)
{
    ONCE_RUNNING();

    ORDER_SECTION_RUN(fc_section_1);
}

/**
 * @brief main函数配置执行完之后自动初始化,操作系统执行之前
 *
 */
void fc_section_init_device(void)
{
    ONCE_RUNNING();

    ORDER_SECTION_RUN(fc_section_2);
}

/**
 * @brief 操作系统创建的第一个任务内自动初始化
 *
 */
void fc_section_init_app(void)
{
    ONCE_RUNNING();

    ORDER_SECTION_RUN(fc_section_3);
}
