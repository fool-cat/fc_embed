/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2021 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER RTT * Real Time Transfer for embedded targets         *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the RTT protocol and J-Link.                       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
*                                                                    *
*       RTT version: 8.20                                           *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------
File    : SEGGER_RTT_Conf.h
Purpose : Implementation of SEGGER real-time transfer (RTT) which
          allows real-time communication on targets which support
          debugger memory accesses while the CPU is running.
Revision: $Rev: 24316 $

*/

/**
 * @file fc_rtt_helper.h
 * @author fool-cat (2696652257@qq.com)
 * @brief 拷贝自SEGGER_RTT_Conf.h和SEGGER_RTT.h
 * @version 1.0
 * @date 2025-05-18
 *
 * @copyright Copyright (c) 2025
 *
 */

// clang-format off

// > 单次包含宏定义
#ifndef _FC_RTT_HELPER_H_
#define _FC_RTT_HELPER_H_

#ifdef __IAR_SYSTEMS_ICC__
  #include <intrinsics.h>
#endif
//
// Target is not allowed to perform other RTT operations while string still has not been stored completely.
// Otherwise we would probably end up with a mixed string in the buffer.
// If using  RTT from within interrupts, multiple tasks or multi processors, define the SEGGER_RTT_LOCK() and SEGGER_RTT_UNLOCK() function here.
//
// SEGGER_RTT_MAX_INTERRUPT_PRIORITY can be used in the sample lock routines on Cortex-M3/4.
// Make sure to mask all interrupts which can send RTT data, i.e. generate SystemView events, or cause task switches.
// When high-priority interrupts must not be masked while sending RTT data, SEGGER_RTT_MAX_INTERRUPT_PRIORITY needs to be adjusted accordingly.
// (Higher priority = lower priority number)
// Default value for embOS: 128u
// Default configuration in FreeRTOS: configMAX_SYSCALL_INTERRUPT_PRIORITY: ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
// In case of doubt mask all interrupts: 1 << (8 - BASEPRI_PRIO_BITS) i.e. 1 << 5 when 3 bits are implemented in NVIC
// or define SEGGER_RTT_LOCK() to completely disable interrupts.
//
#ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY
  #define SEGGER_RTT_MAX_INTERRUPT_PRIORITY         (0x20)   // Interrupt priority to lock on SEGGER_RTT_LOCK on Cortex-M3/4 (Default: 0x20)
#endif

