/**
 * @file fc_stp_base.h
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2024-07-27
 *
 * @copyright Copyright (c) 2024
 *
 */

// > 单次包含宏定义
#ifndef __FC_STEPPER_H_
#define __FC_STEPPER_H_

/** fc_stp_base component version string — keep in sync with Cversion in fool_cat.fc_embed.pdsc */
#define FC_STP_BASE_VERSION "2.0.0"

#include <stddef.h>

#include "fc_config.h"
#include "../core/fc_arch.h"

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef fc_dev_assert
    #define fc_dev_assert(exp) fc_assert(exp)
#endif

#ifndef FC_STP_ABS
    #define FC_STP_ABS(x) (((x) > 0) ? (x) : -(x))
#endif

//! 原子操作中不允许调用阻塞函数,也不能直接退出函数,建议支持嵌套
#ifndef STP_ATOMIC_ENTER
    #define STP_ATOMIC_ENTER(obj) \
        FC_ATOMIC_SCOPE           \
        {
#endif

#ifndef STP_ATOMIC_EXIT
    #define STP_ATOMIC_EXIT(obj) }
#endif

//! 电机心跳原子操作,用于保证心跳跟几个其他函数设置函数不能同时调用,一般可以手动约束保证原子性,可以不实现
#ifndef STP_HEART_ENTER
    #define STP_HEART_ENTER(obj) (void)0
#endif

#ifndef STP_HEART_EXIT
    #define STP_HEART_EXIT(obj) (void)0
#endif

    // 枚举重定义必须包含对应枚举的原型
    typedef enum _fc_stp_ioctl_cmd_t fc_stp_ioctl_cmd_t;  // 电机底层控制命令枚举
    typedef enum _fc_stp_move_t      fc_stp_move_t;       // 电机移动方式枚举
    typedef enum _fc_stp_calc_t      fc_stp_calc_t;       // 电机计算方式枚举

    typedef struct _fc_stp_base_t fc_stp_base_t;  // 电机对象前置声明

    // 电机底层控制命令
    enum _fc_stp_ioctl_cmd_t
    {
        // 动作信号沿,发出动作信号沿必须发出动作脉冲沿
        FC_STP_IOCTL_EDGE_FREE = 0,  // 发出自由信号沿
        FC_STP_IOCTL_EDGE_ACTION,    // 发出动作信号沿

        // 枚举旋转的方向,正面对轴方向观测
        FC_STP_IOCTL_DIR_POSITIVE,  // 通常设置顺时针方向为正方向
        FC_STP_IOCTL_DIR_NEGATIVE,  // 确保设置为正负方向对应的物理方向

        FC_STP_IOCTL_ENABLE,   // 使能
        FC_STP_IOCTL_DISABLE,  // 禁用

        // 纯逻辑概念
        FC_STP_IOCTL_START,  // 开始运行,可以在这里面开启定时器
        FC_STP_IOCTL_END,    // 结束运行,可以在这里面关闭定时器

        // ... 锁定.降低电流等...
    };

    // 电机移动方式,绝对移动/相对移动
    enum _fc_stp_move_t
    {
        FC_STP_MOVE_TO = 0,   // 绝对移动
        FC_STP_MOVE_RELATIVE  // 相对移动
    };

    // 曲线计算阶段
    enum _fc_stp_calc_t
    {
        FC_STP_CLAC_START = 0,  // 在运行之前准备曲线数据
        FC_STP_CLAC_NEXT,       // 每次运行计算下一次的速度
        FC_STP_CLAC_DEC,        // 在运行过程中主动触发减速
        FC_STP_CLAC_END,        // 在运行之后清理曲线数据
        FC_STP_CLAC_CHANGE,     // 在运行过程中变更到达位置,需要重新计算曲线,暂时不支持
    };

    //> ARM架构少于等于4个参数是直接使用寄存器的,速度更快,所以这里的参数尽量少于等于4个
    // 平台实现对接函数指针类型
    typedef void (*fc_stp_ioctl_t)(fc_stp_base_t *stp, fc_stp_ioctl_cmd_t cmd);                     // 函数指针类型,负责基础的IO逻辑操作等
    typedef void (*fc_stp_freq_set_t)(fc_stp_base_t *stp, int32_t freq, size_t *scale);             // 函数指针类型,设置脉冲速度到传入频率
    typedef void (*fc_curve_func_t)(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next);  // 函数指针类型,计算下一次的速度,用函数指针的形式开放出去用于匹配更多速度曲线实现

    struct _fc_stp_base_t
    {
        // 物理层对接
        void             *user;      // 用户数据指针,可以用于存储用户自定义数据
        fc_stp_ioctl_t    ioctl;     // 平台操作函数指针
        fc_stp_freq_set_t freq_set;  // 设置发出的脉冲速度(就是改变当前速度)

        // 曲线参数相关参数,通过void*指针实现泛型
        void           *curve;  // 曲线对象
        fc_curve_func_t curve_func;

        // 位置相关参数
        // int32_t a_now;            // 当前加速度,放到曲线中处理
        int32_t v_now;            // 当前速度,速度没有正负之分,方向在ioctl中根据目标位置和当前位置进行设置
        int32_t s_now;            // 当前位置
        int32_t s_target;         // 需要到达的位置
        int32_t s_last;           // 上一次的位置
        int32_t s_each_increase;  // 每次增加的位置,使用这个参数保证速度为无符号值

        size_t pulse_scale;  // 脉冲比率,多少次心跳发出一次脉冲
        size_t heart_index;  // 心跳索引,每次心跳增加一次,到达pulse_scale时从头开始

        // 曲线平台段优化:
        // 当速度已经进入平台区且到减速点之前都不会变化时,
        // 允许曲线实现把未来若干步的“无需重算”步数写到这里.
        // base层会在这些步内跳过curve_func(NEXT)和freq_set调用.
        size_t curve_hold_steps;
    };

    void fc_stp_base_init(fc_stp_base_t *stp, fc_stp_ioctl_t ioctl, fc_stp_freq_set_t freq_set, void *user);  // 初始化电机,绑定平台操作函数

    void fc_stp_base_catch_curve(fc_stp_base_t *stp, void *curve, fc_curve_func_t curve_func);

    bool fc_stp_base_heart(fc_stp_base_t *stp);

    bool fc_stp_base_move(fc_stp_base_t *stp, fc_stp_move_t move_mode, int32_t s);

    void fc_stp_base_stop(fc_stp_base_t *stp, bool safe_stop);

    int32_t fc_stp_base_get_pos(fc_stp_base_t *stp);

    bool fc_stp_base_arrived(fc_stp_base_t *stp);

    bool fc_stp_base_rectify_pos(fc_stp_base_t *stp, fc_stp_move_t move_mode, int32_t s);  // 矫正位置,只能在停止状态下使用

    void fc_stp_base_shift_pos(fc_stp_base_t *stp, int32_t s);  // 位置偏移,任何时候都能使用

    void fc_stp_base_ioctl(fc_stp_base_t *stp, fc_stp_ioctl_cmd_t cmd);

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ __FC_STEPPER_H_
