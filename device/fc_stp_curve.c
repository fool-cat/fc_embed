#include "fc_stp_base.h"
#include "fc_stp_curve.h"
#include <limits.h>

static inline void     stp_curve_set_hold_steps(fc_stp_base_t *stp, int32_t s_remain, int32_t s_dec);
static inline int32_t  stp_div_ceil_i64(int64_t n, int64_t d);
static inline uint64_t stp_curve_step_time_ns(int32_t v);
static int32_t         stp_curve_find_s_acc_by_time(uint64_t target_ns, int32_t v_max, int32_t v_start, uint64_t (*calc_time_ns)(int32_t, int32_t, int32_t));
static uint64_t        ladder_curve_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc);
static uint64_t        trapezoid_curve_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc);
static uint64_t        s_curve_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc);
static uint64_t        s_curve_tri_acc_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc);
static inline int32_t  s_curve_calc_speed_by_progress(s_curve_t *curve, int32_t s_progress);
static inline int32_t  s_curve_tri_acc_speed_by_progress(s_curve_tri_acc_t *curve, int32_t s_progress);

static inline uint64_t stp_curve_step_time_ns(int32_t v)
{
    fc_dev_assert(v > 0);
    return (1000000000ULL + ((uint64_t)v >> 1)) / (uint64_t)v;
}

static int32_t stp_curve_find_s_acc_by_time(
    uint64_t target_ns, int32_t v_max, int32_t v_start, uint64_t (*calc_time_ns)(int32_t, int32_t, int32_t))
{
    fc_dev_assert(v_max > 0);
    fc_dev_assert(v_start > 0);
    fc_dev_assert(calc_time_ns);

    if (target_ns == 0)
    {
        return 1;
    }

    uint64_t v_avg = ((uint64_t)v_max + (uint64_t)v_start) >> 1;
    if (v_avg == 0)
    {
        v_avg = 1;
    }

    uint64_t guess = (target_ns * v_avg + 999999999ULL) / 1000000000ULL;
    int32_t  hi = (guess > (uint64_t)INT32_MAX) ? INT32_MAX : (int32_t)guess;
    if (hi < 1)
    {
        hi = 1;
    }

    while (hi < INT32_MAX)
    {
        uint64_t t_hi = calc_time_ns(v_max, v_start, hi);
        if (t_hi >= target_ns)
        {
            break;
        }

        int32_t next_hi = (hi > (INT32_MAX >> 1)) ? INT32_MAX : (hi << 1);
        if (next_hi == hi)
        {
            break;
        }
        hi = next_hi;
    }

    int32_t lo = 1;
    while (lo < hi)
    {
        int32_t  mid = lo + ((hi - lo) >> 1);
        uint64_t t_mid = calc_time_ns(v_max, v_start, mid);
        if (t_mid < target_ns)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }

    int32_t  best = lo;
    uint64_t best_t = calc_time_ns(v_max, v_start, best);
    uint64_t best_diff = (best_t > target_ns) ? (best_t - target_ns) : (target_ns - best_t);

    if (best > 1)
    {
        uint64_t prev_t = calc_time_ns(v_max, v_start, best - 1);
        uint64_t prev_diff = (prev_t > target_ns) ? (prev_t - target_ns) : (target_ns - prev_t);
        if (prev_diff <= best_diff)
        {
            best = best - 1;
        }
    }

    return best < 1 ? 1 : best;
}

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

static uint64_t ladder_curve_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc)
{
    ladder_curve_t curve = {0};
    uint64_t       time_ns = 0;
    int32_t        v_now = v_start;

    ladder_curve_init(&curve, v_max, v_start, s_acc);
    for (int32_t i = 0; i < s_acc; ++i)
    {
        time_ns += stp_curve_step_time_ns(v_now);
        if (v_now < curve.v_max)
        {
            v_now += curve.a_now;
            v_now = v_now > curve.v_max ? curve.v_max : v_now;
        }
    }

    return time_ns;
}

