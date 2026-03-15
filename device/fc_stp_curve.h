
// > 单次包含宏定义
#ifndef _FC_STEPPER_CURVE_H_
#define _FC_STEPPER_CURVE_H_

#include "fc_stp_base.h"

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

    //+********************************* 3段式对称梯形曲线 **********************************/

    // 对称梯形速度曲线,本质就是加速度为恒定值的曲线
    typedef struct __ladder_curve_t ladder_curve_t;

    // 都是非负数,不要设为负数
    struct __ladder_curve_t
    {
        int32_t v_target;      // 设定速度
        int32_t s_acc_target;  // 实际需要的长度

        int32_t a_now;  // 恒定加速度,不需要用户设置

        int32_t v_max;    // 最大速度
        int32_t v_start;  // 启动速度,结束速度
        int32_t s_acc;    // 加速需要的长度
    };

    void ladder_curve_init(ladder_curve_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc);  // 建议(v_max-v_start)/s_acc为整数
    void ladder_curve_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next);          // 函数指针类型,计算下一次的速度,用函数指针的形式开放出去用于匹配更多速度曲线实现

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_STEPPER_CURVE_H_
