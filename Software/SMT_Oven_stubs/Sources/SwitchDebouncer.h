/**
 * @file    SwitchDebouncer.h
 * @brief   Switch Debouncer
 *
 *  Created on: 24 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SWITCHDEBOUNCER_H_
#define SOURCES_SWITCHDEBOUNCER_H_

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
    * Cast to integer\n
    * @note Removes SW_REPEATING flag
    *
    * @return Value converted to integer
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
 *
 * Switch debouncer\n
 * It regularly polls the 5 switches ands adds switch-presses to a queue
 *
 * @tparam f1           F1 switch GPIO
 * @tparam f2           F2 switch GPIO
 * @tparam f3           F3 switch GPIO
 * @tparam f4           F4 switch GPIO
 * @tparam sel          Select switch GPIO
 *
 * F1..F4 have auto-repeat function
 */
template<typename f1, typename f2, typename f3, typename f4, typename sel>
class SwitchDebouncer : private CMSIS::TimerClass {

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
   volatile SwitchValue switchNum;

   /** Queue used to transfer key presses to consumers */
   CMSIS::MessageQueue<SwitchValue, 3> keyQueue;

   /** Buffer to implement peek() */
   SwitchValue lookaheadKey;

   uint debounceCount = 0;
   uint lastSnapshot  = 0;

   /**
    * Get Key from queue
    *
    * @param waitInMilliseconds How long to wait for key
    *
    * @return Key value or SW_NONE if none available before timeout
    */
   SwitchValue deQueue(uint32_t waitInMilliseconds) {
      osEvent event = keyQueue.get(waitInMilliseconds);
      if (event.status == osEventMessage) {
         return (SwitchValue)(event.value.v);
      }
      return SwitchValue::SW_NONE;
   }

   /**
    * Called at a regular rate by a CMSIS timer to poll the switches
    */
   void callback() override {
      uint8_t snapshot =
            (f1::isPressed()? SwitchValue::SW_F1:0)|
            (f2::isPressed()? SwitchValue::SW_F2:0)|
            (f3::isPressed()? SwitchValue::SW_F3:0)|
            (f4::isPressed()? SwitchValue::SW_F4:0)|
            (sel::isPressed()?SwitchValue::SW_S:0);

      if ((snapshot != 0) && (snapshot == lastSnapshot)) {
         // Keys pressed and unchanged
         debounceCount++;
         if (debounceCount == DEBOUNCE_THRESHOLD) {
            // Consider de-bounced
            keyQueue.put(SwitchValue(snapshot), 0);
         }
         if ((debounceCount >= REPEAT_THRESHOLD) &&
               ((debounceCount % REPEAT_PERIOD) == 0) &&
               ((snapshot&SwitchValue::SW_S) == 0)) {
            // Pressed and held - auto-repeat
            // Note - S Key does not repeat
            keyQueue.put(SwitchValue(snapshot).setRepeating(), 0);
         }
      }
      else {
         // Restart debounce time
         debounceCount = 0;
      }
      lastSnapshot  = snapshot;
   }

public:
   /**
    * Create the switch monitor
    */
   SwitchDebouncer() {
      using namespace USBDM;
      f1::setInput(PinPull_Up);
      f2::setInput(PinPull_Up);
      f3::setInput(PinPull_Up);
      f4::setInput(PinPull_Up);
      sel::setInput(PinPull_Up);

      keyQueue.create();
      start(TICK_INTERVAL);
   }

   /**
    * Check if a button is pressed without removing it from the buffer
    *
    * @return button value or SW_NONE if none pressed
    */
   SwitchValue peekButton() {
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
   SwitchValue getButton(uint32_t millisecondsToWait=osWaitForever) {
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

#endif /* SOURCES_SWITCHDEBOUNCER_H_ */