size_t ladder_curve_init_time(ladder_curve_t *curve, int32_t v_max, int32_t v_start, size_t ms)
{
    fc_dev_assert(curve);

    int32_t s_acc = stp_curve_find_s_acc_by_time((uint64_t)ms * 1000000ULL, v_max, v_start, ladder_curve_accel_time_ns);
    ladder_curve_init(curve, v_max, v_start, s_acc);
    return (size_t)s_acc;
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

static inline int32_t ladder_calc_stop_steps(ladder_curve_t *curve, int32_t v_now)
{
    fc_dev_assert(curve);

    if (v_now <= curve->v_start)
    {
        return 0;
    }

    return stp_div_ceil_i64((int64_t)(v_now - curve->v_start), (int64_t)curve->a_now);
}

static inline void ladder_calc_next(fc_stp_base_t *stp, ladder_curve_t *curve, int32_t *v_next)
{
    int32_t s_diff = stp->s_target - stp->s_now;
    s_diff = FC_STP_ABS(s_diff);
    int32_t s_stop = ladder_calc_stop_steps(curve, *v_next);
    stp->curve_hold_steps = 0;
    if (s_diff <= s_stop)  // 根据当前速度动态判断是否进入减速阶段
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

    if (*v_next == curve->v_target && s_diff > s_stop)
    {
        stp_curve_set_hold_steps(stp, s_diff, s_stop);
    }
}

static inline void ladder_calc_end(fc_stp_base_t *stp, ladder_curve_t *curve, int32_t *v_now)
{
    return;
}

static inline void ladder_calc_dec(fc_stp_base_t *stp, ladder_curve_t *curve, int32_t *v_dec)
{
    int32_t s_stop = ladder_calc_stop_steps(curve, *v_dec);
    s_stop = s_stop < 1 ? 1 : s_stop;

    if (stp->s_each_increase > 0)  // 正向运动
    {
        stp->s_target = stp->s_now + s_stop;  // 强制进入减速阶段
    }
    else  // 反向运动
    {
        stp->s_target = stp->s_now - s_stop;  // 强制进入减速阶段
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

//+********************************* 标准梯形曲线 **********************************/

static inline int64_t stp_square_i32(int32_t x)
{
    return (int64_t)x * (int64_t)x;
}

static inline int32_t stp_isqrt_i64(int64_t x)
{
    if (x <= 0)
    {
        return 0;
    }

    // Integer square root: returns floor(sqrt(x)).
    // Based on the binary restoring method, avoids any floating-point dependency.
    uint64_t n = (uint64_t)x;
    uint64_t res = 0;
    uint64_t bit = 1ULL << 62;

    while (bit > n)
    {
        bit >>= 2;
    }

    while (bit != 0)
    {
        if (n >= res + bit)
        {
            n -= res + bit;
            res = (res >> 1) + bit;
        }
        else
        {
            res >>= 1;
        }
        bit >>= 2;
    }

    return (int32_t)res;
}

static inline int32_t stp_div_ceil_i64(int64_t n, int64_t d)
{
    fc_dev_assert(d > 0);
    if (n <= 0)
    {
        return 0;
    }
    return (int32_t)((n + d - 1) / d);
}

static inline void stp_curve_set_hold_steps(fc_stp_base_t *stp, int32_t s_remain, int32_t s_dec)
{
    fc_dev_assert(stp);

    if (s_remain <= (s_dec + 1))
    {
        stp->curve_hold_steps = 0;
        return;
    }

    stp->curve_hold_steps = (size_t)(s_remain - s_dec - 1);
}

void trapezoid_curve_init(trapezoid_curve_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc)
{
    fc_dev_assert(curve);
    fc_dev_assert(v_max > 0);
    fc_dev_assert(v_start > 0);
    fc_dev_assert(s_acc > 0);
    fc_dev_assert(v_max >= v_start);

    curve->v_max = v_max;
    curve->v_start = v_start;
    curve->s_acc = s_acc;

    // 由 v^2 = v0^2 + 2as 反推恒定加速度.
    // 向上取整,保证理论上不晚于s_acc步达到v_max.
    int64_t v_diff_sq = stp_square_i32(v_max) - stp_square_i32(v_start);
    curve->a_abs = stp_div_ceil_i64(v_diff_sq, 2LL * (int64_t)s_acc);
    curve->a_abs = curve->a_abs < 1 ? 1 : curve->a_abs;

    curve->v_target = v_start;
    curve->s_acc_target = 0;
}

static uint64_t trapezoid_curve_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc)
{
    trapezoid_curve_t curve = {0};
    uint64_t          time_ns = 0;

    trapezoid_curve_init(&curve, v_max, v_start, s_acc);
    for (int32_t i = 0; i < s_acc; ++i)
    {
        int64_t v_sq = stp_square_i32(curve.v_start) + 2LL * (int64_t)curve.a_abs * (int64_t)i;
        int32_t v_now = stp_isqrt_i64(v_sq);
        v_now = v_now > curve.v_max ? curve.v_max : v_now;
        v_now = v_now < curve.v_start ? curve.v_start : v_now;
        time_ns += stp_curve_step_time_ns(v_now);
    }

    return time_ns;
}

size_t trapezoid_curve_init_time(trapezoid_curve_t *curve, int32_t v_max, int32_t v_start, size_t ms)
{
    fc_dev_assert(curve);

    int32_t s_acc =
        stp_curve_find_s_acc_by_time((uint64_t)ms * 1000000ULL, v_max, v_start, trapezoid_curve_accel_time_ns);
    trapezoid_curve_init(curve, v_max, v_start, s_acc);
    return (size_t)s_acc;
}

static inline int32_t trapezoid_calc_peak_speed(trapezoid_curve_t *curve, int32_t s_total)
{
    // 对称加减速,且起始/结束速度相同为v_start:
    // v_peak^2 = v_start^2 + a * s_total
    int64_t v_peak_sq = stp_square_i32(curve->v_start) + (int64_t)curve->a_abs * (int64_t)s_total;
    int32_t v_peak = stp_isqrt_i64(v_peak_sq);
    if (v_peak < curve->v_start)
    {
        v_peak = curve->v_start;
    }
    return v_peak > curve->v_max ? curve->v_max : v_peak;
}

static inline void trapezoid_calc_start(fc_stp_base_t *stp, trapezoid_curve_t *curve, int32_t *v_start)
{
    int32_t s_total = FC_STP_ABS(stp->s_target - stp->s_now);
    int32_t s_to_vmax =
        stp_div_ceil_i64(stp_square_i32(curve->v_max) - stp_square_i32(curve->v_start), 2LL * (int64_t)curve->a_abs);

    stp->s_last = stp->s_now;  // 记录这次运动的起点

    if (s_total > (s_to_vmax << 1))
    {
        curve->v_target = curve->v_max;
        curve->s_acc_target = s_to_vmax;
    }
    else
    {
        curve->v_target = trapezoid_calc_peak_speed(curve, s_total);
        curve->s_acc_target =
            stp_div_ceil_i64(stp_square_i32(curve->v_target) - stp_square_i32(curve->v_start), 2LL * (int64_t)curve->a_abs);
    }

    *v_start = curve->v_start;
}

static inline void trapezoid_calc_next(fc_stp_base_t *stp, trapezoid_curve_t *curve, int32_t *v_next)
{
    int32_t s_moved = FC_STP_ABS(stp->s_now - stp->s_last);
    int32_t s_remain = FC_STP_ABS(stp->s_target - stp->s_now);
    stp->curve_hold_steps = 0;

    // 加速分支: v^2 = v0^2 + 2as
    int64_t v_acc_sq = stp_square_i32(curve->v_start) + 2LL * (int64_t)curve->a_abs * (int64_t)s_moved;
    int32_t v_acc = stp_isqrt_i64(v_acc_sq);

    // 减速分支: 以相同加速度减回v_start所需的剩余距离
    int64_t v_dec_sq = stp_square_i32(curve->v_start) + 2LL * (int64_t)curve->a_abs * (int64_t)s_remain;
    int32_t v_dec = stp_isqrt_i64(v_dec_sq);

    int32_t v_cmd = v_acc < v_dec ? v_acc : v_dec;
    v_cmd = v_cmd > curve->v_target ? curve->v_target : v_cmd;
    v_cmd = v_cmd < curve->v_start ? curve->v_start : v_cmd;

    *v_next = v_cmd;

    if (v_cmd == curve->v_target && s_remain > curve->s_acc_target)
    {
        stp_curve_set_hold_steps(stp, s_remain, curve->s_acc_target);
    }
}

static inline void trapezoid_calc_dec(fc_stp_base_t *stp, trapezoid_curve_t *curve, int32_t *v_dec)
{
    int32_t s_stop =
        stp_div_ceil_i64(stp_square_i32(*v_dec) - stp_square_i32(curve->v_start), 2LL * (int64_t)curve->a_abs);

    // 至少保留1步,避免在中途把目标直接设置为当前位置导致状态机停在“未正常结束”的状态.
    s_stop = s_stop < 1 ? 1 : s_stop;

    if (stp->s_each_increase > 0)
    {
        stp->s_target = stp->s_now + s_stop;
    }
    else
    {
        stp->s_target = stp->s_now - s_stop;
    }
}

void trapezoid_curve_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next)
{
    fc_dev_assert(stp);
    trapezoid_curve_t *curve = (trapezoid_curve_t *)stp->curve;
    fc_dev_assert(curve);

    switch (calc_type)
    {
    case FC_STP_CLAC_NEXT:
        trapezoid_calc_next(stp, curve, v_next);
        break;

    case FC_STP_CLAC_DEC:
        trapezoid_calc_dec(stp, curve, v_next);
        break;

    case FC_STP_CLAC_END:
        break;

    case FC_STP_CLAC_START:
        trapezoid_calc_start(stp, curve, v_next);
        break;

    default:
        break;
    }
}

//+********************************* S形曲线 **********************************/

#define S_CURVE_Q_SHIFT 15
#define S_CURVE_Q_ONE (1UL << S_CURVE_Q_SHIFT)

static inline uint32_t s_curve_u32_clamp(uint32_t x, uint32_t min_v, uint32_t max_v)
{
    return x < min_v ? min_v : (x > max_v ? max_v : x);
}

static inline uint32_t stp_div_round_u64(uint64_t n, uint32_t d)
{
    fc_dev_assert(d > 0);
    return (uint32_t)((n + ((uint64_t)d >> 1)) / d);
}

static inline uint32_t s_curve_ease_q15(int32_t progress, int32_t total)
{
    // 使用三次 smoothstep: f(u) = 3u^2 - 2u^3
    // 特点:
    // 1. 纯整数/定点实现
    // 2. 两端一阶导数为0, 适合作为S形平滑函数
    if (total <= 0)
    {
        return S_CURVE_Q_ONE;
    }
    if (progress <= 0)
    {
        return 0;
    }
    if (progress >= total)
    {
        return S_CURVE_Q_ONE;
    }

    uint32_t u_q15 = stp_div_round_u64((uint64_t)progress * (uint64_t)S_CURVE_Q_ONE, (uint32_t)total);
    uint64_t u2_q30 = (uint64_t)u_q15 * (uint64_t)u_q15;
    uint64_t u3_q45 = u2_q30 * (uint64_t)u_q15;

    uint32_t term1_q15 = (uint32_t)((3ULL * u2_q30 + (1ULL << (S_CURVE_Q_SHIFT - 1))) >> S_CURVE_Q_SHIFT);
    uint32_t term2_q15 = (uint32_t)((2ULL * u3_q45 + (1ULL << ((S_CURVE_Q_SHIFT * 2) - 1))) >> (S_CURVE_Q_SHIFT * 2));
    uint32_t y_q15 = term1_q15 - term2_q15;
    return s_curve_u32_clamp(y_q15, 0, S_CURVE_Q_ONE);
}

void s_curve_init(s_curve_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc)
{
    fc_dev_assert(curve);
    fc_dev_assert(v_max > 0);
    fc_dev_assert(v_start > 0);
    fc_dev_assert(s_acc > 0);
    fc_dev_assert(v_max >= v_start);

    curve->v_max = v_max;
    curve->v_start = v_start;
    curve->s_acc = s_acc;

    curve->v_target = v_start;
    curve->s_acc_target = 0;
}

static uint64_t s_curve_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc)
{
    s_curve_t curve = {0};
    uint64_t  time_ns = 0;

    s_curve_init(&curve, v_max, v_start, s_acc);
    curve.v_target = curve.v_max;
    curve.s_acc_target = curve.s_acc;
    for (int32_t i = 0; i < s_acc; ++i)
    {
        int32_t v_now = s_curve_calc_speed_by_progress(&curve, i);
        time_ns += stp_curve_step_time_ns(v_now);
    }

    return time_ns;
}

