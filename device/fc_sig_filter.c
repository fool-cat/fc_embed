#include "fc_sig_filter.h"

static int32_t fc_sig_filter_popcount_u32(uint32_t value)
{
    int32_t count = 0;
    while (value)
    {
        count += (int32_t)(value & 0x01u);
        value >>= 1;
    }
    return count;
}

//+*********************************  **********************************/

void fc_sig_filter_scale_init(fc_sig_scale_filter_t *filter, int32_t count_repeat, int32_t count_critical)
{
    fc_dev_assert(filter != NULL);
    fc_dev_assert(count_repeat > 0);
    fc_dev_assert(count_critical > 0);
    fc_dev_assert(count_critical > count_repeat);

    filter->count_repeat = count_repeat;
    filter->count_critical = count_critical;
    filter->count_valid = 0;
    filter->count_invalid = 0;
}

fc_sig_state_t fc_sig_filter_scale(void *filter, fc_sig_t *sig, fc_sig_state_t state)
{
    fc_dev_assert(sig != NULL);
    fc_sig_scale_filter_t *scale = (fc_sig_scale_filter_t *)filter;
    fc_dev_assert(scale != NULL);

    if (FC_SIGNAL_STATE_VALID == state)
    {
        scale->count_valid++;
        scale->count_invalid--;  //> 有一次有效则减一

        if (scale->count_invalid < 0)  //> 保证计数不会小于0
        {
            scale->count_invalid = 0;
        }

        if (scale->count_valid > scale->count_critical)  //> 保证计数不会超过临界计数
        {
            scale->count_valid = scale->count_critical;
            return FC_SIGNAL_STATE_VALID;  // 直接返回优化, 避免下面的判断浪费算力
        }
    }
    else
    {
        scale->count_valid--;
        scale->count_invalid++;

        if (scale->count_valid < 0)
        {
            scale->count_valid = 0;
        }

        if (scale->count_invalid > scale->count_critical)
        {
            scale->count_invalid = scale->count_critical;
            return FC_SIGNAL_STATE_INVALID;  // 直接返回优化, 避免下面的判断浪费算力
        }
    }

    if (scale->count_valid + scale->count_invalid >= scale->count_critical)
    {
        if (scale->count_valid >= scale->count_repeat && FC_SIGNAL_STATE_VALID == state)
        {
            return FC_SIGNAL_STATE_VALID;
        }
        else if (scale->count_invalid >= scale->count_repeat && FC_SIGNAL_STATE_INVALID == state)
        {
            return FC_SIGNAL_STATE_INVALID;
        }
    }

    return sig->state_steady;
}

//+*********************************  **********************************/

void fc_sig_filter_repeat_init(fc_sig_repeat_filter_t *filter, int32_t count_repeat)
{
    fc_dev_assert(filter != NULL);
    fc_dev_assert(count_repeat > 0);

    filter->count_repeat = count_repeat;
    filter->count_valid = 0;
    filter->count_invalid = 0;
}

fc_sig_state_t fc_sig_filter_repeat(void *filter, fc_sig_t *sig, fc_sig_state_t state)
{
    fc_dev_assert(sig != NULL);
    fc_sig_repeat_filter_t *repeat = (fc_sig_repeat_filter_t *)filter;
    fc_dev_assert(repeat != NULL);

    if (FC_SIGNAL_STATE_VALID == state)
    {
        repeat->count_valid++;
        repeat->count_invalid = 0;                       //> 有一次有效就清零
        if (repeat->count_valid > repeat->count_repeat)  //> 保证总计数不会超过重复计数
        {
            repeat->count_valid = repeat->count_repeat;
            return FC_SIGNAL_STATE_VALID;
        }
    }
    else
    {
        repeat->count_valid = 0;
        repeat->count_invalid++;
        if (repeat->count_invalid > repeat->count_repeat)  //> 保证总计数不会超过重复计数
        {
            repeat->count_invalid = repeat->count_repeat;
            return FC_SIGNAL_STATE_INVALID;
        }
    }

    if (repeat->count_valid + repeat->count_invalid >= repeat->count_repeat)
    {
        return state;  // 当前状态就是稳态
    }

    return sig->state_steady;
}

//+*********************************  **********************************/

void fc_sig_filter_asym_repeat_init(fc_sig_asym_repeat_filter_t *filter, int32_t count_valid_repeat, int32_t count_invalid_repeat)
{
    fc_dev_assert(filter != NULL);
    fc_dev_assert(count_valid_repeat > 0);
    fc_dev_assert(count_invalid_repeat > 0);

    filter->count_valid_repeat = count_valid_repeat;
    filter->count_invalid_repeat = count_invalid_repeat;
    filter->count_valid = 0;
    filter->count_invalid = 0;
}

