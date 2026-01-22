// <<< Use Configuration Wizard in Context Menu >>>

/**
 * @file fc_config.h
 * @author fool_cat (2696652257@qq.com)
 * @brief fc_embed配置头文件 - 把一些关键宏配置放这里,具体的可以去相关的.h/.c文件中查看说明
 * @version 1.0
 * @date 2025-09-02
 *
 * @note 在 Keil MDK 中,点击此文件底部的 "Configuration Wizard" 标签
 *       可以通过图形界面配置所有选项
 *
 * @copyright Copyright (c) 2025
 */

#ifndef _FC_CONFIG_TEMPLATE_H_
    #define _FC_CONFIG_TEMPLATE_H_

//+********************************* 配置项说明 **********************************/
/**
 * Configuration Wizard 使用说明:
 *
 * 1. 本文件作为模版文件,移植时需定义一份fc_config.h文件,并保证文件的可包含性(#include 能找到)
 * 2. 可以旋转将本文件完整复制到fc_config.h中进行修改,也可以在fc_config.h中#include此文件
 * 3. 在 Keil MDK 中,点击 fc_config.h 文件底部的 "Configuration Wizard" 标签
 * 4. 点击编辑器底部的 "Configuration Wizard" 标签
 * 5. 通过图形界面配置所有选项 (复选框、下拉框、数值输入框)
 * 6. 保存文件后配置重新编译
 */

/**
 * =====================================================================
 * Custom Header Files Section
 * =====================================================================
 * Add your custom #include statements here if needed.
 *
 * Examples:
 *   #include "stm32f4xx_hal.h"
 *   #include "FreeRTOS.h"
 *   #include "cmsis_os.h"
 *
 * Note: Configuration Wizard does not support dynamic header inclusion.
 *       Please manually edit this section to add your headers.
 * =====================================================================
 */

    //+********************************* auto_init **********************************/

    // Add your custom includes below:
    // #include "your_header.h"

    // <h> Auto Init Configuration
    // <i> Auto initialization framework configuration

    //   <q> USE_FC_AUTO_INIT - Enable Auto Initialization
    //   <i> Enable auto-initialization before main() function
    //   <i> Default: 1 (Enabled)
    #define USE_FC_AUTO_INIT 1

// </h>

//+********************************* port **********************************/

// fc_port的锁函数宏
// #define FC_PORT_LOCK(port, rb_index, dir) ((void)0)
// #define FC_PORT_UNLOCK(port, rb_index, dir) ((void)0)

    // <h> Port Configuration
    // <i> Port and ring buffer configuration

    //   <o> PORT_RB_NUM - Number of Ring Buffers <1-16>
    //   <i> Number of ring buffers per port (similar to SEGGER RTT)
    //   <i> Default: 8
    #define PORT_RB_NUM 8

    //   <o> FIFO_TX_LOG2_SIZE - TX FIFO Size (log2) <8-16>
    //   <i> Output ring buffer size as power of 2 (e.g., 12 = 4096 bytes)
    //   <i> Default: 12 (4KB)
    #define FIFO_TX_LOG2_SIZE 12

    //   <o> STDOUT_TX_SINGLE_MAX_SHIFT - TX Single Max Shift <1-4>
    //   <i> Single TX max bytes = buffer_size / (2^n), smaller value allows more data per transfer
    //   <i> Default: 2
    #define STDOUT_TX_SINGLE_MAX_SHIFT 2

    //   <q> PHY_SERIAL_TX_ENABLE - Enable Serial TX
    //   <i> Enable continuous serial transmission
    //   <i> Default: 0 (Disabled)
    #define PHY_SERIAL_TX_ENABLE 0

    //   <o> FIFO_RX_LOG2_SIZE - RX FIFO Size (log2) <6-12>
    //   <i> Input ring buffer size as power of 2 (e.g., 8 = 256 bytes)
    //   <i> Default: 8 (256B)
    #define FIFO_RX_LOG2_SIZE 8

    //   <o> STDIN_RX_SINGLE_MAX_SHIFT - RX Single Max Shift <1-4>
    //   <i> Single RX max bytes = buffer_size / (2^n), smaller value allows more data per transfer
    //   <i> Default: 1
    #define STDIN_RX_SINGLE_MAX_SHIFT 1

    //   <q> PHY_SERIAL_RX_ENABLE - Enable Serial RX
    //   <i> Enable continuous serial reception
    //   <i> Default: 0 (Disabled)
    #define PHY_SERIAL_RX_ENABLE 0