size_t s_curve_init_time(s_curve_t *curve, int32_t v_max, int32_t v_start, size_t ms)
{
    fc_dev_assert(curve);

    int32_t s_acc = stp_curve_find_s_acc_by_time((uint64_t)ms * 1000000ULL, v_max, v_start, s_curve_accel_time_ns);
    s_curve_init(curve, v_max, v_start, s_acc);
    return (size_t)s_acc;
}

static inline void s_curve_calc_start(fc_stp_base_t *stp, s_curve_t *curve, int32_t *v_start)
{
    int32_t s_total = FC_STP_ABS(stp->s_target - stp->s_now);

    stp->s_last = stp->s_now;  // 记录这次运动的起点

    if ((s_total >> 1) > curve->s_acc)
    {
        curve->s_acc_target = curve->s_acc;
        curve->v_target = curve->v_max;
    }
    else
    {
        // 短行程时退化为对称S形三角曲线
        curve->s_acc_target = s_total >> 1;
        if (curve->s_acc_target < 1)
        {
            curve->s_acc_target = 1;
        }

        // 保持与用户配置量级一致的简单缩放:
        // 总长度不足时,按加速长度比例缩小峰值速度.
        int64_t dv = (int64_t)(curve->v_max - curve->v_start) * (int64_t)curve->s_acc_target;
        dv /= (int64_t)curve->s_acc;
        curve->v_target = curve->v_start + (int32_t)dv;
        curve->v_target = curve->v_target > curve->v_max ? curve->v_max : curve->v_target;
    }

    *v_start = curve->v_start;
}

