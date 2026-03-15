#include "fc_stp_base.h"
#include <string.h>

/**
 * @brief 初始化步进电机基础模块
 *
 * @param stp
 * @param ioctl io控制函数指针
 * @param freq_set 频率设置函数指针
 * @param user 用户数据指针
 */
void fc_stp_base_init(fc_stp_base_t *stp, fc_stp_ioctl_t ioctl, fc_stp_freq_set_t freq_set, void *user)
{
    memset(stp, 0, sizeof(fc_stp_base_t));

    stp->ioctl = ioctl;
    stp->freq_set = freq_set;
    stp->user = user;

    //> pulse_scale必须>=1,但是设置为1有要求,意味着发出动作脉冲后无需再次进入fc_stp_base_heart发出恢复脉冲(不调用stp->ioctl(stp, FC_STP_IOCTL_EDGE_FREE))
    stp->pulse_scale = 2;  // 默认为2,两次中断发出一个完整脉冲,除非发出脉冲后固定时间不用管动作释放脉冲可以设置为1
}

void fc_stp_base_catch_curve(fc_stp_base_t *stp, void *curve, fc_curve_func_t curve_func)
{
    fc_dev_assert(stp);
    fc_dev_assert(curve_func);

    stp->curve = curve;
    stp->curve_func = curve_func;  // 设置曲线函数指针
}

/**
 * @brief 步进电机基础心跳函数
 * 这里面不涉及方向的更改,只是简单的位置于速度设置,方向只需要在启动之前更改完毕
 * @param stp
 */
void fc_stp_base_heart(fc_stp_base_t *stp)
{
    fc_dev_assert(stp);
    fc_dev_assert(stp->ioctl);
    fc_dev_assert(stp->freq_set);
    fc_dev_assert(stp->curve_func);

    if (stp->s_now == stp->s_target)  // 到达目标位置
    {
        return;
    }
    STP_HEART_ENTER(stp);
    ++stp->heart_index;
    // 每次第一次进入就发出脉冲
    if (1 == stp->heart_index)
    {
        stp->ioctl(stp, FC_STP_IOCTL_EDGE_ACTION);
    }
    else if (2 == stp->heart_index)  // 第二次就释放脉冲
    {
        stp->ioctl(stp, FC_STP_IOCTL_EDGE_FREE);
    }
    // 更改速度有两种方式,一种固定频率更改pulse_scale,第二种是改频率,都更改也行!
    if (stp->heart_index >= stp->pulse_scale)  // 计数到最大值,进入下一次周期,更改速度
    {
        stp->heart_index = 0;
        stp->s_now += stp->s_each_increase;  // 位置更改放到这里
        if (stp->s_now != stp->s_target)
        {
            //>速度限制也丢给用户自己处理curve_func中自行确保速度不超过限制
            stp->curve_func(stp, FC_STP_CLAC_NEXT, &(stp->v_now));  // 重新计算下次速度
        }
        else  // 到达指定位置了
        {
            stp->v_now = 0;                                        // 速度清零
            stp->s_last = stp->s_now;                              // 记录上一次的位置
            stp->ioctl(stp, FC_STP_IOCTL_END);                     // 标记结束
            stp->curve_func(stp, FC_STP_CLAC_END, &(stp->v_now));  // 每次运行之后都会调用的API,用于清理曲线的数据之类的
        }
        // 调整fc_stp_isr调用频率,可以通过改变中断也可以通过固定频率调用改变pulse_scale实现
        stp->freq_set(stp, stp->v_now, &(stp->pulse_scale));  // 设置为实际需要的频率
    }
    STP_HEART_EXIT(stp);
}

/**
 * @brief 步进电机基础移动函数
 *
 * @param stp
 * @param move_mode 运行模式
 * @param s 目标位置或相对位移
 * @return true
 * @return false
 */
bool fc_stp_base_move(fc_stp_base_t *stp, fc_stp_move_t move_mode, int32_t s)
{
    fc_dev_assert(stp);
    fc_dev_assert(stp->ioctl);
    fc_dev_assert(stp->freq_set);
    fc_dev_assert(stp->curve_func);

    if (false == fc_stp_base_arrived(stp))  // 点对点的运行必须等到到达目标位置才能再次运行
    {
        return false;  // 点对点移动必须在电机停止之后进行
    }

    int32_t temp_target = 0;  // 用于延缓改变s_target,尽可能减少STP_HEART_ENTER依赖

    // STP_HEART_ENTER(stp);
    switch (move_mode)
    {
    case FC_STP_MOVE_TO:
        temp_target = s;
        break;

    case FC_STP_MOVE_RELATIVE:
        temp_target = stp->s_target + s;
        break;

    default:
        temp_target = stp->s_target;  // 未知模式默认不移动
        fc_dev_assert(false);
        break;
    }
    stp->s_last = stp->s_now;
    if (stp->s_now != temp_target)
    {
        stp->ioctl(stp, FC_STP_IOCTL_ENABLE);  // 使能
        if (temp_target > stp->s_now)          // 设定点处于当前正方向
        {
            stp->ioctl(stp, FC_STP_IOCTL_DIR_POSITIVE);  // 设置为正方向
            stp->s_each_increase = 1;
        }
        else  // 设定点处于当前负方向
        {
            stp->ioctl(stp, FC_STP_IOCTL_DIR_NEGATIVE);  // 设置为负方向
            stp->s_each_increase = -1;
        }
        STP_HEART_ENTER(stp);
        stp->s_target = temp_target;
        stp->curve_func(stp, FC_STP_CLAC_START, &(stp->v_now));  // 需要在这里面计算曲线相关数据并且设置初始速度
        stp->freq_set(stp, stp->v_now, &(stp->pulse_scale));     // 设置为实际需要的频率
        stp->ioctl(stp, FC_STP_IOCTL_START);                     // 标记启动
        STP_HEART_EXIT(stp);
    }
    else
    {
        stp->v_now = 0;  // 速度清零
        stp->s_target = temp_target;
        stp->freq_set(stp, stp->v_now, &(stp->pulse_scale));  // 设置为实际需要的频率
        stp->ioctl(stp, FC_STP_IOCTL_START);                  // 标记启动
        stp->ioctl(stp, FC_STP_IOCTL_END);                    // 标记结束
    }
    // STP_HEART_EXIT(stp);

    return true;
}

