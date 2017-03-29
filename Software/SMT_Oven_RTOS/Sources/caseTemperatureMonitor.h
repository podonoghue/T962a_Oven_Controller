/**
 * @file    caseTemperatureMonitor.h
 * @brief   Case Temperature Monitor
 *
 *  Created on: 1Oct.,2016
 *      Author: podonoghue
 */

#ifndef SOURCES_CASETEMPERATUREMONITOR_CPP_
#define SOURCES_CASETEMPERATUREMONITOR_CPP_

#include "cmsis.h"
#include "max31855.h"
#include "temperatureSensors.h"

/**
 * Monitor for case temperature \n
 * Uses CMSIS timer callback
 *
 * @tparam CaseFan      PWM controlling the case fan
 * @tparam startTemp    Temperature at which to start the fan at MIN_FAN_SPEED %
 * @tparam maxTemp      Temperature at which the fan is to be 100% on
 *
 */
template<typename CaseFan, int startTemp=35, int maxTemp=45>
class CaseTemperatureMonitor {
   static TemperatureSensors &tempSensor;

   // Minimum speed to run the fan at
   static constexpr int MIN_FAN_SPEED = 10;

   static void checkCaseTemp(const void *) {
      float coldReference = tempSensor.getCaseTemperature();
      int dutyCycle = MIN_FAN_SPEED + (100*(coldReference-startTemp))/(maxTemp-startTemp);
      if (dutyCycle<MIN_FAN_SPEED) {
         dutyCycle = 0;
      }
      if (dutyCycle>100) {
         dutyCycle = 100;
      }
      CaseFan::setDutyCycle(dutyCycle);
   }
   CMSIS::Timer<osTimerPeriodic> timer{checkCaseTemp};

public:
   /*
    * Create case temperature monitor
    *
    * @tparam Sensor       Temperature sensor
    */
   CaseTemperatureMonitor() {
   }

   void initialise() {
      CaseFan::enable();
      CaseFan::setPeriod(20*USBDM::ms);
      CaseFan::setDutyCycle(0);
      timer.create();
      timer.start(1000 /* ms */);
   }
};

template<typename CaseFan, int startTemp, int maxTemp>
TemperatureSensors &CaseTemperatureMonitor<CaseFan, startTemp, maxTemp>::tempSensor = temperatureSensors;

#endif /* SOURCES_CASETEMPERATUREMONITOR_CPP_ */