static inline int32_t s_curve_calc_speed_by_progress(s_curve_t *curve, int32_t s_progress)
{
    int32_t dv = curve->v_target - curve->v_start;
    if (dv <= 0)
    {
        return curve->v_start;
    }
    if (curve->s_acc_target <= 0)
    {
        return curve->v_target;
    }

    uint32_t ease_q15 = s_curve_ease_q15(s_progress, curve->s_acc_target);
    int32_t  v_cmd = curve->v_start + (int32_t)(((int64_t)dv * (int64_t)ease_q15 + (S_CURVE_Q_ONE >> 1)) >> S_CURVE_Q_SHIFT);
    if (v_cmd < curve->v_start)
    {
        v_cmd = curve->v_start;
    }
    if (v_cmd > curve->v_target)
    {
        v_cmd = curve->v_target;
    }
    return v_cmd;
}

static inline int32_t s_curve_calc_stop_steps(s_curve_t *curve, int32_t v_now)
{
    if (v_now <= curve->v_start)
    {
        return 0;
    }
    if (curve->s_acc_target <= 0)
    {
        return 0;
    }
    if (v_now >= curve->v_target)
    {
        return curve->s_acc_target;
    }

    // safe stop触发频率很低,这里用二分搜索反推“从当前速度减到v_start”所需步数,
    // 用少量比较换掉反三角/浮点运算.
    int32_t lo = 0;
    int32_t hi = curve->s_acc_target;
    while (lo < hi)
    {
        int32_t mid = lo + ((hi - lo) >> 1);
        int32_t v_mid = s_curve_calc_speed_by_progress(curve, mid);
        if (v_mid < v_now)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    return lo;
}

static inline void s_curve_calc_next(fc_stp_base_t *stp, s_curve_t *curve, int32_t *v_next)
{
    int32_t s_moved = FC_STP_ABS(stp->s_now - stp->s_last);
    int32_t s_remain = FC_STP_ABS(stp->s_target - stp->s_now);
    stp->curve_hold_steps = 0;

    int32_t v_acc = s_curve_calc_speed_by_progress(curve, s_moved);
    int32_t v_dec = s_curve_calc_speed_by_progress(curve, s_remain);

    int32_t v_cmd = v_acc < v_dec ? v_acc : v_dec;
    v_cmd = v_cmd > curve->v_target ? curve->v_target : v_cmd;
    v_cmd = v_cmd < curve->v_start ? curve->v_start : v_cmd;
    *v_next = v_cmd;

    if (v_cmd == curve->v_target && s_remain > curve->s_acc_target)
    {
        stp_curve_set_hold_steps(stp, s_remain, curve->s_acc_target);
    }
}

static inline void s_curve_calc_dec(fc_stp_base_t *stp, s_curve_t *curve, int32_t *v_dec)
{
    int32_t s_stop = s_curve_calc_stop_steps(curve, *v_dec);

    // 至少保留1步,避免在中途把目标直接设置为当前位置导致状态机停在“未正常结束”的状态.
    s_stop = s_stop < 1 ? 1 : s_stop;

    if (stp->s_each_increase > 0)
    {
        stp->s_target = stp->s_now + s_stop;
    }
    else
    {
        stp->s_target = stp->s_now - s_stop;
    }
}

void s_curve_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next)
{
    fc_dev_assert(stp);
    s_curve_t *curve = (s_curve_t *)stp->curve;
    fc_dev_assert(curve);

    switch (calc_type)
    {
    case FC_STP_CLAC_NEXT:
        s_curve_calc_next(stp, curve, v_next);
        break;

    case FC_STP_CLAC_DEC:
        s_curve_calc_dec(stp, curve, v_next);
        break;

    case FC_STP_CLAC_END:
        break;

    case FC_STP_CLAC_START:
        s_curve_calc_start(stp, curve, v_next);
        break;

    default:
        break;
    }
}

