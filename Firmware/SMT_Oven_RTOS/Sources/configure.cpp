/**
 * @file    configure.cpp
 * @brief   Configuration
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#include "configure.h"
#include "settings.h"

// Note: Most objects are stateless static objects declared in Configure.h

/** SPI used for LCD and Thermocouples */
USBDM::Spi0 spi;

/** LCD */
LCD_ST7920 lcd(spi, lcd_cs_num);

/** PWM for heater & oven fan */
ZeroCrossingPwm <Heater, HeaterLed, OvenFan, OvenFanLed, Vmains> ovenControl(fanKickTime);

/** Switch debouncer for front panel buttons */
SwitchDebouncer<F1Button, F2Button, F3Button, F4Button, SButton> buttons;

/**
 * PID controller parameters
 */
static constexpr float kp = 20.0;   //4.0f; // 20.0
static constexpr float ki =  0.016; //0.0f; //  0.016
static constexpr float kd = 62.5;   //0.0f; // 62.5

/**
 * Set output controlling oven
 *
 * @param dutyCycle Controls the Heater/Fan
 */
void outPutControl(float dutyCycle) {
   int heaterDutycycle;
   int fanDutycycle;

   if (dutyCycle>=0) {
      heaterDutycycle = dutyCycle;
      fanDutycycle    = minimumFanSpeed;
   }
   else {
      heaterDutycycle = 0;
      fanDutycycle    = -dutyCycle;
      if (fanDutycycle<minimumFanSpeed) {
         fanDutycycle = minimumFanSpeed;
         heaterDutycycle = 10;
      }
   }
   ovenControl.setHeaterDutycycle(heaterDutycycle);
   ovenControl.setFanDutycycle(fanDutycycle);
}

/**
 * Get oven temperature
 * Averages multiple thermocouple inputs
 *
 * @return Averaged oven temperature
 */
float getTemperature() {
   return temperatureSensors.getTemperature();
}

/** PID controller */
Pid_T<getTemperature, outPutControl> pid(kp, ki, kd, pidInterval, -100, 100);

/** Thermocouples */
TemperatureSensors temperatureSensors;

/** Monitor for case temperature */
CaseTemperatureMonitor<CaseFan, caseMonitor_pit_channel> caseTemperatureMonitor;

/**
 * Mutex to protect Interactive and Remote control
 */
CMSIS::Mutex *interactiveMutex;
