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

// 已经隐式包含到了fc_foreach中
// FC_EXTERN(fc_auto_init_0);
// FC_EXTERN(fc_auto_init_1);
// FC_EXTERN(fc_auto_init_2);
// FC_EXTERN(fc_auto_init_3);

// 防止默认的空段警告
static void __void__func(void)
{
    (void)0;
}
INIT_EXPORT_ENV(__void__func);

// 可以无需显式调用
/**
 * @brief main 函数之前自动初始化
 *
 */
__attribute__((constructor)) fc_used void fc_section0_init_func(void)
{
    static bool init = false;
    if (init)
    {
        return;
    }
    init = true;

    fc_auto_init_func_t* ptr = NULL;
    fc_foreach(fc_auto_init_0, ptr)
    {
        if (ptr)
        {
            (*ptr)();
        }
    }
}

//+*********************************  **********************************/
// 以下需要显式调用
/**
 * @brief main函数配置完时钟之后自动初始化
 *
 */
void fc_section1_init_func(void)
{
    static bool init = false;
    if (init)
    {
        return;
    }
    init = true;

    fc_auto_init_func_t* ptr = NULL;
    fc_foreach(fc_auto_init_1, ptr)
    {
        if (ptr)
        {
            (*ptr)();
        }
    }
}

/**
 * @brief main函数配置执行完之后自动初始化,操作系统执行之前
 *
 */
void fc_section2_init_func(void)
{
    static bool init = false;
    if (init)
    {
        return;
    }
    init = true;

    fc_auto_init_func_t* ptr = NULL;
    fc_foreach(fc_auto_init_2, ptr)
    {
        if (ptr)
        {
            (*ptr)();
        }
    }
}

/**
 * @brief 操作系统创建的第一个任务内自动初始化
 *
 */
void fc_section3_init_func(void)
{
    static bool init = false;
    if (init)
    {
        return;
    }
    init = true;

    fc_auto_init_func_t* ptr = NULL;
    fc_foreach(fc_auto_init_3, ptr)
    {
        if (ptr)
        {
            (*ptr)();
        }
    }
}