//+********************************* 三角加速度型S曲线 **********************************/

static inline uint32_t s_curve_tri_acc_ease_q15(int32_t progress, int32_t total)
{
    // 分段二次:
    // u in [0, 0.5]:  y = 2u^2
    // u in [0.5, 1]:  y = 1 - 2(1-u)^2
    //
    // 其一阶导数(在当前离散实现里可理解为“每步速度增量”的形状)为:
    // y' = 4u,                 u in [0, 0.5]
    // y' = 4(1-u),            u in [0.5, 1]
    //
    // 因此表现为对称三角形.
    if (total <= 0)
    {
        return S_CURVE_Q_ONE;
    }
    if (progress <= 0)
    {
        return 0;
    }
    if (progress >= total)
    {
        return S_CURVE_Q_ONE;
    }

    uint32_t u_q15 = stp_div_round_u64((uint64_t)progress * (uint64_t)S_CURVE_Q_ONE, (uint32_t)total);
    uint32_t half_q15 = (uint32_t)(S_CURVE_Q_ONE >> 1);

    if (u_q15 <= half_q15)
    {
        uint64_t u2_q30 = (uint64_t)u_q15 * (uint64_t)u_q15;
        uint32_t y_q15 = (uint32_t)((2ULL * u2_q30 + (1ULL << (S_CURVE_Q_SHIFT - 1))) >> S_CURVE_Q_SHIFT);
        return s_curve_u32_clamp(y_q15, 0, S_CURVE_Q_ONE);
    }
    else
    {
        uint32_t one_minus_u_q15 = (uint32_t)(S_CURVE_Q_ONE - u_q15);
        uint64_t t2_q30 = (uint64_t)one_minus_u_q15 * (uint64_t)one_minus_u_q15;
        uint32_t dec_q15 = (uint32_t)((2ULL * t2_q30 + (1ULL << (S_CURVE_Q_SHIFT - 1))) >> S_CURVE_Q_SHIFT);
        uint32_t y_q15 = (uint32_t)(S_CURVE_Q_ONE - dec_q15);
        return s_curve_u32_clamp(y_q15, 0, S_CURVE_Q_ONE);
    }
}

