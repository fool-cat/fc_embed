#include "fc_stp_base.h"
#include "fc_stp_curve.h"
#include <math.h>
#include "fc_stp_curve.h"

//+********************************* 对称梯形曲线初始化 **********************************/
void ladder_curve_init(ladder_curve_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc)
{
    fc_dev_assert(curve);
    fc_dev_assert(v_max > 0);
    fc_dev_assert(v_start > 0);
    fc_dev_assert(s_acc > 0);

    curve->v_max = v_max;
    curve->v_start = v_start;
    curve->s_acc = s_acc;

    int32_t v_diff = curve->v_max - curve->v_start;
    v_diff = FC_STP_ABS(v_diff);

    // 加速度是恒定的,理论只需要计算一次
    curve->a_now = (v_diff + curve->s_acc - 1) / curve->s_acc;  // 向上取整
    // 限制至少为1
    curve->a_now = curve->a_now < 1 ? 1 : curve->a_now;
}

//+********************************* 对称梯形曲线 **********************************/

static inline void ladder_calc_start(fc_stp_base_t *stp, ladder_curve_t *curve, int32_t *v_start)
{
    int32_t s_diff = stp->s_target - stp->s_now;
    s_diff = FC_STP_ABS(s_diff);

    stp->s_last = stp->s_now;  // 记录上一次的位置

    // 距离能够覆盖加减速段,速度曲线是梯形
    if ((s_diff >> 1) > curve->s_acc)
    {
        curve->s_acc_target = curve->s_acc;
        curve->v_target = curve->v_max;
    }
    else  // 距离可能不足以覆盖加减速段,速度曲线三角形(等于也属于三角形)
    {
        curve->s_acc_target = s_diff / 2;
        curve->v_target = curve->v_start + curve->a_now * s_diff / 2;
    }

    *v_start = curve->v_start;  // 速度初始化为启动速度
}

static inline void ladder_calc_next(fc_stp_base_t *stp, ladder_curve_t *curve, int32_t *v_next)
{
    int32_t s_diff = stp->s_target - stp->s_now;
    s_diff = FC_STP_ABS(s_diff);
    if (s_diff <= curve->s_acc_target)  // 判断是否进入了减速的阶段
    {
        // 减速阶段
        *v_next -= curve->a_now;
    }
    else  // 加速或者匀速阶段
    {
        *v_next += curve->a_now;
    }

    // 限定速度
    *v_next = *v_next > curve->v_target ? curve->v_target : (*v_next < curve->v_start ? curve->v_start : *v_next);  // 限制速度
}

static inline void ladder_calc_end(fc_stp_base_t *stp, ladder_curve_t *curve, int32_t *v_now)
{
    return;
}

static inline void ladder_calc_dec(fc_stp_base_t *stp, ladder_curve_t *curve, int32_t *v_dec)
{
    int32_t s_diff = stp->s_now - stp->s_last;
    s_diff = FC_STP_ABS(s_diff);

    if (s_diff >= curve->s_acc_target)  // 已经度过了加速阶段
    {
        if (stp->s_each_increase > 0)  // 正向运动
        {
            stp->s_target = stp->s_now + curve->s_acc_target;  // 强制进入减速阶段
        }
        else  // 反向运动
        {
            stp->s_target = stp->s_now - curve->s_acc_target;  // 强制进入减速阶段
        }
    }
    else  // 还在加速阶段,强制进入减速阶段
    {
        if (stp->s_each_increase > 0)  // 正向运动
        {
            stp->s_target = stp->s_now + s_diff;
        }
        else  // 反向运动
        {
            stp->s_target = stp->s_now - s_diff;
        }
    }
}

//+*********************************  **********************************/

void ladder_curve_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next)
{
    fc_dev_assert(stp);
    ladder_curve_t *curve = (ladder_curve_t *)stp->curve;
    fc_dev_assert(curve);

    switch (calc_type)
    {
    case FC_STP_CLAC_NEXT:  // 计算下一次的速度,ISR
        ladder_calc_next(stp, curve, v_next);
        break;

    case FC_STP_CLAC_DEC:  // 强制进入减速状态,ISR
        ladder_calc_dec(stp, curve, v_next);
        break;

    case FC_STP_CLAC_END:  // 运行结束,清理曲线数据,可能ISR中
        // ladder_calc_end(stp, curve, v_next);
        break;

    case FC_STP_CLAC_START:  // 运行开始,计算曲线数据,可能ISR中
        ladder_calc_start(stp, curve, v_next);
        break;

    default:
        break;
    }
}