/*********************************************************************
*
*       RTT lock configuration for SEGGER Embedded Studio,
*       Rowley CrossStudio and GCC
*/
#if ((defined(__SES_ARM) || defined(__SES_RISCV) || defined(__CROSSWORKS_ARM) || defined(__GNUC__) || defined(__clang__)) && !defined (__CC_ARM) && !defined(WIN32))
  #if (defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__))
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                    unsigned int _SEGGER_RTT__LockState;                                         \
                                  __asm volatile ("mrs   %0, primask  \n\t"                         \
                                                  "movs  r1, #1       \n\t"                         \
                                                  "msr   primask, r1  \n\t"                         \
                                                  : "=r" (_SEGGER_RTT__LockState)                                \
                                                  :                                                 \
                                                  : "r1", "cc"                                      \
                                                  );

    #define SEGGER_RTT_UNLOCK()   __asm volatile ("msr   primask, %0  \n\t"                         \
                                                  :                                                 \
                                                  : "r" (_SEGGER_RTT__LockState)                                 \
                                                  :                                                 \
                                                  );                                                \
                                }
  #elif (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8_1M_MAIN__))
    #ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY
      #define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                    unsigned int _SEGGER_RTT__LockState;                                         \
                                  __asm volatile ("mrs   %0, basepri  \n\t"                         \
                                                  "mov   r1, %1       \n\t"                         \
                                                  "msr   basepri, r1  \n\t"                         \
                                                  : "=r" (_SEGGER_RTT__LockState)                                \
                                                  : "i"(SEGGER_RTT_MAX_INTERRUPT_PRIORITY)          \
                                                  : "r1", "cc"                                      \
                                                  );

    #define SEGGER_RTT_UNLOCK()   __asm volatile ("msr   basepri, %0  \n\t"                         \
                                                  :                                                 \
                                                  : "r" (_SEGGER_RTT__LockState)                                 \
                                                  :                                                 \
                                                  );                                                \
                                }

  #elif (defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__))
    #define SEGGER_RTT_LOCK() {                                                \
                                 unsigned int _SEGGER_RTT__LockState;                       \
                                 __asm volatile ("mrs r1, CPSR \n\t"           \
                                                 "mov %0, r1 \n\t"             \
                                                 "orr r1, r1, #0xC0 \n\t"      \
                                                 "msr CPSR_c, r1 \n\t"         \
                                                 : "=r" (_SEGGER_RTT__LockState)            \
                                                 :                             \
                                                 : "r1", "cc"                  \
                                                 );

    #define SEGGER_RTT_UNLOCK() __asm volatile ("mov r0, %0 \n\t"              \
                                                "mrs r1, CPSR \n\t"            \
                                                "bic r1, r1, #0xC0 \n\t"       \
                                                "and r0, r0, #0xC0 \n\t"       \
                                                "orr r1, r1, r0 \n\t"          \
                                                "msr CPSR_c, r1 \n\t"          \
                                                :                              \
                                                : "r" (_SEGGER_RTT__LockState)              \
                                                : "r0", "r1", "cc"             \
                                                );                             \
                            }
  #elif defined(__riscv) || defined(__riscv_xlen)
    #define SEGGER_RTT_LOCK()  {                                               \
                                 unsigned int _SEGGER_RTT__LockState;                       \
                                 __asm volatile ("csrr  %0, mstatus  \n\t"     \
                                                 "csrci mstatus, 8   \n\t"     \
                                                 "andi  %0, %0,  8   \n\t"     \
                                                 : "=r" (_SEGGER_RTT__LockState)            \
                                                 :                             \
                                                 :                             \
                                                );

  #define SEGGER_RTT_UNLOCK()    __asm volatile ("csrr  a1, mstatus  \n\t"     \
                                                 "or    %0, %0, a1   \n\t"     \
                                                 "csrs  mstatus, %0  \n\t"     \
                                                 :                             \
                                                 : "r"  (_SEGGER_RTT__LockState)            \
                                                 : "a1"                        \
                                                );                             \
                               }
  #else
    #define SEGGER_RTT_LOCK()
    #define SEGGER_RTT_UNLOCK()
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for IAR EWARM
*/
#ifdef __ICCARM__
  #if (defined (__ARM6M__)          && (__CORE__ == __ARM6M__))             ||                      \
      (defined (__ARM8M_BASELINE__) && (__CORE__ == __ARM8M_BASELINE__))
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                  unsigned int _SEGGER_RTT__LockState;                                           \
                                  _SEGGER_RTT__LockState = __get_PRIMASK();                                      \
                                  __set_PRIMASK(1);

    #define SEGGER_RTT_UNLOCK()   __set_PRIMASK(_SEGGER_RTT__LockState);                                         \
                                }
  #elif (defined (__ARM7EM__)         && (__CORE__ == __ARM7EM__))          ||                      \
        (defined (__ARM7M__)          && (__CORE__ == __ARM7M__))           ||                      \
        (defined (__ARM8M_MAINLINE__) && (__CORE__ == __ARM8M_MAINLINE__))  ||                      \
        (defined (__ARM8M_MAINLINE__) && (__CORE__ == __ARM8M_MAINLINE__))
    #ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY
      #define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                  unsigned int _SEGGER_RTT__LockState;                                           \
                                  _SEGGER_RTT__LockState = __get_BASEPRI();                                      \
                                  __set_BASEPRI(SEGGER_RTT_MAX_INTERRUPT_PRIORITY);

    #define SEGGER_RTT_UNLOCK()   __set_BASEPRI(_SEGGER_RTT__LockState);                                         \
                                }
  #elif (defined (__ARM7A__) && (__CORE__ == __ARM7A__))                    ||                      \
        (defined (__ARM7R__) && (__CORE__ == __ARM7R__))
    #define SEGGER_RTT_LOCK() {                                                                     \
                                 unsigned int _SEGGER_RTT__LockState;                                            \
                                 __asm volatile ("mrs r1, CPSR \n\t"                                \
                                                 "mov %0, r1 \n\t"                                  \
                                                 "orr r1, r1, #0xC0 \n\t"                           \
                                                 "msr CPSR_c, r1 \n\t"                              \
                                                 : "=r" (_SEGGER_RTT__LockState)                                 \
                                                 :                                                  \
                                                 : "r1", "cc"                                       \
                                                 );

    #define SEGGER_RTT_UNLOCK() __asm volatile ("mov r0, %0 \n\t"                                   \
                                                "mrs r1, CPSR \n\t"                                 \
                                                "bic r1, r1, #0xC0 \n\t"                            \
                                                "and r0, r0, #0xC0 \n\t"                            \
                                                "orr r1, r1, r0 \n\t"                               \
                                                "msr CPSR_c, r1 \n\t"                               \
                                                :                                                   \
                                                : "r" (_SEGGER_RTT__LockState)                                   \
                                                : "r0", "r1", "cc"                                  \
                                                );                                                  \
                            }
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for IAR RX
*/
#ifdef __ICCRX__
  #define SEGGER_RTT_LOCK()   {                                                                     \
                                unsigned long _SEGGER_RTT__LockState;                                            \
                                _SEGGER_RTT__LockState = __get_interrupt_state();                                \
                                __disable_interrupt();

  #define SEGGER_RTT_UNLOCK()   __set_interrupt_state(_SEGGER_RTT__LockState);                                   \
                              }
