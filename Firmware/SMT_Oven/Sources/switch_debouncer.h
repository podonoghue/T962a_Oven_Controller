/*
 * switchDebouncer.h
 *
 *  Created on: 24 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SWITCH_DEBOUNCER_H_
#define SOURCES_SWITCH_DEBOUNCER_H_

/**
 * @tparam timerChannel The PIT channel number to use for callback
 * @tparam f1           F1 switch
 * @tparam f2           F2 switch
 * @tparam f3           F3 switch
 * @tparam f4           F4 switch
 * @tparam sel          S  switch
 *
 * F1..F4 have auto-repeat function
 */
enum SwitchValue {
   SW_NONE = 0,
   SW_F1   = 1<<0,
   SW_F2   = 1<<1,
   SW_F3   = 1<<2,
   SW_F4   = 1<<3,
   SW_S    = 1<<4,
};

template<unsigned timerChannel, typename f1, typename f2, typename f3, typename f4, typename sel>
class SwitchDebouncer {
private:
   // Time to debounce the switch (ms)
   static constexpr int DEBOUNCE_THRESHOLD = 20;
   // Auto-repeat delay (ms)
   static constexpr int REPEAT_THRESHOLD   = 1000;
   // Auto-repeat period (ms)
   static constexpr int REPEAT_PERIOD      = 200;

   // Last pressed switch
   static volatile SwitchValue switchNum;

   /**
    * Called at a regular rate to poll the switches
    */
   static void switchHandler(void) {
      static uint sw1Count = 0;
      static uint sw2Count = 0;
      static uint sw3Count = 0;
      static uint sw4Count = 0;
      static uint sw5Count = 0;

      if (!f1::read()) {
         sw1Count++;
         if ((sw1Count == DEBOUNCE_THRESHOLD) || ((sw1Count >= REPEAT_THRESHOLD) && ((sw1Count % REPEAT_PERIOD) == 0))) {
            switchNum = SW_F1;
         }
      }
      else {
         sw1Count = 0;
      }
      if (!f2::read()) {
         sw2Count++;
         if ((sw2Count == DEBOUNCE_THRESHOLD) || ((sw2Count >= REPEAT_THRESHOLD) && ((sw2Count % REPEAT_PERIOD) == 0))) {
            switchNum = SW_F2;
         }
      }
      else {
         sw2Count = 0;
      }
      if (!f3::read()) {
         sw3Count++;
         if ((sw3Count == DEBOUNCE_THRESHOLD) || ((sw3Count >= REPEAT_THRESHOLD) && ((sw3Count % REPEAT_PERIOD) == 0))){
            switchNum = SW_F3;
         }
      }
      else {
         sw3Count = 0;
      }
      if (!f4::read()) {
         sw4Count++;
         if ((sw4Count == DEBOUNCE_THRESHOLD) || ((sw4Count > REPEAT_THRESHOLD) && ((sw4Count % REPEAT_PERIOD) == 0))) {
            switchNum = SW_F4;
         }
      }
      else {
         sw4Count = 0;
      }
      if (!sel::read()) {
         sw5Count++;
         if (sw5Count == DEBOUNCE_THRESHOLD) {
            switchNum = SW_S;
         }
      }
      else {
         sw5Count = 0;
      }
   }

public:
   /**
    * Initialise the switch monitoring
    */
   static void initialise() {
      f1::setInput();
      f2::setInput();
      f3::setInput();
      f4::setInput();
      sel::setInput();
      USBDM::Pit::enable();
      USBDM::Pit::configureChannel(timerChannel, 1*USBDM::ms);
      USBDM::Pit::setCallback(timerChannel, switchHandler);
      USBDM::Pit::enableInterrupts(timerChannel);
      switchNum = SW_NONE;
   }

   /**
    * Check if a button is pressed without removing it from the buffer
    *
    * @return button value or zero if none
    */
   static SwitchValue peekButton() {
      return switchNum;
   }

   /**
    * Get button press
    *
    * @return button value or zero if none pressed since last queried
    */
   static SwitchValue getButton() {
      disableInterrupts();
      SwitchValue t = switchNum;
      switchNum = SW_NONE;
      enableInterrupts();
      return t;
   }
};

template<unsigned timerChannel, typename f1, typename f2, typename f3, typename f4, typename sel>
volatile SwitchValue SwitchDebouncer<timerChannel, f1, f2, f3, f4, sel>::switchNum;

#endif /* SOURCES_SWITCH_DEBOUNCER_H_ */
