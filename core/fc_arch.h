/****************************************************************************
 *  Copyright 2024 Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/**
 * @brief 摘自perf_counter,平台原子操作实现部分
 *
 */

/*============================ INCLUDES ======================================*/

#include "fc_config.h"

#ifndef __FC_CFG_DISABLE_DEFAULT_ARCH_PORTING__
    #define __FC_CFG_DISABLE_DEFAULT_ARCH_PORTING__ 0
#endif

#if !__FC_CFG_DISABLE_DEFAULT_ARCH_PORTING__

    // > 单次包含宏定义
    #ifndef _FC_ARCH_H_
        #define _FC_ARCH_H_

        #include "cmsis_compiler.h"

        #include "fc_helper.h"

    /*============================ MACROS ========================================*/
    /*============================ MACROFIED FUNCTIONS ===========================*/

        #ifndef __fc_sync_barrier__
            #define __fc_sync_barrier__(...) \
                do                           \
                {                            \
                    __DSB();                 \
                    __ISB();                 \
                } while (0)
        #endif

        #ifndef __STATIC_INLINE
            #define __STATIC_INLINE static inline
        #endif

/*============================ TYPES =========================================*/
typedef uint32_t fc_global_interrupt_status_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

__STATIC_INLINE
fc_global_interrupt_status_t fc_disable_global_interrupt(void)
{
    fc_global_interrupt_status_t tStatus = __get_PRIMASK();
    __disable_irq();

    return tStatus;
}

__STATIC_INLINE
void fc_resume_global_interrupt(fc_global_interrupt_status_t tStatus)
{
    __set_PRIMASK(tStatus);
}

        #ifndef FC_ATOMIC_SCOPE
            #define FC_ATOMIC_SCOPE                                     \
                fc_using(fc_global_interrupt_status_t SAFE_NAME(temp) = \
                             fc_disable_global_interrupt(),             \
                         fc_resume_global_interrupt(SAFE_NAME(temp)))
        #endif

    #endif  //\ _FC_ARCH_H_

#endif
