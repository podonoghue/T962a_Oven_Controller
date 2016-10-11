/*
 * switchDebouncer.h
 *
 *  Created on: 24 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SWITCH_DEBOUNCER_H_
#define SOURCES_SWITCH_DEBOUNCER_H_

#include "pid.h"

/**
 * Return values from switch debouncer
 */
enum SwitchValue {
   SW_NONE = 0,
   SW_F1   = 1<<0,
   SW_F2   = 1<<1,
   SW_F3   = 1<<2,
   SW_F4   = 1<<3,
   SW_S    = 1<<4,
   SW_F3F4 = SW_F3|SW_F4, // used for +/- keys together
};

/**
 * @tparam timerChannel The PIT channel number to use for callback
 * @tparam f1           F1 switch GPIO
 * @tparam f2           F2 switch GPIO
 * @tparam f3           F3 switch GPIO
 * @tparam f4           F4 switch GPIO
 * @tparam sel          S  switch GPIO
 *
 * F1..F4 have auto-repeat function
 */
template<unsigned timerChannel, typename f1, typename f2, typename f3, typename f4, typename sel>
class SwitchDebouncer {
private:
   /**
    * Time to debounce the switch (ms)
    * Long than about 100 ms is a perceptible delay
    * This also affects how easy it is to do intentional multi-presses
    */
   static constexpr int DEBOUNCE_THRESHOLD = 100;

   /* Auto-repeat delay (ms) */
   static constexpr int REPEAT_THRESHOLD   = 1000;

   /* Auto-repeat period (ms) */
   static constexpr int REPEAT_PERIOD      = 200;

   /** Last pressed switch */
   static volatile SwitchValue switchNum __attribute__((aligned (4)));

   /** Indicates that the key is repeating */
   static volatile bool repeating;

   /**
    * Called at a regular rate to poll the switches
    */
   static void switchHandler(void) {
      static uint debounceCount = 0;
      static uint lastSnapshot  = 0;

      uint snapshot =
            (f1::read()?SW_F1:0)|
            (f2::read()?SW_F2:0)|
            (f3::read()?SW_F3:0)|
            (f4::read()?SW_F4:0)|
            (sel::read()?SW_S:0);

      if ((snapshot != 0) && (snapshot == lastSnapshot)) {
         debounceCount++;
         if (debounceCount == DEBOUNCE_THRESHOLD) {
            switchNum = (SwitchValue)snapshot;
            repeating = false;
         }
         if ((debounceCount >= REPEAT_THRESHOLD) && ((debounceCount % REPEAT_PERIOD) == 0)) {
            switchNum = (SwitchValue)snapshot;
            repeating = true;
         }
      }
      else {
         debounceCount = 0;
         repeating = false;
      }
      lastSnapshot  = snapshot;
   }

public:
   /**
    * Initialise the switch monitoring
    */
   SwitchDebouncer() {
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
      repeating = false;
   }

   /**
    * Check if a button is pressed without removing it from the buffer
    *
    * @return button value or SW_NONE if none pressed
    */
   static SwitchValue peekButton() {
      return switchNum;
   }

   /**
    * Check if a button is repeating (i.e. button is held)
    *
    * @return True if repeating
    */
   static bool isRepeating() {
      return repeating;
   }

   /**
    * Get button press
    *
    * @return button value or SW_NONE if none pressed since last queried
    */
   static SwitchValue getButton() {
      SwitchValue t;
      // Bit messy but avoids masking interrupts or clobbering a key press
      do {
         t = (SwitchValue)__LDREXW((volatile long unsigned *)&switchNum);
      } while(__STREXW(SW_NONE, (volatile long unsigned *)&switchNum) != 0);
      return t;
   }
};

/** Last pressed switch */
template<unsigned timerChannel, typename f1, typename f2, typename f3, typename f4, typename sel>
volatile SwitchValue SwitchDebouncer<timerChannel, f1, f2, f3, f4, sel>::switchNum;

/** Indicates that the key is repeating */
template<unsigned timerChannel, typename f1, typename f2, typename f3, typename f4, typename sel>
volatile bool SwitchDebouncer<timerChannel, f1, f2, f3, f4, sel>::repeating;

#endif /* SOURCES_SWITCH_DEBOUNCER_H_ */
