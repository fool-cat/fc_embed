
// > 单次包含宏定义
#ifndef _FC_STEPPER_CURVE_H_
#define _FC_STEPPER_CURVE_H_

/** fc_stp_curve component version string — keep in sync with Cversion in fool_cat.fc_embed.pdsc */
#define FC_STP_CURVE_VERSION "2.0.0"

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

        int32_t a_now;        // 前段每步速度增量
        int32_t a_back;       // 后段每步速度增量
        int32_t s_acc_front;  // 前段长度
        int32_t s_acc_back;   // 后段长度

        int32_t v_max;    // 最大速度
        int32_t v_start;  // 启动速度,结束速度
        int32_t s_acc;    // 加速需要的长度
    };

    void   ladder_curve_init(ladder_curve_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc);   // 建议(v_max-v_start)/s_acc为整数
    size_t ladder_curve_init_time(ladder_curve_t *curve, int32_t v_max, int32_t v_start, size_t ms);  // 初始化曲线,不直接指定s_acc,改为预期加速时间(ms),返回实际s_acc
    void   ladder_curve_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next);           // 函数指针类型,计算下一次的速度,用函数指针的形式开放出去用于匹配更多速度曲线实现

    //+********************************* 标准梯形曲线 **********************************/

    typedef struct __trapezoid_curve_t trapezoid_curve_t;

    // 都是非负数,不要设为负数
    struct __trapezoid_curve_t
    {
        int32_t v_target;      // 当前这次运动实际能够达到的峰值速度
        int32_t s_acc_target;  // 当前这次运动实际需要的加速长度

        int32_t a_abs;  // 恒定加速度绝对值(steps/s^2)

        int32_t v_max;    // 最大速度
        int32_t v_start;  // 启动速度,结束速度
        int32_t s_acc;    // 从v_start加速到v_max所需步数
    };

    // 与ladder_curve_init保持相同的接口风格:
    // 根据(v_max, v_start, s_acc)反推恒定加速度a, 使得理论上在s_acc步内从v_start加速到v_max
    void   trapezoid_curve_init(trapezoid_curve_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc);
    size_t trapezoid_curve_init_time(trapezoid_curve_t *curve, int32_t v_max, int32_t v_start, size_t ms);
    void   trapezoid_curve_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next);

    //+********************************* S形曲线 **********************************/

    typedef struct __s_curve_t s_curve_t;

    // 这套S形曲线针对当前“每步更新一次速度”的框架做适配:
    // 使用平滑函数构造加速/减速段, 让速度变化更平滑.
    struct __s_curve_t
    {
        int32_t v_target;      // 当前这次运动实际能够达到的峰值速度
        int32_t s_acc_target;  // 当前这次运动实际需要的加速长度

        int32_t v_max;    // 最大速度
        int32_t v_start;  // 启动速度,结束速度
        int32_t s_acc;    // 期望的加速长度
    };

    void   s_curve_init(s_curve_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc);
    size_t s_curve_init_time(s_curve_t *curve, int32_t v_max, int32_t v_start, size_t ms);
    void   s_curve_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next);

    //+********************************* 三角加速度型S曲线 **********************************/

    typedef struct __s_curve_tri_acc_t s_curve_tri_acc_t;

    // 这套S形曲线的目标是:
    // 让“每步速度增量”呈现近似对称三角形.
    // 在当前每步更新一次速度的框架下,这会比smoothstep型S曲线更接近
    // “加速度上升-下降对称”的直观表现.
    struct __s_curve_tri_acc_t
    {
        int32_t v_target;      // 当前这次运动实际能够达到的峰值速度
        int32_t s_acc_target;  // 当前这次运动实际需要的加速长度

        int32_t v_max;    // 最大速度
        int32_t v_start;  // 启动速度,结束速度
        int32_t s_acc;    // 期望的加速长度
    };

    void   s_curve_tri_acc_init(s_curve_tri_acc_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc);
    size_t s_curve_tri_acc_init_time(s_curve_tri_acc_t *curve, int32_t v_max, int32_t v_start, size_t ms);
    void   s_curve_tri_acc_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next);

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_STEPPER_CURVE_H_
