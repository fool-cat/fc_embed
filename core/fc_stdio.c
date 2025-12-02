/**
 * @file fc_stdio.c
 * @author fool_cat (2696652257@qq.com)
 * @brief 统一编译
 * @version 1.0
 * @date 2025-02-20
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "fc_stdio.h"

//+********************************* 格式化API **********************************/

// sprintf
#include "./utils/fc_sprintf.c"

// snprintf
#include "./utils/fc_snprintf.c"

// vsprintf
#include "./utils/fc_vsprintf.c"

// vsnprintf
#include "./utils/fc_vsnprintf.c"

// fprintf
#include "./utils/fc_fprintf.c"

// vfprintf
#include "./utils/fc_vfprintf.c"  // core function

//+********************************* port API **********************************/

#include "./utils/fc_port_vprintf.c"
