/**
 * @file fc_type.h
 * @author fool_cat (2696652257@qq.com)
 * @brief 自定义类型
 * @version 1.0
 * @date 2026-03-06
 *
 * @copyright Copyright (c) 2026
 *
 */

// > 单次包含宏定义
#ifndef _FC_TYPE_H_
#define _FC_TYPE_H_

#include <stdint.h>
#include <stdbool.h>

#include "fc_config.h"

#ifndef fc_time_t
    #define fc_time_t int64_t
#endif  // !fc_time_t

#ifndef fc_get_ms
    #define fc_get_ms() get_system_ms()
#endif  // ! fc_get_ms

#ifndef fc_get_time
    #define fc_get_time() fc_get_ms()
#endif

#ifndef fc_dev_assert
    #define fc_dev_assert(x) ((void)(0))
#endif  //\ fc_dev_assert

#endif  //\ _FC_TYPE_H_
