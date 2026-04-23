/**
 * @file fc_sig_filter.h
 * @author fool_cat (2696652257@qq.com)
 * @brief 为信号对象添加滤波策略
 * @version 1.0
 * @date 2026-03-28
 *
 * @copyright Copyright (c) 2026
 *
 */

// > 单次包含宏定义
#ifndef _FC_SIG_FILTER_H_
#define _FC_SIG_FILTER_H_

#include "fc_sig.h"

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif

    //+********************************* 比例滤波器 **********************************/
    /**
     * @brief 在count_repeat次数内有效或者无效计数大于count_critical时, 就输出数值较大的状态
     * 用的比较少,仍容易出现抖动的情况
     */
    typedef struct _fc_sig_scale_filter_t fc_sig_scale_filter_t;
    struct _fc_sig_scale_filter_t
    {
        int32_t count_repeat;    // 稳定阈值
        int32_t count_valid;     // 有效计数
        int32_t count_invalid;   // 无效计数
        int32_t count_critical;  // 临界计数
    };

    void           fc_sig_filter_scale_init(fc_sig_scale_filter_t *filter, int32_t count_repeat, int32_t count_critical);
    fc_sig_state_t fc_sig_filter_scale(void *filter, fc_sig_t *sig, fc_sig_state_t state);

    //+********************************* 连续重复滤波器 **********************************/
    /**
     * @brief 在count_repeat次数内有效或者无效计数大于count_critical时, 就输出数值较大的状态,特点是状态跳转一下另一个状态会清空计数
     * 常用于信号的去抖处理,只有一个信号连续多少次都没有变化才输出该状态,适合在临界点会持续抖动的信号
     */
    typedef struct _fc_sig_repeat_filter_t fc_sig_repeat_filter_t;
    struct _fc_sig_repeat_filter_t
    {
        int32_t count_repeat;   // 重复计数
        int32_t count_valid;    // 有效计数
        int32_t count_invalid;  // 无效计数
    };

    void           fc_sig_filter_repeat_init(fc_sig_repeat_filter_t *filter, int32_t count_repeat);
    fc_sig_state_t fc_sig_filter_repeat(void *filter, fc_sig_t *sig, fc_sig_state_t state);

    //+********************************* 非对称连续重复滤波器 **********************************/
    /**
     * @brief 有效和无效分别使用不同的重复计数,适合吸合和释放需求不一致的场景
     * 比如有效要快速响应,无效要更稳一些,可以将count_valid_repeat设置小一点, count_invalid_repeat设置大一点
     */
    typedef struct _fc_sig_asym_repeat_filter_t fc_sig_asym_repeat_filter_t;
    struct _fc_sig_asym_repeat_filter_t
    {
        int32_t count_valid_repeat;    // 切换到有效态所需的连续计数
        int32_t count_invalid_repeat;  // 切换到无效态所需的连续计数
        int32_t count_valid;           // 当前有效连续计数
        int32_t count_invalid;         // 当前无效连续计数
    };

    void           fc_sig_filter_asym_repeat_init(fc_sig_asym_repeat_filter_t *filter, int32_t count_valid_repeat, int32_t count_invalid_repeat);
    fc_sig_state_t fc_sig_filter_asym_repeat(void *filter, fc_sig_t *sig, fc_sig_state_t state);

    //+********************************* 积分滤波器 **********************************/
    /**
     * @brief 使用上下积分的方式靠近目标状态,短时间抖动不会立刻打断整体趋势
     * 累计到上阈值输出有效,累计到下阈值输出无效,介于中间时保持稳态
     * 为了避免有效和无效阈值重叠,建议count_threshold设置为大于count_max的一半
     */
    typedef struct _fc_sig_integrator_filter_t fc_sig_integrator_filter_t;
    struct _fc_sig_integrator_filter_t
    {
        int32_t count_max;        // 积分上限
        int32_t count_threshold;  // 输出阈值,需满足0 < count_threshold < count_max
        int32_t accumulator;      // 当前积分值
    };

    void           fc_sig_filter_integrator_init(fc_sig_integrator_filter_t *filter, int32_t count_max, int32_t count_threshold, fc_sig_state_t init_state);
    fc_sig_state_t fc_sig_filter_integrator(void *filter, fc_sig_t *sig, fc_sig_state_t state);

    //+********************************* 滑动窗口多数决滤波器 **********************************/
    /**
     * @brief 对最近count_window次采样进行多数决,适合周期采样的传感器状态判定
     * count_threshold表示窗口内至少有多少次为目标状态才切换,通常可设置为一半以上
     * 当前实现最多支持32次窗口,若要严格多数决,建议count_threshold设置为大于count_window的一半
     */
    typedef struct _fc_sig_window_filter_t fc_sig_window_filter_t;
    struct _fc_sig_window_filter_t
    {
        int32_t  count_window;     // 滑动窗口大小,范围1~32
        int32_t  count_threshold;  // 多数决阈值
        int32_t  count_sample;     // 当前已有样本数
        uint32_t history;          // 历史位图,1表示有效,0表示无效
    };

    void           fc_sig_filter_window_init(fc_sig_window_filter_t *filter, int32_t count_window, int32_t count_threshold);
    fc_sig_state_t fc_sig_filter_window(void *filter, fc_sig_t *sig, fc_sig_state_t state);

#ifdef __cplusplus
}
#endif  //\ __cplusplus

#endif  //\ _FC_SIG_FILTER_H_