#endif

/*********************************************************************
*
*       RTT lock configuration for IAR RL78
*/
#ifdef __ICCRL78__
  #define SEGGER_RTT_LOCK()   {                                                                     \
                                __istate_t _SEGGER_RTT__LockState;                                               \
                                _SEGGER_RTT__LockState = __get_interrupt_state();                                \
                                __disable_interrupt();

  #define SEGGER_RTT_UNLOCK()   __set_interrupt_state(_SEGGER_RTT__LockState);                                   \
                              }
#endif

/*********************************************************************
*
*       RTT lock configuration for KEIL ARM
*/
#ifdef __CC_ARM
  #if (defined __TARGET_ARCH_6S_M)
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                  unsigned int _SEGGER_RTT__LockState;                                           \
                                  register unsigned char _SEGGER_RTT__PRIMASK __asm( "primask");                 \
                                  _SEGGER_RTT__LockState = _SEGGER_RTT__PRIMASK;                                              \
                                  _SEGGER_RTT__PRIMASK = 1u;                                                     \
                                  __schedule_barrier();

    #define SEGGER_RTT_UNLOCK()   _SEGGER_RTT__PRIMASK = _SEGGER_RTT__LockState;                                              \
                                  __schedule_barrier();                                             \
                                }
  #elif (defined(__TARGET_ARCH_7_M) || defined(__TARGET_ARCH_7E_M))
    #ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY
      #define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                  unsigned int _SEGGER_RTT__LockState;                                           \
                                  register unsigned char BASEPRI __asm( "basepri");                 \
                                  _SEGGER_RTT__LockState = BASEPRI;                                              \
                                  BASEPRI = SEGGER_RTT_MAX_INTERRUPT_PRIORITY;                      \
                                  __schedule_barrier();

    #define SEGGER_RTT_UNLOCK()   BASEPRI = _SEGGER_RTT__LockState;                                              \
                                  __schedule_barrier();                                             \
                                }
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for TI ARM
*/
#ifdef __TI_ARM__
  #if defined (__TI_ARM_V6M0__)
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                  unsigned int _SEGGER_RTT__LockState;                                           \
                                  _SEGGER_RTT__LockState = __get_PRIMASK();                                      \
                                  __set_PRIMASK(1);

    #define SEGGER_RTT_UNLOCK()   __set_PRIMASK(_SEGGER_RTT__LockState);                                         \
                                }
  #elif (defined (__TI_ARM_V7M3__) || defined (__TI_ARM_V7M4__))
    #ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY
      #define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)
    #endif
    #define SEGGER_RTT_LOCK()   {                                                                   \
                                  unsigned int _SEGGER_RTT__LockState;                                           \
                                  _SEGGER_RTT__LockState = _set_interrupt_priority(SEGGER_RTT_MAX_INTERRUPT_PRIORITY);

    #define SEGGER_RTT_UNLOCK()   _set_interrupt_priority(_SEGGER_RTT__LockState);                               \
                                }
  #endif
#endif

