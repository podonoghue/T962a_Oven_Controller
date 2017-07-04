/**
 * @file     rtc.h (180.ARM_Peripherals/Project_Headers/rtc.h)
 * @brief    Real Time Clock
 *
 * @version  V4.12.1.80
 * @date     13 April 2016
 */
#ifndef RTC_H_
#define RTC_H_
 /*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
#include "hardware.h"

namespace USBDM {

/**
 * Type definition for RTC interrupt call back
 *
 *  @param timeSinceEpoch - Time since the epoch in seconds
 */
typedef void (*RTCCallbackFunction)(uint32_t timeSinceEpoch);

template <class Info>
class RtcBase_T {

protected:
   static constexpr volatile RTC_Type *rtc      = Info::rtc;
   static constexpr volatile uint32_t *clockReg = Info::clockReg;

public:
   /**
    * Initialise RTC to default settings\n
    * Configures all RTC pins
    */
   static void initialise() {

      // Enable clock to RTC interface
      // (RTC used its own clock internally)
      *clockReg  |= Info::clockMask;
      __DMB();

      if ((Info::cr&RTC_CR_OSCE_MASK) == 0) {
         // RTC disabled
         return;
      }

      // Configure pins
      Info::initPCRs();

      // Enable to debug RTX startup
#if defined(DEBUG_BUILD) && 0
      // Software reset RTC - trigger cold start
      rtc->CR  = RTC_CR_SWR_MASK;
      rtc->CR  = 0;

      // Disable interrupts
      rtc->IER  = 0;
#endif

      if ((rtc->SR&RTC_SR_TIF_MASK) != 0) {
         // RTC not running yet or invalid - re-initialise

         // Software reset RTC
         rtc->CR  = RTC_CR_SWR_MASK;
         rtc->CR  = 0;

         // Configure oscillator
         // Note - on KL25 this will disable the standard oscillator
         rtc->CR  = Info::cr;

         // Wait startup time
         for (int i=0; i<100000; i++) {
            __asm__("nop");
         }

         // Set current time
         rtc->TSR = Info::coldStartTime;
         rtc->SR  = RTC_SR_TCE_MASK;

         // Time compensation values
         rtc->TCR = RtcInfo::tcr;

         // Lock registers
         rtc->LR  = RtcInfo::lr;

#ifdef RTC_WAR_IERW_MASK
         // Write access
         rtc->WAR = RtcInfo::war;
#endif
#ifdef RTC_RAR_IERR_MASK
         // Read access
         rtc->RAR = RtcInfo::rar;
#endif
      }

      // Update settings
      rtc->CR   = Info::cr;
   }

   /**
    * Enable/disable interrupts in NVIC
    *
    * @param enable true to enable, false to disable
    */
   static void enableNvicInterrupts(bool enable=true) {
      // Clear pending to avoid POR interrupt
      NVIC_ClearPendingIRQ(Info::irqNums[0]);
      if (enable) {
         // Enable interrupts
         NVIC_EnableIRQ(Info::irqNums[0]);

         // Set priority level
         NVIC_SetPriority(Info::irqNums[0], Info::irqLevel);
      }
      else {
         // Disable interrupts
         NVIC_DisableIRQ(Info::irqNums[0]);
      }
   }

   /*
    * Sets the system RTC time
    *
    *  @param timeSinceEpoch - time since the epoch in seconds
    */
   static void setTime(uint32_t timeSinceEpoch) {
      rtc->SR  = 0;
      rtc->TSR = timeSinceEpoch;
      rtc->SR  = RTC_SR_TCE_MASK;
   }

   /**
    *  Get current alarm time
    *
    *  @return alarm time as 32-bit number
    */
   static uint32_t getTime(void) {
      return rtc->TSR;
   }

   /**
    *  Get current alarm time
    *
    *  @return alarm time as 32-bit number
    */
   uint32_t getAlarmTime(void) {
      return rtc->TAR;
   }

   /**
    *  Set alarm time
    *
    *  @param timeSinceEpoch - Alarm time in seconds relative to the epoch
    */
   static void rtc_setAlarmTime(uint32_t timeSinceEpoch) {
      rtc->TAR = timeSinceEpoch;
   }

};

/**
 * Template class to provide RTC callback
 */
template<class Info>
class RtcIrq_T : public RtcBase_T<Info> {

protected:
   /** Callback function for ISR */
   static RTCCallbackFunction callback;

public:
   /**
    * IRQ handler
    */
   static void irqHandler(void) {
      if (callback != 0) {
         callback(RtcBase_T<Info>::rtc->TSR);
      }
      if ((RtcBase_T<Info>::rtc->SR&RTC_SR_TAF_MASK) != 0) {
         // Clear alarm
         RtcBase_T<Info>::rtc->TAR   = 0;
      }
   }

   /**
    * Enable/disable RTC Alarm interrupts
    *
    * @param enable True=>enable, False=>disable
    */
   static void enableAlarmInterrupts(bool enable=true) {
      if (enable) {
         RTC->IER   |= RTC_IER_TAIE_MASK;
      }
      else {
         RTC->IER   &= ~RTC_IER_TAIE_MASK;
      }
   }
   /**
    * Enable/disable RTC Seconds interrupts
    *
    * @param enable True=>enable, False=>disable
    */
   static void enableSecondsInterrupts(bool enable=true) {
      if (enable) {
         RTC->IER   |= RTC_IER_TSIE_MASK;
      }
      else {
         RTC->IER   &= ~RTC_IER_TSIE_MASK;
      }
   }
   /**
    * Set Callback function
    *
    *   @param theCallback - Callback function to be executed on RTC alarm interrupt
    */
   static void setCallback(RTCCallbackFunction theCallback) {
      callback = theCallback;
   }
   /**
    * Set alarm time
    *
    *   @param time        - Time to set alarm for (time since the epoch in seconds)
    */
   static void setAlarm(uint32_t time) {
         RtcBase_T<Info>::rtc->TAR = time;
   }
};

template<class Info> RTCCallbackFunction RtcIrq_T<Info>::callback = 0;

#ifdef USBDM_RTC_IS_DEFINED
/**
 * Class representing RTC
 */
using Rtc = RtcIrq_T<RtcInfo>;

#endif

} // End namespace USBDM

#endif /* RTC_H_ */