void s_curve_tri_acc_init(s_curve_tri_acc_t *curve, int32_t v_max, int32_t v_start, int32_t s_acc)
{
    fc_dev_assert(curve);
    fc_dev_assert(v_max > 0);
    fc_dev_assert(v_start > 0);
    fc_dev_assert(s_acc > 0);
    fc_dev_assert(v_max >= v_start);

    curve->v_max = v_max;
    curve->v_start = v_start;
    curve->s_acc = s_acc;

    curve->v_target = v_start;
    curve->s_acc_target = 0;
}

static uint64_t s_curve_tri_acc_accel_time_ns(int32_t v_max, int32_t v_start, int32_t s_acc)
{
    s_curve_tri_acc_t curve = {0};
    uint64_t          time_ns = 0;

    s_curve_tri_acc_init(&curve, v_max, v_start, s_acc);
    curve.v_target = curve.v_max;
    curve.s_acc_target = curve.s_acc;
    for (int32_t i = 0; i < s_acc; ++i)
    {
        int32_t v_now = s_curve_tri_acc_speed_by_progress(&curve, i);
        time_ns += stp_curve_step_time_ns(v_now);
    }

    return time_ns;
}

size_t s_curve_tri_acc_init_time(s_curve_tri_acc_t *curve, int32_t v_max, int32_t v_start, size_t ms)
{
    fc_dev_assert(curve);

    int32_t s_acc =
        stp_curve_find_s_acc_by_time((uint64_t)ms * 1000000ULL, v_max, v_start, s_curve_tri_acc_accel_time_ns);
    s_curve_tri_acc_init(curve, v_max, v_start, s_acc);
    return (size_t)s_acc;
}

