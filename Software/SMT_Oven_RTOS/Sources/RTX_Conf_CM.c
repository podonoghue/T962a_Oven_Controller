/*----------------------------------------------------------------------------
 *      CMSIS-RTOS  -  RTX
 *----------------------------------------------------------------------------
 *      Name:    RTX_Conf_CM.c
 *      Purpose: Configuration of CMSIS RTX Kernel for Cortex-M
 *      Rev.:    V4.70.1
 *----------------------------------------------------------------------------
 *
 * Copyright (c) 1999-2009 KEIL, 2009-2015 ARM Germany GmbH
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  - Neither the name of ARM  nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * Based on RTX_Conf_CM.c
 *---------------------------------------------------------------------------*/

#include "cmsis_os.h"
#include "RTX_Conf_CM.cfg"

#define OS_TRV          ((uint32_t)(((double)OS_CLOCK*(double)OS_TICK)/1E6)-1)


/*----------------------------------------------------------------------------
 *      Global Functions
 *---------------------------------------------------------------------------*/

/*--------------------------- os_idle_demon ---------------------------------*/

/// \brief The idle demon is running when no other thread is ready to run
void os_idle_demon (void) {

  for (;;) {
    /* HERE: include optional user code to be executed when no thread runs.*/
     __asm__("wfi");
  }
}

#if (OS_SYSTICK == 0)   // Functions for alternative timer as RTX kernel timer

/*--------------------------- os_tick_init ----------------------------------*/

/// \brief Initializes an alternative hardware timer as RTX kernel timer
/// \return                             IRQ number of the alternative hardware timer
int os_tick_init (void) {
  return (-1);  /* Return IRQ number of timer (0..239) */
}

/*--------------------------- os_tick_val -----------------------------------*/

/// \brief Get alternative hardware timer's current value (0 .. OS_TRV)
/// \return                             Current value of the alternative hardware timer
uint32_t os_tick_val (void) {
  return (0);
}

/*--------------------------- os_tick_ovf -----------------------------------*/

/// \brief Get alternative hardware timer's  overflow flag
/// \return                             Overflow flag\n
///                                     - 1 : overflow
///                                     - 0 : no overflow
uint32_t os_tick_ovf (void) {
  return (0);
}

/*--------------------------- os_tick_irqack --------------------------------*/

/// \brief Acknowledge alternative hardware timer interrupt
void os_tick_irqack (void) {
  /* ... */
}

#endif   // (OS_SYSTICK == 0)

/*--------------------------- os_error --------------------------------------*/

/* OS Error Codes */
#define OS_ERROR_STACK_OVF      1
#define OS_ERROR_FIFO_OVF       2
#define OS_ERROR_MBX_OVF        3
#define OS_ERROR_TIMER_OVF      4

extern osThreadId svcThreadGetId (void) __attribute__ ((unused));

typedef struct {          /* Post Service Fifo Entry                 */
  void       *id;              /* Object Identification                   */
  uint32_t    arg;             /* Object Argument                         */
} PostServiceQueueEntry;

/** Post Service Queue */
typedef struct {
  uint8_t     first;            /* FIFO Head Index                         */
  uint8_t     last;             /* FIFO Tail Index                         */
  uint8_t     count;            /* Number of stored items in FIFO          */
  uint8_t     size;             /* FIFO Size                               */
  PostServiceQueueEntry q[1];   /* FIFO Content                            */
} PostServiceQueue;

typedef struct {
   uint8_t     cb_type;                 /** Control Block Type                      */
   uint8_t     state;                   /** State flag variable                     */
   uint8_t     isr_st;                  /** State flag variable for isr functions   */
   struct OS_TCB *p_lnk;           /** Chain of tasks waiting for message      */
   uint16_t    first;                   /** Index of the message list begin         */
   uint16_t    last;                    /** Index of the message list end           */
   uint16_t    count;                   /** Actual number of stored messages        */
   uint16_t    size;                    /** Maximum number of stored messages       */
   void   *msg[1];                 /** FIFO for Message pointers 1st element   */
} MessageBoxQueue;

volatile  void  *id;

void pulse() {
   typedef struct GPIO_Type {
      volatile uint32_t  PDOR;                         /**< 0000: Port Data Output Register                                    */
      volatile uint32_t  PSOR;                         /**< 0004: Port Set Output Register                                     */
      volatile uint32_t  PCOR;                         /**< 0008: Port Clear Output Register                                   */
      volatile uint32_t  PTOR;                         /**< 000C: Port Toggle Output Register                                  */
      volatile uint32_t  PDIR;                         /**< 0010: Port Data Input Register                                     */
      volatile uint32_t  PDDR;                         /**< 0014: Port Data Direction Register                                 */
   } GPIO_Type;
#define GPIOB_BasePtr                  0x400FF040UL //!< Peripheral base address
#define GPIOB                          ((GPIO_Type *) GPIOB_BasePtr) //!< Freescale base pointer
   GPIOB->PCOR = (1<<17);
//   for(unsigned i=0; i<100; i++) {
//      GPIOB->PTOR = (1<<17);
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//      __asm__("nop");
//   }
//   GPIOB->PSOR = (1<<17);
}

void iterateMessageboxQueue(void *q) {
   MessageBoxQueue *queue = (MessageBoxQueue*)q;
   uint8_t iter = queue->first;
   pulse();
   for(unsigned count = queue->count; count>0; count--) {
      id = queue->msg[iter];
      iter++;
      if (iter>=queue->size) {
         iter = 0;
      }
   }
}

/// \brief Called when a runtime error is detected
/// \param[in]   error_code   actual error code that has been detected
void os_error (uint32_t error_code) {

   //osThreadId err_task = svcThreadGetId();
   //(void)err_task;

  /* HERE: include optional code to be executed on runtime error. */
  switch (error_code) {
    case OS_ERROR_STACK_OVF:
      /* Stack overflow detected for the currently running task. */
      /* Thread can be identified by calling svcThreadGetId().   */
      __asm__("bkpt #0");
      break;
    case OS_ERROR_FIFO_OVF:
      /* ISR FIFO Queue buffer overflow detected. */
      __asm__("bkpt #0");
      break;
    case OS_ERROR_MBX_OVF:
      /* Mailbox overflow detected. */
      __asm__("bkpt #0");
      break;
    case OS_ERROR_TIMER_OVF:
      /* User Timer Callback Queue overflow detected. */
      __asm__("bkpt #0");
      break;
    default:
      break;
  }

  for (;;);
}


/*----------------------------------------------------------------------------
 *      RTX Configuration Functions
 *---------------------------------------------------------------------------*/

#include "RTX_CM_lib.h"

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
