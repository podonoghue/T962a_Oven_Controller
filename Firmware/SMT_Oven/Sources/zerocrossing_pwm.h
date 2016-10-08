/*
 * zerocrossing_pwm.h
 *
 *  Created on: 24 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_ZEROCROSSING_PWM_H_
#define SOURCES_ZEROCROSSING_PWM_H_

#include "flash.h"

/**
 * Simple zero-crossing PWM for oven fan and heater controlled by a zero-crossing SSDs
 *
 * The switching waveform will be synchronised to the mains zero crossing
 *
 * @tparam OvenFan    USBDM::Gpio controlling the oven fan SSD
 * @tparam Heater     USBDM::Gpio controlling the oven heater SSD
 * @tparam FtmChannel USBDM::Cmp used for mains sensing
 */
template<typename Heater, typename HeaterLed, typename Fan, typename FanLed, typename Vmains>
class ZeroCrossingPwm {
private:

   /** Duty cycle for Heater */
   static int  heaterDutycycle;
   /** Duty cycle for Fan */
   static int  fanDutycycle;
   /** Count down for fan kick */
   static int  fanKick;

   /**
    * Number of mains half-cycles to run the fan before switching to PWM mode
    * This is to overcome the static friction of the fan on low duty-cycle
    */
   USBDM::Nonvolatile<int> &fanKickTime;

   /*
    * Function is called on zero-crossings of the mains
    * Implements a simple PWM with variable period (~20ms - ~1s @50Hz mains)
    */
   static void callbackFunction(int status) {
      // Keeps track of heater drive
      static int heaterDutycount = 0;
      // Keeps track of fan drive
      static int fanDutycount = 0;
      (void)status;
#if 0
      // Simple PWM with fixed period
      static int cycleCount = 0;
      cycleCount++;
      if (cycleCount>=20) {
         cycleCount = 0;
      }
      //      Heater::toggle();
      //      Fan::toggle();
      Heater::write(cycleCount<heaterDutycycle);
      Fan::write(cycleCount<fanDutycycle);
#else
      // Variable period PWM
      int wholePart;
      heaterDutycount += heaterDutycycle;
      wholePart = heaterDutycount/100;
      heaterDutycount -= 100*wholePart;
      Heater::write(wholePart>0);
      HeaterLed::write(wholePart>0);

      if (fanKick>0) {
         // Still kicking
         fanKick--;
         Fan::set();
         FanLed::on();
      }
      else {
         // PWM
         fanDutycount += fanDutycycle;
         wholePart = fanDutycount/100;
         fanDutycount -= 100*wholePart;
         Fan::write(wholePart>0);
         FanLed::write(wholePart>0);
      }
#endif
   }

public:
   /**
    * Create Zero-crossing PWM
    *
    * @param fanKickTime Non-volatile variable used to control the fan kick time applied when starting
    */
   ZeroCrossingPwm(USBDM::Nonvolatile<int> &fanKickTime) : fanKickTime(fanKickTime) {
      initialise();
   }

private:
   static void initialise() {
      heaterDutycycle = 0;
      fanDutycycle    = 0;
      HeaterLed::init();
      Heater::setOutput();
      Heater::low();
      FanLed::init();
      Fan::setOutput();
      Fan::low();

      /**
       * Set up comparator to generate events on zero-crossings
       */
      Vmains::enable();
      Vmains::setCallback(callbackFunction);
      Vmains::enableRisingEdgeInterrupts();
      Vmains::enableFallingEdgeInterrupts();
      Vmains::setDacLevel(32, 1, true);
      Vmains::selectInputs(1,7);
   }

public:
   /**
    * Set duty cycle of fan
    *
    * @param dutycycle Percentage duty-cycle to set
    *
    * @note The fan is run at full speed for a short time when first started from 0% duty-cycle
    */
   void setFanDutycycle(int dutycycle) {
      if (dutycycle == 0) {
         // Ensure no kick in progress
         fanKick = 0;
      }
      else if (fanDutycycle == 0) {
         // Turn on - apply kick
         fanKick = fanKickTime;
      }
      fanDutycycle = dutycycle;
   }

   /**
    * Set duty cycle of heater
    *
    * @param dutycycle Percentage duty-cycle to set
    */
   static void setHeaterDutycycle(int dutycycle) {
      heaterDutycycle = dutycycle;
   }
   /**
    * Get duty cycle of fan
    *
    * @return dutycycle Percentage duty-cycle to set
    */
   static int getFanDutycycle() {
      return fanDutycycle;
   }

   /**
    * Get duty cycle of heater
    *
    * @return dutycycle Percentage duty-cycle to set
    */
   static int getHeaterDutycycle() {
      return heaterDutycycle;
   }
};

template<typename Heater, typename HeaterLed, typename Fan, typename FanLed, typename Vmains>
int  ZeroCrossingPwm<Heater, HeaterLed, Fan, FanLed, Vmains>::heaterDutycycle = 0;
template<typename Heater, typename HeaterLed, typename Fan, typename FanLed, typename Vmains>
int  ZeroCrossingPwm<Heater, HeaterLed, Fan, FanLed, Vmains>::fanDutycycle = 0;
template<typename Heater, typename HeaterLed, typename Fan, typename FanLed, typename Vmains>
int  ZeroCrossingPwm<Heater, HeaterLed, Fan, FanLed, Vmains>::fanKick = 0;

#endif /* SOURCES_ZEROCROSSING_PWM_H_ */