static inline void s_curve_tri_acc_calc_start(fc_stp_base_t *stp, s_curve_tri_acc_t *curve, int32_t *v_start)
{
    int32_t s_total = FC_STP_ABS(stp->s_target - stp->s_now);

    stp->s_last = stp->s_now;  // 记录这次运动的起点

    if ((s_total >> 1) > curve->s_acc)
    {
        curve->s_acc_target = curve->s_acc;
        curve->v_target = curve->v_max;
    }
    else
    {
        curve->s_acc_target = s_total >> 1;
        if (curve->s_acc_target < 1)
        {
            curve->s_acc_target = 1;
        }

        int64_t dv = (int64_t)(curve->v_max - curve->v_start) * (int64_t)curve->s_acc_target;
        dv /= (int64_t)curve->s_acc;
        curve->v_target = curve->v_start + (int32_t)dv;
        curve->v_target = curve->v_target > curve->v_max ? curve->v_max : curve->v_target;
    }

    *v_start = curve->v_start;
}

static inline int32_t s_curve_tri_acc_speed_by_progress(s_curve_tri_acc_t *curve, int32_t s_progress)
{
    int32_t dv = curve->v_target - curve->v_start;
    if (dv <= 0)
    {
        return curve->v_start;
    }
    if (curve->s_acc_target <= 0)
    {
        return curve->v_target;
    }

    uint32_t ease_q15 = s_curve_tri_acc_ease_q15(s_progress, curve->s_acc_target);
    int32_t  v_cmd = curve->v_start + (int32_t)(((int64_t)dv * (int64_t)ease_q15 + (S_CURVE_Q_ONE >> 1)) >> S_CURVE_Q_SHIFT);
    if (v_cmd < curve->v_start)
    {
        v_cmd = curve->v_start;
    }
    if (v_cmd > curve->v_target)
    {
        v_cmd = curve->v_target;
    }
    return v_cmd;
}