// </h>

//+********************************* log **********************************/

// <h> Logger Configuration
// <i> Logger component configuration

//   <q> FC_LOG_ENABLE - Enable Logger
//   <i> Enable logging functionality
//   <i> Default: 1 (Enabled)
    #define FC_LOG_ENABLE 1

    //   <o> FC_LOG_LINE_SIZE - Log Line Buffer Size <32-512:8>
    //   <i> Log line buffer size in bytes
    //   <i> Default: 128
    #define FC_LOG_LINE_SIZE 128

    //   <q> FC_LOG_USING_COLOR - Enable Color Output
    //   <i> Enable ANSI color codes for colored output
    //   <i> Default: 1 (Enabled)
    #define FC_LOG_USING_COLOR 1

    //   <o> FC_LOG_POOL_TOTAL_SIZE - Log Memory Pool Size <1024-16384:256>
    //   <i> Total memory pool size for logger in bytes
    //   <i> Default: 4096 (4KB)
    #define FC_LOG_POOL_TOTAL_SIZE (4 * 1024)

// Note: The following string options are defined below but not configurable via Configuration Wizard
// You can modify them directly in code if needed:
// - FC_LOG_PREFIX_FMT: Log prefix format (default: "(%d)%s:")
// - FC_LOG_END: Log end string (default: "" or "\r\n")
// - FC_LOG_ERROR_HEAD/WARNING_HEAD/INFO_HEAD/DEBUG_HEAD/VERBOSE_HEAD: Log level prefixes

    #define FC_LOG_PREFIX_FMT ":%s->%d:\t"
    #define FC_LOG_PREFIX_CONTENT __FC_LOG_MACRO_EXPANDING(__FUNCTION__, (uint32_t)__LINE__)

    // #define FC_LOG_END ""
    // #define FC_LOG_ERROR_HEAD "E"
    // #define FC_LOG_WARNING_HEAD "W"
    // #define FC_LOG_INFO_HEAD "I"
    // #define FC_LOG_DEBUG_HEAD "D"
    // #define FC_LOG_VERBOSE_HEAD "V"

    // </h>

    //+********************************* pool **********************************/

    // <h> Memory Pool Configuration
    // <i> Memory pool configuration

    //   <q> FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC - Enable Dynamic Allocation
    //   <i> Allow memory pool to use dynamic memory (malloc/free) when static memory is exhausted
    //   <i> Default: 0 (Disabled)
    #define FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC 0

    // </h>

    //+********************************* 基于日志的传输层 **********************************/

    // <h> Transport Configuration
    // <i> Transport component configuration

    //   <o> FC_DIVISION_NUM_MAX_LEN - Port Number Max Length <1-8>
    //   <i> Maximum length of port number in paging information
    //   <i> Default: 4 (supports 0-9999)
    #define FC_DIVISION_NUM_MAX_LEN 4

    //   <o> FC_DIVISION_NUM_MAX - Port Number Max Value <99-99999999>
    //   <i> Maximum value of port number, must match FC_DIVISION_NUM_MAX_LEN
    //   <i> Default: 9999
    #define FC_DIVISION_NUM_MAX 9999

    // </h>

    //+********************************* stdio **********************************/

    // <h> StdIO Configuration
    // <i> Standard I/O configuration

    //   <q> XF_USE_LLI - Enable Long Long Integer Support
    //   <i> Support %lld, %llu for 64-bit integer formatting
    //   <i> Default: 1 (Enabled)
    #define XF_USE_LLI 1

    //   <q> XF_USE_FP - Enable Floating Point Support
    //   <i> Support %f, %e, %E for floating point formatting (increases code size significantly)
    //   <i> Default: 1 (Enabled)
    #define XF_USE_FP 1

// </h>

#endif  //\ _FC_CONFIG_TEMPLATE_H_

// <<< end of configuration section >>>
