#include "fc_sig.h"
#include "fc_sig_filter.h"

void fc_sig_init(fc_sig_t *sig, fc_sig_ioctl_t ioctl, void *user)
{
    fc_dev_assert(sig != NULL);
    fc_dev_assert(ioctl != NULL);
    // fc_dev_assert(name != NULL);//> name可以为空

    SIGNAL_ATOMIC_ENTER(sig);

    sig->event = FC_SIGNAL_EVENT_FREE;
    // 循环清零
    for (size_t i = 0; i < sizeof(fc_sig_t); i++)
    {
        ((uint8_t *)sig)[i] = 0;
    }
    sig->event = FC_SIGNAL_EVENT_FREE;
    sig->filter = NULL;
    sig->filter_func = NULL;

    sig->ioctl = ioctl;
    sig->user = user;

    sig->state_steady = sig->ioctl(sig, FC_SIGNAL_IOCTL_GET_STATE);
    sig->state_last = sig->state_steady;

    SIGNAL_ATOMIC_EXIT(sig);
}

/**
 * @brief
 *
 * @param sig
 * @param filter
 * @param filter_func
 */
void fc_sig_catch_filter(fc_sig_t *sig, void *filter, fc_sig_filter_t filter_func)
{
    fc_dev_assert(sig != NULL);

    SIGNAL_ATOMIC_ENTER(sig);
    sig->state_steady = sig->state_last;
    sig->filter = filter;
    sig->filter_func = filter_func;
    SIGNAL_ATOMIC_EXIT(sig);
}

//+********************************* 信号状态机任务 **********************************/
void fc_sig_heart(fc_sig_t *sig)
{
    fc_dev_assert(sig != NULL);
    fc_dev_assert(sig->ioctl != NULL);

    SIGNAL_HEART_ENTER(sig);

    fc_sig_state_t now_state = sig->ioctl(sig, FC_SIGNAL_IOCTL_GET_STATE);

    if (sig->filter_func)
    {
        sig->state_steady = sig->filter_func(sig->filter, sig, now_state);
    }
    else
    {
        sig->state_steady = now_state;  // 无滤波
    }

    switch (sig->event)  // 状态机处理
    {
    case FC_SIGNAL_EVENT_FREE:  // 自由状态无需管理
        break;

    case FC_SIGNAL_EVENT_EDGE_TRIGGER:
        if (sig->state_last != sig->state_steady && sig->state_trigger == sig->state_steady)  // 状态满足
        {
            fc_dev_assert(sig->func_trigger != NULL);
            if (FC_SIGNAL_TRIGGER_RET_END == sig->func_trigger(sig))  // 执行一次触发函数,如果触发函数返回结束表示不需要再触发了,否则会持续在改变时触发
            {
                sig->event = FC_SIGNAL_EVENT_FREE;
            }
        }
        break;

    case FC_SIGNAL_EVENT_EDGE_BOTH_TRIGGER:
        if (sig->state_last != sig->state_steady)  // 状态满足
        {
            fc_dev_assert(sig->func_trigger != NULL);
            if (FC_SIGNAL_TRIGGER_RET_END == sig->func_trigger(sig))  // 执行一次触发函数,如果触发函数返回结束表示不需要再触发了,否则会持续在改变时触发
            {
                sig->event = FC_SIGNAL_EVENT_FREE;
            }
        }
        break;

    default:
        fc_dev_assert(false);
        break;
    }

    sig->state_last = sig->state_steady;  //> 记录上一次状态

    SIGNAL_HEART_EXIT(sig);
}

void fc_sig_trigger(fc_sig_t *sig, fc_sig_trigger_t func_trigger, fc_sig_state_t state_trigger)
{
    fc_dev_assert(sig != NULL);
    // fc_dev_assert(func_trigger != NULL); // 可以为空表示不需要触发

    SIGNAL_ATOMIC_ENTER(sig);

    sig->event = FC_SIGNAL_EVENT_FREE;

    sig->func_trigger = func_trigger;
    if (NULL != func_trigger)
    {
        if (state_trigger == FC_SIGNAL_STATE_UNDEFINED)
        {
            sig->event = FC_SIGNAL_EVENT_EDGE_BOTH_TRIGGER;
        }
        else
        {
            sig->state_trigger = state_trigger;
            sig->event = FC_SIGNAL_EVENT_EDGE_TRIGGER;
        }
    }

    SIGNAL_ATOMIC_EXIT(sig);
}

fc_sig_state_t fc_sig_state(fc_sig_t *sig)
{
    fc_dev_assert(sig != NULL);

    return sig->state_steady;
}

fc_sig_state_t fc_sig_ioctl(fc_sig_t *sig, fc_sig_ioctl_cmd_t cmd)
{
    fc_dev_assert(sig != NULL);
    fc_dev_assert(sig->ioctl != NULL);

    return sig->ioctl(sig, cmd);
}
