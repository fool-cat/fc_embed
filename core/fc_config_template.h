// <<< Use Configuration Wizard in Context Menu >>>

/**
 * @file fc_config.h
 * @author fool_cat (2696652257@qq.com)
 * @brief fc_embed配置头文件 - 所有可配置项的集中管理
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

    // Add your custom includes below:
    // #include "your_header.h"

    // <h> Auto Init Configuration
    // <i> Auto initialization framework configuration

    //   <q> USE_FC_AUTO_INIT - Enable Auto Initialization
    //   <i> Enable auto-initialization before main() function
    //   <i> Default: 1 (Enabled)
    #define USE_FC_AUTO_INIT 1

    // </h>

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

    //   <o> FC_LOG_STACK_LINE_SIZE - Log Stack Line Size <32-512:8>
    //   <i> Log buffer size on stack in bytes
    //   <i> Default: FC_LOG_LINE_SIZE
    #define FC_LOG_STACK_LINE_SIZE (FC_LOG_LINE_SIZE)

    //   <q> FC_LOG_USING_COLOR - Enable Color Output
    //   <i> Enable ANSI color codes for colored output
    //   <i> Default: 1 (Enabled)
    #define FC_LOG_USING_COLOR 1

    //   <q> FC_LOG_NOPREFIX_API - Enable No-Prefix API
    //   <i> Provide simplified log macros without fc_ prefix (log_debug, log_info, etc.)
    //   <i> Default: 1 (Enabled)
    #define FC_LOG_NOPREFIX_API 1

    //   <o> FC_LOG_POOL_TOTAL_SIZE - Log Memory Pool Size <1024-16384:256>
    //   <i> Total memory pool size for logger in bytes
    //   <i> Default: 4096 (4KB)
    #define FC_LOG_POOL_TOTAL_SIZE (4 * 1024)

    //   <o> FC_LOG_ALLOC_BLOCK_SIZE - Log Allocation Block Size <64-512:64>
    //   <i> Memory pool block size in bytes
    //   <i> Default: 128
    #define FC_LOG_ALLOC_BLOCK_SIZE 128

// Note: The following string options are defined below but not configurable via Configuration Wizard
// You can modify them directly in code if needed:
// - FC_LOG_PREFIX_FMT: Log prefix format (default: "(%d)%s:")
// - FC_LOG_END: Log end string (default: "" or "\r\n")
// - FC_LOG_ERROR_HEAD/WARNING_HEAD/INFO_HEAD/DEBUG_HEAD/VERBOSE_HEAD: Log level prefixes

    #define FC_LOG_PREFIX_FMT "(%d)%s:"
    #define FC_LOG_END ""
    #define FC_LOG_ERROR_HEAD "E"
    #define FC_LOG_WARNING_HEAD "W"
    #define FC_LOG_INFO_HEAD "I"
    #define FC_LOG_DEBUG_HEAD "D"
    #define FC_LOG_VERBOSE_HEAD "V"

    // </h>

    // <h> Memory Pool Configuration
    // <i> Memory pool configuration

    //   <q> FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC - Enable Dynamic Allocation
    //   <i> Allow memory pool to use dynamic memory (malloc/free) when static memory is exhausted
    //   <i> Default: 0 (Disabled)
    #define FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC 0

    // </h>

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

    //   <o> SZB_OUTPUT - Output Buffer Size <16-128:8>
    //   <i> Internal buffer size for printf/sprintf functions
    //   <i> Default: 32
    #define SZB_OUTPUT 32

    //   <q> FC_FIFO_VPRINTF_LINEAR_WRITE - Enable Linear Write Mode
    //   <i> Use linear write mode for better performance
    //   <i> Default: 1 (Enabled)
    #define FC_FIFO_VPRINTF_LINEAR_WRITE 1

    // Note: XF_DPC (Decimal Point Character) is defined below
    // Modify directly in code if you need a different decimal separator
    #define XF_DPC '.'

    // </h>

    // <h> FIFO Configuration
    // <i> FIFO (ring buffer) configuration

    //   <q> FC_USE_STD_MEMCPY - Use Standard memcpy
    //   <i> Use standard library memcpy (0 = use byte-by-byte copy)
    //   <i> Default: 1 (Enabled)
    #define FC_USE_STD_MEMCPY 1

// </h>

//+********************************* 高级配置 (通常不需要修改) **********************************/
/**
 * Advanced Configuration - Normally no need to modify
 *
 * The following macros can be overridden in your project if needed:
 *
 * - fc_assert(x)                     - Runtime assertion
 * - FC_WAIT_MOMENT()                 - Wait/delay function
 * - FC_PORT_LOCK(port, rb, dir)      - Port lock function
 * - FC_PORT_UNLOCK(port, rb, dir)    - Port unlock function
 * - FC_PORT_LOSE_HOOK(...)           - Data loss hook
 * - FC_LOG_OBJ                       - Log output object
 * - FC_LOG_LOSE_HOOK(...)            - Log data loss hook
 * - FC_LOG_PREFIX_CONTENT            - Log prefix content macro
 * - FC_STDOUT_OBJ                    - Standard output object
 * - FC_STDOUT_RB_INDEX               - Standard output ring buffer index
 * - FC_STDIN_OBJ                     - Standard input object
 * - FC_STDIN_RB_INDEX                - Standard input ring buffer index
 * - FC_POOL_ATOMIC_ENTER(obj)        - Pool atomic section enter
 * - FC_POOL_ATOMIC_EXIT(obj)         - Pool atomic section exit
 * - FC_POOL_MALLOC(size)             - Pool malloc function
 * - FC_POOL_FREE(ptr)                - Pool free function
 * - fc_fifo_assert(x)                - FIFO assertion
 * - fc_fifo_memcpy(dst, src, size)   - FIFO memory copy function
 *
 * Define these macros in your project before including fc_embed headers if you need
 * custom implementations.
 */

//+********************************* 配置项说明 **********************************/
/**
 * Configuration Wizard 使用说明:
 *
 * 1. 在 Keil MDK 中打开本文件
 * 2. 点击编辑器底部的 "Configuration Wizard" 标签
 * 3. 通过图形界面配置所有选项 (复选框、下拉框、数值输入框)
 * 4. 保存文件后配置立即生效
 *
 * 配置优先级:
 * 1. 工程中自定义的 fc_config.h (最高优先级)
 * 2. 本模板文件中的配置
 * 3. 源代码中的默认值 (最低优先级)
 *
 * 使用方法:
 * 1. 复制本文件并重命名为 fc_config.h
 * 2. 使用 Configuration Wizard 修改配置
 * 3. 将 fc_config.h 添加到工程的包含路径
 */

#endif  //\ _FC_CONFIG_TEMPLATE_H_

// <<< end of configuration section >>>
