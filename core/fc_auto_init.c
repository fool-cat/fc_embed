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

// 已经隐式包含到了section_info中
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
 * @brief 根据优先级顺序执行(从小到大)
 *
 */
#define ORDER_SECTION_RUN(section_name)                                    \
    do                                                                     \
    {                                                                      \
        fc_auto_init_elem_t *ptr = NULL;                                   \
        size_t               elem_count = 0;                               \
        section_info(section_name, ptr, elem_count);                       \
        size_t order_min = 0;                                              \
        size_t order_max = 0;                                              \
        size_t count_total = 0;                                            \
        for (size_t i = 0; i < elem_count; i++)                            \
        {                                                                  \
            if (count_total == 0)                                          \
            {                                                              \
                order_min = ptr[i].order;                                  \
                order_max = ptr[i].order;                                  \
            }                                                              \
            if (ptr[i].order < order_min)                                  \
            {                                                              \
                order_min = ptr[i].order;                                  \
            }                                                              \
            if (ptr[i].order > order_max)                                  \
            {                                                              \
                order_max = ptr[i].order;                                  \
            }                                                              \
            count_total++;                                                 \
        }                                                                  \
        if (count_total == 0)                                              \
        {                                                                  \
            break;                                                         \
        }                                                                  \
        size_t order_now = order_min;                                      \
        size_t count_now = 0;                                              \
        for (;;)                                                           \
        {                                                                  \
            for (size_t i = 0; i < elem_count; i++)                        \
            {                                                              \
                if (ptr[i].order == order_now)                             \
                {                                                          \
                    if (ptr[i].func)                                       \
                    {                                                      \
                        (ptr[i].func)();                                   \
                    }                                                      \
                    count_now++;                                           \
                }                                                          \
            }                                                              \
            if (count_now >= count_total)                                  \
            {                                                              \
                break;                                                     \
            }                                                              \
            size_t order_next = order_max;                                 \
            for (size_t i = 0; i < elem_count; i++)                        \
            {                                                              \
                if (ptr[i].order > order_now && ptr[i].order < order_next) \
                {                                                          \
                    order_next = ptr[i].order;                             \
                }                                                          \
            }                                                              \
            order_now = order_next;                                        \
        }                                                                  \
    } while (0)

/**
 * @brief main 函数之前自动初始化
 *
 */
#if USE_FC_AUTO_INIT
// 可以无需显式调用
__attribute__((constructor)) fc_used void fc_auto_init_env(void)
#else
void fc_auto_init_env(void)
#endif
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
void fc_auto_init_clock(void)
{
    ONCE_RUNNING();

    ORDER_SECTION_RUN(fc_section_1);
}

/**
 * @brief main函数配置执行完之后自动初始化,操作系统执行之前
 *
 */
void fc_auto_init_device(void)
{
    ONCE_RUNNING();

    ORDER_SECTION_RUN(fc_section_2);
}

/**
 * @brief 操作系统创建的第一个任务内自动初始化
 *
 */
void fc_auto_init_app(void)
{
    ONCE_RUNNING();

    ORDER_SECTION_RUN(fc_section_3);
}

//+********************************* 觉得不够还可以继续加,请参考头文件和本文件 **********************************/

// 防止默认的空段警告
static void __void__func(void)
{
    (void)0;
}
INIT_EXPORT_ENV(__void__func);
INIT_EXPORT_CLOCK(__void__func);
INIT_EXPORT_DEVICE(__void__func);
INIT_EXPORT_APP(__void__func);

/**
 *自定义类型请参考头文件实现
 *
 * @brief 如果有添加,请不要直接加在本文件和对应的头文件中,请自行添加新的文件实现
 *
 *
 */