fc_sig_state_t fc_sig_filter_asym_repeat(void *filter, fc_sig_t *sig, fc_sig_state_t state)
{
    fc_dev_assert(sig != NULL);
    fc_sig_asym_repeat_filter_t *repeat = (fc_sig_asym_repeat_filter_t *)filter;
    fc_dev_assert(repeat != NULL);

    if (FC_SIGNAL_STATE_VALID == state)
    {
        repeat->count_valid++;
        repeat->count_invalid = 0;

        if (repeat->count_valid >= repeat->count_valid_repeat)
        {
            repeat->count_valid = repeat->count_valid_repeat;
            return FC_SIGNAL_STATE_VALID;
        }
    }
    else
    {
        repeat->count_valid = 0;
        repeat->count_invalid++;

        if (repeat->count_invalid >= repeat->count_invalid_repeat)
        {
            repeat->count_invalid = repeat->count_invalid_repeat;
            return FC_SIGNAL_STATE_INVALID;
        }
    }

    return sig->state_steady;
}

//+*********************************  **********************************/

void fc_sig_filter_integrator_init(fc_sig_integrator_filter_t *filter, int32_t count_max, int32_t count_threshold, fc_sig_state_t init_state)
{
    fc_dev_assert(filter != NULL);
    fc_dev_assert(count_max > 0);
    fc_dev_assert(count_threshold > 0);
    fc_dev_assert(count_threshold < count_max);
    fc_dev_assert((count_threshold * 2) > count_max);

    filter->count_max = count_max;
    filter->count_threshold = count_threshold;

    if (FC_SIGNAL_STATE_VALID == init_state)
    {
        filter->accumulator = count_max;
    }
    else
    {
        filter->accumulator = 0;
    }
}

fc_sig_state_t fc_sig_filter_integrator(void *filter, fc_sig_t *sig, fc_sig_state_t state)
{
    fc_dev_assert(sig != NULL);
    fc_sig_integrator_filter_t *integrator = (fc_sig_integrator_filter_t *)filter;
    fc_dev_assert(integrator != NULL);

    if (FC_SIGNAL_STATE_VALID == state)
    {
        integrator->accumulator++;
        if (integrator->accumulator > integrator->count_max)
        {
            integrator->accumulator = integrator->count_max;
        }
    }
    else
    {
        integrator->accumulator--;
        if (integrator->accumulator < 0)
        {
            integrator->accumulator = 0;
        }
    }

    if (integrator->accumulator >= integrator->count_threshold)
    {
        return FC_SIGNAL_STATE_VALID;
    }

    if (integrator->accumulator <= (integrator->count_max - integrator->count_threshold))
    {
        return FC_SIGNAL_STATE_INVALID;
    }

    return sig->state_steady;
}

//+*********************************  **********************************/

void fc_sig_filter_window_init(fc_sig_window_filter_t *filter, int32_t count_window, int32_t count_threshold)
{
    fc_dev_assert(filter != NULL);
    fc_dev_assert(count_window > 0);
    fc_dev_assert(count_window <= 32);
    fc_dev_assert(count_threshold > 0);
    fc_dev_assert(count_threshold <= count_window);
    fc_dev_assert((count_threshold * 2) > count_window);

    filter->count_window = count_window;
    filter->count_threshold = count_threshold;
    filter->count_sample = 0;
    filter->history = 0;
}

fc_sig_state_t fc_sig_filter_window(void *filter, fc_sig_t *sig, fc_sig_state_t state)
{
    fc_dev_assert(sig != NULL);
    fc_sig_window_filter_t *window = (fc_sig_window_filter_t *)filter;
    fc_dev_assert(window != NULL);

    uint32_t mask;
    int32_t  count_valid;

    if (window->count_window >= 32)
    {
        mask = 0xffffffffu;
    }
    else
    {
        mask = (1u << window->count_window) - 1u;
    }

    window->history <<= 1;
    if (FC_SIGNAL_STATE_VALID == state)
    {
        window->history |= 0x01u;
    }
    window->history &= mask;

    if (window->count_sample < window->count_window)
    {
        window->count_sample++;
    }

    if (window->count_sample < window->count_window)
    {
        return sig->state_steady;
    }

    count_valid = fc_sig_filter_popcount_u32(window->history);
    if (count_valid >= window->count_threshold)
    {
        return FC_SIGNAL_STATE_VALID;
    }

    if ((window->count_window - count_valid) >= window->count_threshold)
    {
        return FC_SIGNAL_STATE_INVALID;
    }

    return sig->state_steady;
}