/*********************************************************************
*
*       RTT lock configuration for CCRX
*/
#ifdef __RX
  #include <machine.h>
  #define SEGGER_RTT_LOCK()   {                                                                     \
                                unsigned long _SEGGER_RTT__LockState;                                            \
                                _SEGGER_RTT__LockState = get_psw() & 0x010000;                                   \
                                clrpsw_i();

  #define SEGGER_RTT_UNLOCK()   set_psw(get_psw() | _SEGGER_RTT__LockState);                                     \
                              }
#endif

/*********************************************************************
*
*       RTT lock configuration for embOS Simulation on Windows
*       (Can also be used for generic RTT locking with embOS)
*/
#if defined(WIN32) || defined(SEGGER_RTT_LOCK_EMBOS)

void OS_SIM_EnterCriticalSection(void);
void OS_SIM_LeaveCriticalSection(void);

#define SEGGER_RTT_LOCK()       {                                                                   \
                                  OS_SIM_EnterCriticalSection();

#define SEGGER_RTT_UNLOCK()       OS_SIM_LeaveCriticalSection();                                    \
                                }
#endif

/*********************************************************************
*
*       RTT lock configuration fallback
*/
#ifndef   SEGGER_RTT_LOCK
  #define SEGGER_RTT_LOCK()                // Lock RTT (nestable)   (i.e. disable interrupts)
#endif

#ifndef   SEGGER_RTT_UNLOCK
  #define SEGGER_RTT_UNLOCK()              // Unlock RTT (nestable) (i.e. enable previous interrupt lock state)
#endif

//+*********************************  **********************************/

