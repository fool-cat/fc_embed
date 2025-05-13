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
#include "fc_sprintf.c"

// snprintf
#include "fc_snprintf.c"

// vsprintf
#include "fc_vsprintf.c"

// vsnprintf
#include "fc_vsnprintf.c"

// fprintf
#include "fc_fprintf.c"

// vfprintf
#include "fc_vfprintf.c"  // core function

//+********************************* 格式化解析API **********************************/

#if 0

// sscanf
    #include "fc_sscanf.c"

// vscanf
    #include "fc_vscanf.c"

// vsscanf
    #include "fc_vsscanf.c"

// fscanf
    #include "fc_fscanf.c"

// vfscanf
    #include "fc_vfscanf.c"  // core function

#endif

//+********************************* port API **********************************/

#include "fc_port_vprintf.c"

// #include "fc_port_vscanf.c"
