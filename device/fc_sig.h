/**
 * @file fc_signal.h
 * @author 独霸一方 (2696652257@qq.com)
 * @brief //> 二值信号库抽象层
 * @version 1.0
 * @date 2024-04-18
 *
 * @copyright Copyright (c) 2024
 *
 */

// > 单次包含宏定义
#ifndef _FC_SIGNAL_H_
#define _FC_SIGNAL_H_

#include <stdint.h>
#include <stdbool.h>

#include "fc_type.h"

#ifndef fc_dev_assert
    #define fc_dev_assert(exp) fc_assert(exp)
#endif  // !fc_dev_assert

#ifndef FC_ATOMIC_SCOPE
    #define FC_ATOMIC_SCOPE
#endif

//! 原子操作中不允许调用阻塞函数,也不能直接退出函数,建议支持嵌套
#ifndef SIGNAL_ATOMIC_ENTER
    #define SIGNAL_ATOMIC_ENTER(obj) \
        FC_ATOMIC_SCOPE              \
        {
#endif

#ifndef SIGNAL_ATOMIC_EXIT
    #define SIGNAL_ATOMIC_EXIT(obj) }
#endif

//! 状态机任务原子操作,用于保证心跳跟几个其他函数设置函数不能同时调用,一般可以手动约束保证原子性,可以不实现
#ifndef SIGNAL_HEART_ENTER
    #define SIGNAL_HEART_ENTER(obj) (void)0
#endif

#ifndef SIGNAL_HEART_EXIT
    #define SIGNAL_HEART_EXIT(obj) (void)0
#endif

typedef enum  // 信号由平台管理,反应当前实时状态
{
    FC_SIGNAL_STATE_INVALID = 0,
    FC_SIGNAL_STATE_VALID = 1,
    FC_SIGNAL_STATE_UNDEFINED = 2,
} fc_sig_state_t;

typedef enum  // ioctl的命令
{
    FC_SIGNAL_IOCTL_GET_STATE,
} fc_sig_ioctl_cmd_t;

typedef enum  // 信号事件由自身管理,用于状态机维护,用户不需要关心
{
    FC_SIGNAL_EVENT_FREE = 0,
    FC_SIGNAL_EVENT_EDGE_TRIGGER,  //! 单边沿触发,根据触发函数的返回值决定是单次触发还是持续触发
    FC_SIGNAL_EVENT_EDGE_BOTH_TRIGGER,
} fc_sig_event_t;

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief 这个"信号"(二值状态)是一个抽象概念,可以是一个传感器的状态,也可以是一个开关的状态甚至可以是一个函数的完成情况
     * 取决于平台操作函数的实现,用户只需要关心信号的状态,不需要关心具体的信号是什么,对接平台的时候当调用获取状态时,只需要返回对应的状态
     * 比如传感器可以是高电平有效也可以是低电平有效,但是调用获取状态的时候平台需要自行处理最终结果只返回逻辑上所需的"有效"或"无效"状态
     */

    typedef enum
    {
        FC_SIGNAL_TRIGGER_RET_END = 0,                                // 只有明确触发之后不再触发才返回结束
        FC_SIGNAL_TRIGGER_RET_AGAIN,                                  // 继续触发
        FC_SIGNAL_TRIGGER_RET_DEFAULT = FC_SIGNAL_TRIGGER_RET_AGAIN,  // 默认继续触发
    } fc_sig_trigger_ret_t;

    typedef struct __fc_sig_t fc_sig_t;                                                            // 结构体前置声明
    typedef fc_sig_trigger_ret_t (*fc_sig_trigger_t)(fc_sig_t *sig);                               //! 信号满足触发操作函数指针类型,返回真则结束触发,否则会持续在跳变时触发
    typedef fc_sig_state_t (*fc_sig_filter_t)(void *filter, fc_sig_t *sig, fc_sig_state_t state);  //! 信号状态滤波函数指针类型,返回滤波后的状态
    typedef fc_sig_state_t (*fc_sig_ioctl_t)(fc_sig_t *sig, fc_sig_ioctl_cmd_t cmd);

    struct __fc_sig_t
    {
        void          *user;   // 用户自定义数据
        fc_sig_ioctl_t ioctl;  // 平台操作函数指针

        void           *filter;
        fc_sig_filter_t filter_func;  // 信号状态滤波函数指针

        fc_sig_trigger_t func_trigger;  // 信号满足触发操作函数指针,要求为无阻塞

        fc_sig_state_t state_trigger;  // 触发状态
        fc_sig_state_t state_steady;   // 滤波后的状态(稳态)
        fc_sig_state_t state_last;     // 上一次状态

        fc_sig_event_t event;  // 信号状态机维护
    };

    //+********************************* 对外使用API **********************************/

    //> 初始化及绑定平台操作函数
    void fc_sig_init(fc_sig_t *sig, fc_sig_ioctl_t ioctl, void *user);

    // > 绑定滤波对象
    void fc_sig_catch_filter(fc_sig_t *sig, void *filter, fc_sig_filter_t filter_func);

    //> 信号状态机心跳,需要周期调用
    void fc_sig_heart(fc_sig_t *sig);

    //> 信号触发,从其他状态跳变到指定状态才触发,func_trigger可以设置为空表示自由态
    void fc_sig_trigger(fc_sig_t *sig, fc_sig_trigger_t func_trigger, fc_sig_state_t state_trigger);

    //> 获取信号滤波后的状态
    fc_sig_state_t fc_sig_state(fc_sig_t *sig);

    //> 直接调用ioctl接口
    fc_sig_state_t fc_sig_ioctl(fc_sig_t *sig, fc_sig_ioctl_cmd_t cmd);

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_SIGNAL_H_