static inline int32_t s_curve_tri_acc_stop_steps(s_curve_tri_acc_t *curve, int32_t v_now)
{
    if (v_now <= curve->v_start)
    {
        return 0;
    }
    if (curve->s_acc_target <= 0)
    {
        return 0;
    }
    if (v_now >= curve->v_target)
    {
        return curve->s_acc_target;
    }

    int32_t lo = 0;
    int32_t hi = curve->s_acc_target;
    while (lo < hi)
    {
        int32_t mid = lo + ((hi - lo) >> 1);
        int32_t v_mid = s_curve_tri_acc_speed_by_progress(curve, mid);
        if (v_mid < v_now)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    return lo;
}

static inline void s_curve_tri_acc_calc_next(fc_stp_base_t *stp, s_curve_tri_acc_t *curve, int32_t *v_next)
{
    int32_t s_moved = FC_STP_ABS(stp->s_now - stp->s_last);
    int32_t s_remain = FC_STP_ABS(stp->s_target - stp->s_now);
    stp->curve_hold_steps = 0;

    int32_t v_acc = s_curve_tri_acc_speed_by_progress(curve, s_moved);
    int32_t v_dec = s_curve_tri_acc_speed_by_progress(curve, s_remain);

    int32_t v_cmd = v_acc < v_dec ? v_acc : v_dec;
    v_cmd = v_cmd > curve->v_target ? curve->v_target : v_cmd;
    v_cmd = v_cmd < curve->v_start ? curve->v_start : v_cmd;
    *v_next = v_cmd;

    if (v_cmd == curve->v_target && s_remain > curve->s_acc_target)
    {
        stp_curve_set_hold_steps(stp, s_remain, curve->s_acc_target);
    }
}

static inline void s_curve_tri_acc_calc_dec(fc_stp_base_t *stp, s_curve_tri_acc_t *curve, int32_t *v_dec)
{
    int32_t s_stop = s_curve_tri_acc_stop_steps(curve, *v_dec);

    s_stop = s_stop < 1 ? 1 : s_stop;

    if (stp->s_each_increase > 0)
    {
        stp->s_target = stp->s_now + s_stop;
    }
    else
    {
        stp->s_target = stp->s_now - s_stop;
    }
}

void s_curve_tri_acc_func(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next)
{
    fc_dev_assert(stp);
    s_curve_tri_acc_t *curve = (s_curve_tri_acc_t *)stp->curve;
    fc_dev_assert(curve);

    switch (calc_type)
    {
    case FC_STP_CLAC_NEXT:
        s_curve_tri_acc_calc_next(stp, curve, v_next);
        break;

    case FC_STP_CLAC_DEC:
        s_curve_tri_acc_calc_dec(stp, curve, v_next);
        break;

    case FC_STP_CLAC_END:
        break;

    case FC_STP_CLAC_START:
        s_curve_tri_acc_calc_start(stp, curve, v_next);
        break;

    default:
        break;
    }
}