/**
 * @brief 电机停止触发
 *
 * @param stp
 * @param safe_stop 是否安全停止,true为安全停止(减速到停止), false为急停(立即停止)
 */
void fc_stp_base_stop(fc_stp_base_t *stp, bool safe_stop)
{
    fc_dev_assert(stp);
    fc_dev_assert(stp->freq_set);
    fc_dev_assert(NULL != stp->curve_func);

    // STP_HEART_ENTER(stp);
    {
        if (safe_stop)  // 缓停
        {
            STP_HEART_ENTER(stp);
            stp->curve_func(stp, FC_STP_CLAC_DEC, &(stp->v_now));  // 调用函数进入减速状态
            stp->freq_set(stp, stp->v_now, &(stp->pulse_scale));
            STP_HEART_EXIT(stp);
        }
        else
        {
            stp->s_target = stp->s_now;  // 设置为当前位置,心跳函数再次进入会无操作
            stp->v_now = 0;              // 速度清零
            stp->heart_index = 0;        // 重置计数器
            // stp->a_now = 0;             // 加速度设置为0

            stp->ioctl(stp, FC_STP_IOCTL_EDGE_FREE);  // 释放为下一次做准备
            stp->curve_func(stp, FC_STP_CLAC_END, &(stp->v_now));

            stp->s_target = stp->s_now;  // 再设置一次,防止中间产生过心跳调用
            stp->freq_set(stp, stp->v_now, &(stp->pulse_scale));
        }
    }
    // STP_HEART_EXIT(stp);

    return;
}

/**
 * @brief 获取当前位置
 *
 * @param stp
 * @return int32_t
 */
int32_t fc_stp_base_get_pos(fc_stp_base_t *stp)
{
    fc_dev_assert(stp);
    return stp->s_now;
}

/**
 * @brief 判断电机是否到达目标位置
 *
 * @param stp
 * @return true
 * @return false
 */
bool fc_stp_base_arrived(fc_stp_base_t *stp)
{
    fc_dev_assert(stp);
    // 此电机对象是针对点对点的运动,所以位置到达了速度及加速度都是0
    if (stp->s_now == stp->s_target && stp->v_now == 0)  // 到达目标位置
    {
        // if (stp->a_now == 0)  // 电机本体不引入加速度,曲线对象中可能引入
        return true;
    }
    return false;
}

/**
 * @brief 修正电机位置,只能在电机停止时调用
 *
 * @param stp
 * @param move_mode
 * @param s
 * @return true
 * @return false
 */
bool fc_stp_base_rectify_pos(fc_stp_base_t *stp, fc_stp_move_t move_mode, int32_t s)
{
    fc_dev_assert(stp);
    bool ret = false;
    STP_ATOMIC_ENTER(stp);
    if (fc_stp_base_arrived(stp))
    {
        if (FC_STP_MOVE_TO == move_mode)
        {
            stp->s_now = s;
            stp->s_target = s;
            stp->s_last = s;
        }
        else if (FC_STP_MOVE_RELATIVE == move_mode)
        {
            stp->s_now += s;
            stp->s_target += s;
            stp->s_last += s;
        }
        ret = true;
    }
    STP_ATOMIC_EXIT(stp);
    return ret;
}

/**
 * @brief 电机位置偏移,慎用!这个接口通常用于给曲线对象使用
 *
 * @param stp
 * @param s
 */
void fc_stp_base_shift_pos(fc_stp_base_t *stp, int32_t s)
{
    fc_dev_assert(stp);

    STP_ATOMIC_ENTER(stp);
    stp->s_now += s;
    stp->s_target += s;
    stp->s_last += s;
    STP_ATOMIC_EXIT(stp);

    return;
}

/**
 * @brief 直接调用ioctl接口
 *
 * @param stp
 * @param cmd
 */
void fc_stp_base_ioctl(fc_stp_base_t *stp, fc_stp_ioctl_cmd_t cmd)
{
    fc_dev_assert(stp);
    stp->ioctl(stp, cmd);
}
