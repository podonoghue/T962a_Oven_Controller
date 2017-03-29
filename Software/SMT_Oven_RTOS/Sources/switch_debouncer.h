/**
 * @file    switch_debouncer.h
 * @brief   Switch Debouncer
 *
 *  Created on: 24 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SWITCH_DEBOUNCER_H_
#define SOURCES_SWITCH_DEBOUNCER_H_

#include "cmsis.h"

/**
 * Return values from switch
 */
class SwitchValue {

public:
   enum Values {
      SW_NONE  = 0,
      SW_F1    = 1<<0,
      SW_F2    = 1<<1,
      SW_F3    = 1<<2,
      SW_F4    = 1<<3,
      SW_S     = 1<<4,
      SW_F3F4  = SW_F3|SW_F4, // used for F3 & F4 keys together (chording)
   };

private:
   Values fValue;
   static constexpr uint8_t SW_REPEATING = 1<<7;

public:
   /**
    * Cast to int\n
    * @note Removes SW_REPEATING flag
    *
    * @return Value converted to int
    */
   operator int() const {
      return (int)fValue&~SW_REPEATING;
   }
   /**
    * Indicates if key is repeating
    *
    * @return true  Indicates repeating key-press
    * @return false Indicates normal key-press
    */
   bool isRepeating() const {
      return (int)fValue&SW_REPEATING;
   }
   /**
    * Makes key repeating
    *
    * @return Reference to modified object
    */
   SwitchValue &setRepeating() {
      fValue = (Values)((int)fValue|(int)SW_REPEATING);
      return *this;
   }
   /**
    * Constructor
    *
    * @param value Value to base switch on
    */
   SwitchValue(int value) {
      fValue = (Values)value;
   }
   /**
    * Default constructor
    */
   SwitchValue() {
      fValue = SW_NONE;
   }
   /**
    * Indivisible get value and clear operation
    *
    * @return Previous switch value
    *
    * @note The current value is cleared (set to SW_NONE)
    */
   SwitchValue clear() {
      SwitchValue t;
      // Bit messy but avoids masking interrupts or clobbering a key press
      do {
         t = (SwitchValue)__LDREXW((volatile long unsigned *)&fValue);
      } while(__STREXW(SW_NONE, (volatile long unsigned *)&fValue) != 0);
      return t;
   }
};

/**
 * @tparam f1           F1 switch GPIO
 * @tparam f2           F2 switch GPIO
 * @tparam f3           F3 switch GPIO
 * @tparam f4           F4 switch GPIO
 * @tparam sel          Select switch GPIO
 *
 * F1..F4 have auto-repeat function
 */
template<typename f1, typename f2, typename f3, typename f4, typename sel>
class SwitchDebouncer {

private:
   /*
    * Interval for switch scanning
    */
   static constexpr int TICK_INTERVAL = 10; // ms
   /**
    * Time to debounce the switch (in TICK_INTERVAL)
    * Longer than about 100 ms is a perceptible delay
    * This also affects how easy it is to do intentional chording (multi-press)
    */
   static constexpr int DEBOUNCE_THRESHOLD = 50/TICK_INTERVAL;

   /* Auto-repeat delay (in TICK_INTERVAL) */
   static constexpr int REPEAT_THRESHOLD   = 1000/TICK_INTERVAL;

   /* Auto-repeat period (in TICK_INTERVAL) */
   static constexpr int REPEAT_PERIOD      = 200/TICK_INTERVAL;

   /** Last pressed switch */
   static volatile SwitchValue switchNum;

   /** Queue used to transfer key presses to consumers */
   static CMSIS::MessageQueue<SwitchValue, 3> keyQueue;

   /**
    * Called at a regular rate to poll the switches
    */
   static void switchHandler(const void *) {
      static uint debounceCount = 0;
      static uint lastSnapshot  = 0;

      uint8_t snapshot =
            (f1::read()? SwitchValue::SW_F1:0)|
            (f2::read()? SwitchValue::SW_F2:0)|
            (f3::read()? SwitchValue::SW_F3:0)|
            (f4::read()? SwitchValue::SW_F4:0)|
            (sel::read()?SwitchValue::SW_S:0);

      if ((snapshot != 0) && (snapshot == lastSnapshot)) {
         debounceCount++;
         if (debounceCount == DEBOUNCE_THRESHOLD) {
            // Consider de-bounced
            keyQueue.put(SwitchValue(snapshot), 0);
         }
         if ((debounceCount >= REPEAT_THRESHOLD) &&
               ((debounceCount % REPEAT_PERIOD) == 0) &&
               ((snapshot&SwitchValue::SW_S) == 0)) {
            // Pressed and held - auto-repeat
            keyQueue.put(SwitchValue(snapshot).setRepeating(), 0);
         }
      }
      else {
         debounceCount = 0;
      }
      lastSnapshot  = snapshot;
   }

   /** Timer used for polling callback */
   static CMSIS::Timer<osTimerPeriodic> timer;

   /** Buffer to implement peek() */
   static SwitchValue lookaheadKey;

   /**
    * Get Key from queue
    *
    * @param waitInMilliseconds How long to wait for key
    *
    * @return Key value or SW_NONE is none available before timeout
    */
   static SwitchValue deQueue(uint32_t waitInMilliseconds) {
      osEvent event = keyQueue.get(waitInMilliseconds);
      if (event.status == osEventMessage) {
         return (SwitchValue)(event.value.v);
      }
      return SwitchValue::SW_NONE;
   }
public:
   /**
    * Create the switch monitor
    */
   SwitchDebouncer() {
      f1::setInput();
      f2::setInput();
      f3::setInput();
      f4::setInput();
      sel::setInput();
   }

   /**
    * Initialise the switch monitoring
    */
   static void initialise() {
      keyQueue.create();
      timer.create();
      timer.start(TICK_INTERVAL);
   }
   /**
    * Check if a button is pressed without removing it from the buffer
    *
    * @return button value or SW_NONE if none pressed
    */
   static SwitchValue peekButton() {
      if (lookaheadKey == SwitchValue::SW_NONE) {
         // No lookahead, try to get a new key without waiting
         lookaheadKey = deQueue(0);
      }
      return lookaheadKey;
   }

   /**
    * Get button press
    *
    * @param millisecondsToWait How long to wait for button
    *
    * @return Button value or SW_NONE if none pressed before timeout
    */
   static SwitchValue getButton(uint32_t millisecondsToWait=osWaitForever) {
      // Get and clear lookahead
      SwitchValue tempKey = lookaheadKey.clear();
      if (tempKey != SwitchValue::SW_NONE) {
         // Return lookahead
         return tempKey;
      }
      // Try to get a new key with wait
      return deQueue(millisecondsToWait);
   }
};

/** Key queue*/
template<typename f1, typename f2, typename f3, typename f4, typename sel>
CMSIS::MessageQueue<SwitchValue, 3> SwitchDebouncer<f1, f2, f3, f4, sel>::keyQueue;

/** Timer for callback */
template<typename f1, typename f2, typename f3, typename f4, typename sel>
CMSIS::Timer<osTimerPeriodic> SwitchDebouncer<f1, f2, f3, f4, sel>::timer{switchHandler};

/** Last pressed switch */
template<typename f1, typename f2, typename f3, typename f4, typename sel>
volatile SwitchValue SwitchDebouncer<f1, f2, f3, f4, sel>::switchNum;

/** Lookahead key */
template<typename f1, typename f2, typename f3, typename f4, typename sel>
SwitchValue SwitchDebouncer<f1, f2, f3, f4, sel>::lookaheadKey;

#endif /* SOURCES_SWITCH_DEBOUNCER_H_ */
