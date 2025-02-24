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

// snprintf
#include "fc_snprintf.c"

// sprintf
#include "fc_sprintf.c"

// vfprintf
#include "fc_vfprintf.c"  // core function

// vsnprintf
#include "fc_vsnprintf.c"

// vsprintf
#include "fc_vsprintf.c"

// clang-format off

// 根据RTE组件配置，选择是否使用标准IO
// 如果配置了RTE组件,会全局定义这个宏
#ifdef _RTE_

#include "RTE_header.h"

// > C/C++兼容性宏定义
#ifdef __cplusplus
extern "C"
{
#endif
    int stdout_putchar(int ch);
    int stdin_getchar(void);
    int stderr_putchar(int ch);
#ifdef __cplusplus
}
#endif  //\ __cplusplus

#ifdef RTE_Compiler_IO_STDOUT_User
/*printf*/
int stdout_putchar(int ch)
{
    return fc_port_putc(&fc_stdout, ch);
}
#endif

#ifdef RTE_Compiler_IO_STDIN_User
/*scanf*/
int stdin_getchar(void)
{
    return fc_port_getc(&fc_stdin);
}
#endif

#ifdef RTE_Compiler_IO_STDERR_User
/*assert*/
int stderr_putchar(int ch)
{
    return fc_port_putc(&fc_stdout, ch);
}
#endif

#endif //\ _RTE_

// clang-format on
