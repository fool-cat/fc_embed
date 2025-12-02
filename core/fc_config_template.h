/**
 * @file fc_config.h
 * @author fool_cat (2696652257@qq.com)
 * @brief fc_embed配置头文件
 * @version 1.0
 * @date 2025-09-02
 *
 * @copyright Copyright (c) 2025
 *
 */

// > 单次包含宏定义
#ifndef _FC_CONFIG_H_
#define _FC_CONFIG_H_

// 直接卡死在断言失败的位置,方便调试
/* dynamic assert */
#define fc_assert(exp) \
    do                 \
    {                  \
        if (!(exp))    \
            while (1)  \
            {          \
            }          \
    } while (0)

#endif  //\ _FC_CONFIG_H_
