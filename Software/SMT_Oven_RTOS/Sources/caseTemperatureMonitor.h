/**
 * @file    caseTemperatureMonitor.h
 * @brief   Case Temperature Monitor
 *
 *  Created on: 1Oct.,2016
 *      Author: podonoghue
 */

#ifndef SOURCES_CASETEMPERATUREMONITOR_CPP_
#define SOURCES_CASETEMPERATUREMONITOR_CPP_

#include "Max31855.h"
#include "TemperatureSensors.h"
#include "cmsis.h"

/**
 * Monitor for case temperature \n
 * Uses CMSIS timer callback
 *
 * @tparam CaseFan      PWM controlling the case fan
 * @tparam START_TEMP   Temperature at which to start the fan at MIN_FAN_SPEED %
 * @tparam MAX_TEMP     Temperature at which the fan is to be 100% on
 */
template<typename CaseFan, int START_TEMP=35, int MAX_TEMP=45>
class CaseTemperatureMonitor : private CMSIS::TimerClass {
   TemperatureSensors &tempSensor;

   // Minimum speed to run the fan at
   static constexpr int MIN_FAN_SPEED = 10;

   /**
    * Called at a regular interval to check case temperature
    */
   void callback() override {
      float coldReference = tempSensor.getCaseTemperature();
      int dutyCycle = MIN_FAN_SPEED + (100*(coldReference-START_TEMP))/(MAX_TEMP-START_TEMP);
      if (dutyCycle<MIN_FAN_SPEED) {
         dutyCycle = 0;
      }
      if (dutyCycle>100) {
         dutyCycle = 100;
      }
      CaseFan::setDutyCycle(dutyCycle);
   }

public:
   /*
    * Create case temperature monitor
    *
    * @param tempSensor Temperature sensor
    */
   CaseTemperatureMonitor(TemperatureSensors &tempSensor) : tempSensor(tempSensor) {
      CaseFan::enable();
      CaseFan::setPeriod(20*USBDM::ms);
      CaseFan::setDutyCycle(0);

      // Check every 5 seconds
      start(5000 /* ms */);
   }
};
#endif /* SOURCES_CASETEMPERATUREMONITOR_CPP_ */
