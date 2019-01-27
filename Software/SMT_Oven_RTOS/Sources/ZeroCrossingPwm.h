/**
 * @file    ZeroCrossingPwm.h
 * @brief   Zero-crossing switch controller
 *
 *  Created on: 24 Sep 2016
 *      Author: podonoghue
 */
#ifndef HEADERS_ZEROCROSSINGPWM_H_
#define HEADERS_ZEROCROSSINGPWM_H_

#include "flash.h"
#include "cmp.h"

/**
 * Simple zero-crossing PWM for oven fan and heater controlled by zero-crossing SSDs
 *
 * The switching waveform will be synchronised to the mains zero crossing
 *
 * @tparam Heater     USBDM::Gpio controlling the oven heater SSD
 * @tparam HeaterLed  USBDM::Gpio controlling the oven heater LED
 * @tparam OvenFan    USBDM::Gpio controlling the oven fan SSD
 * @tparam FanLed     USBDM::Gpio controlling the oven fan LED
 * @tparam Vmains     USBDM::Cmp used for mains sensing
 */
template<typename Heater, typename HeaterLed, typename OvenFan, typename FanLed, typename Vmains>
class ZeroCrossingPwm {

private:
   /** Duty cycle for Heater */
   static int  heaterDutycycle;

   /** Duty cycle for OvenFan */
   static int  fanDutycycle;

   /** Count down for fan kick */
   static int  fanKick;

   /**
    * Number of mains half-cycles to run the fan before switching to PWM mode
    * This is to overcome the static friction of the fan on low duty-cycle
    */
   const USBDM::Nonvolatile<int> &fanKickTime;

   /*
    * Function is called on zero-crossings of the mains.
    * Implements a simple PWM with variable period (~20ms - ~1s @50Hz mains).
    */
   static void callbackFunction(USBDM::CmpStatus status) {
      (void)status;

      // Keeps track of heater drive
      static int heaterDutycount = 0;

      // Keeps track of fan drive
      static int fanDutycount = 0;

#if 0
      // Simple PWM with fixed period
      static int cycleCount = 0;
      cycleCount++;
      if (cycleCount>=20) {
         cycleCount = 0;
      }
      //      Heater::toggle();
      //      OvenFan::toggle();
      Heater::write(cycleCount<heaterDutycycle);
      OvenFan::write(cycleCount<fanDutycycle);
#else
      // Variable period PWM
      int wholePart;
      heaterDutycount += heaterDutycycle;
      wholePart        = heaterDutycount/100;
      heaterDutycount -= 100*wholePart;

      Heater::write(wholePart>0);
      HeaterLed::write(wholePart>0);

      if (fanKick>0) {
         // Still kicking
         fanKick--;
         OvenFan::set();
         FanLed::on();
      }
      else {
         // PWM
         fanDutycount += fanDutycycle;
         wholePart = fanDutycount/100;
         fanDutycount -= 100*wholePart;
         OvenFan::write(wholePart>0);
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
   ZeroCrossingPwm(const USBDM::Nonvolatile<int> &fanKickTime) : fanKickTime(fanKickTime) {
//      initialise();
   }

   static void initialise() {
      using namespace USBDM;

      heaterDutycycle = 0;
      fanDutycycle    = 0;
      HeaterLed::init();
      Heater::setOutput();
      Heater::low();
      FanLed::init();
      OvenFan::setOutput();
      OvenFan::low();

      /**
       * Set up comparator to generate events on mains zero-crossings
       */
      Vmains::configure(
            CmpPower_HighSpeed,
            CmpHysteresis_3,
            CmpPolarity_Noninverted);
      Vmains::configureDac(32, CmpDacSource_Vdda);
      Vmains::selectInputs(Cmp0Input_CmpIn1,Cmp0Input_DacRef);
      Vmains::setCallback(callbackFunction);
      Vmains::enableInterrupts(CmpInterrupt_Both);
      Vmains::enableNvicInterrupts(NvicPriority_Normal);
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
    * @return Duty cycle Percentage duty-cycle to set
    */
   static int getHeaterDutycycle() {
      return heaterDutycycle;
   }
};

template<typename Heater, typename HeaterLed, typename OvenFan, typename FanLed, typename Vmains>
int  ZeroCrossingPwm<Heater, HeaterLed, OvenFan, FanLed, Vmains>::heaterDutycycle = 0;
template<typename Heater, typename HeaterLed, typename OvenFan, typename FanLed, typename Vmains>
int  ZeroCrossingPwm<Heater, HeaterLed, OvenFan, FanLed, Vmains>::fanDutycycle = 0;
template<typename Heater, typename HeaterLed, typename OvenFan, typename FanLed, typename Vmains>
int  ZeroCrossingPwm<Heater, HeaterLed, OvenFan, FanLed, Vmains>::fanKick = 0;

#endif /* HEADERS_ZEROCROSSINGPWM_H_ */