#ifndef RTT_USE_ASM
  //
  // Some cores support out-of-order memory accesses (reordering of memory accesses in the core)
  // For such cores, we need to define a memory barrier to guarantee the order of certain accesses to the RTT ring buffers.
  // Needed for:
  //   Cortex-M7 (ARMv7-M)
  //   Cortex-M23 (ARM-v8M)
  //   Cortex-M33 (ARM-v8M)
  //   Cortex-A/R (ARM-v7A/R)
  //
  // We do not explicitly check for "Embedded Studio" as the compiler in use determines what we support.
  // You can use an external toolchain like IAR inside ES. So there is no point in checking for "Embedded Studio"
  //
  #if (defined __CROSSWORKS_ARM)                  // Rowley Crossworks
    #define _CC_HAS_RTT_ASM_SUPPORT 1
    #if (defined __ARM_ARCH_7M__)                 // Cortex-M3
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
    #elif (defined __ARM_ARCH_7EM__)              // Cortex-M4/M7
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_BASE__)          // Cortex-M23
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_MAIN__)          // Cortex-M33
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined(__ARM_ARCH_8_1M_MAIN__))       // Cortex-M85
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #else
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
    #endif
  #elif (defined __ARMCC_VERSION)
    //
    // ARM compiler
    // ARM compiler V6.0 and later is clang based.
    // Our ASM part is compatible to clang.
    //
    #if (__ARMCC_VERSION >= 6000000)
      #define _CC_HAS_RTT_ASM_SUPPORT 1
    #else
      #define _CC_HAS_RTT_ASM_SUPPORT 0
    #endif
    #if (defined __ARM_ARCH_6M__)                 // Cortex-M0 / M1
      #define _CORE_HAS_RTT_ASM_SUPPORT 0         // No ASM support for this architecture
    #elif (defined __ARM_ARCH_7M__)               // Cortex-M3
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
    #elif (defined __ARM_ARCH_7EM__)              // Cortex-M4/M7
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_BASE__)          // Cortex-M23
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_MAIN__)          // Cortex-M33
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8_1M_MAIN__)        // Cortex-M85
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif \
    ((defined __ARM_ARCH_7A__) || (defined __ARM_ARCH_7R__)) || \
    ((defined __ARM_ARCH_8A__) || (defined __ARM_ARCH_8R__))
      //
      // Cortex-A/R ARMv7-A/R & ARMv8-A/R
      //
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #else
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
    #endif
  #elif ((defined __GNUC__) || (defined __clang__))
    //
    // GCC / Clang
    //
    #define _CC_HAS_RTT_ASM_SUPPORT 1
    // ARM 7/9: __ARM_ARCH_5__ / __ARM_ARCH_5E__ / __ARM_ARCH_5T__ / __ARM_ARCH_5T__ / __ARM_ARCH_5TE__
    #if (defined __ARM_ARCH_7M__)                 // Cortex-M3
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
    #elif (defined __ARM_ARCH_7EM__)              // Cortex-M4/M7
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1         // Only Cortex-M7 needs a DMB but we cannot distinguish M4 and M7 here...
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_BASE__)          // Cortex-M23
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_MAIN__)          // Cortex-M33
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8_1M_MAIN__)        // Cortex-M85
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif \
    (defined __ARM_ARCH_7A__) || (defined __ARM_ARCH_7R__) || \
    (defined __ARM_ARCH_8A__) || (defined __ARM_ARCH_8R__)
      //
      // Cortex-A/R ARMv7-A/R & ARMv8-A/R
      //
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #else
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
    #endif
  #elif ((defined __IASMARM__) || (defined __ICCARM__))
    //
    // IAR assembler/compiler
    //
    #define _CC_HAS_RTT_ASM_SUPPORT 1
    #if (__VER__ < 6300000)
      #define VOLATILE
    #else
      #define VOLATILE volatile
    #endif
    #if (defined __ARM7M__)                            // Needed for old versions that do not know the define yet
      #if (__CORE__ == __ARM7M__)                      // Cortex-M3
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #endif
    #endif
    #if (defined __ARM7EM__)
      #if (__CORE__ == __ARM7EM__)                     // Cortex-M4/M7
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8M_BASELINE__)
      #if (__CORE__ == __ARM8M_BASELINE__)             // Cortex-M23
        #define _CORE_HAS_RTT_ASM_SUPPORT 0
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8M_MAINLINE__)
      #if (__CORE__ == __ARM8M_MAINLINE__)             // Cortex-M33
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8EM_MAINLINE__)
      #if (__CORE__ == __ARM8EM_MAINLINE__)            // Cortex-???
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if\
    ((defined __ARM7A__) && (__CORE__ == __ARM7A__)) || \
    ((defined __ARM7R__) && (__CORE__ == __ARM7R__)) || \
    ((defined __ARM8A__) && (__CORE__ == __ARM8A__)) || \
    ((defined __ARM8R__) && (__CORE__ == __ARM8R__))
      //
      // Cortex-A/R ARMv7-A/R & ARMv8-A/R
      //
       #define _CORE_NEEDS_DMB 1
      #define RTT__DMB() asm VOLATILE ("DMB");
    #endif
  #else
    //
    // Other compilers
    //
    #define _CC_HAS_RTT_ASM_SUPPORT   0
    #define _CORE_HAS_RTT_ASM_SUPPORT 0
  #endif
  //
  // If IDE and core support the ASM version, enable ASM version by default
  //
  #ifndef _CORE_HAS_RTT_ASM_SUPPORT
    #define _CORE_HAS_RTT_ASM_SUPPORT 0              // Default for unknown cores
  #endif
  #if (_CC_HAS_RTT_ASM_SUPPORT && _CORE_HAS_RTT_ASM_SUPPORT)
    #define RTT_USE_ASM                           (1)
  #else
    #define RTT_USE_ASM                           (0)
  #endif
#endif

#ifndef _CORE_NEEDS_DMB
  #define _CORE_NEEDS_DMB 0
#endif

#ifndef RTT__DMB
  #if _CORE_NEEDS_DMB
    #error "Don't know how to place inline assembly for DMB"
  #else
    #define RTT__DMB()
  #endif
#endif


//+********************************* FC USE **********************************/

#ifndef FC_RTT_LOCK
  #define FC_RTT_LOCK() SEGGER_RTT_LOCK()       // Lock RTT (nestable)   (i.e. disable interrupts)
#endif

#ifndef FC_RTT_UNLOCK
  #define FC_RTT_UNLOCK() SEGGER_RTT_UNLOCK()   // Unlock RTT (nestable) (i.e. enable previous interrupt lock state)
#endif

#ifndef FC_RTT_DMB
  #if _CORE_NEEDS_DMB
    #error "Don't know how to place inline assembly for DMB"
  #else
    #define FC_RTT_DMB() RTT__DMB()
#endif

#endif //\ _FC_RTT_HELPER_H_
/*************************** End of file ****************************/
