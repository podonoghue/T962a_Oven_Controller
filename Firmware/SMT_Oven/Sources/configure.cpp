/*
 * configure.cpp
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

/** Temperature sensors */
Max31855 temperatureSensors[4] = {
   Max31855(spi, t1_cs_num, t1Offset),
   Max31855(spi, t2_cs_num, t2Offset),
   Max31855(spi, t3_cs_num, t3Offset),
   Max31855(spi, t4_cs_num, t4Offset),
};

/** PWM for heater & oven fan */
ZeroCrossingPwm <Heater, OvenFan, Vmains> ovenControl(fanKickTime);

/** Switch debouncer for front panel buttons */
SwitchDebouncer<button_pit_channel, F1Button, F2Button, F3Button, F4Button, SButton> buttons;

/**
 * PID controller parameters
 */
static constexpr float kp = 4.0f;
static constexpr float ki = 0.0f;
static constexpr float kd = 0.0f;

float getTemperature() {
   int foundSensorCount = 0;
   float value = 0;
   for (int overSample=0; overSample<4; overSample++) {
      for (int t=0; t<=3; t++) {
         float temperature, coldReference;
         int status = temperatureSensors[t].getReading(temperature, coldReference);
         if (status == 0) {
            foundSensorCount++;
            value += temperature;
         }
      }
   }
   if (foundSensorCount==0) {
      // Safe value to return!
      return 10000.0;
   }
   return value/foundSensorCount;
}

void setHeater(float dutyCycle) {
   ovenControl.setHeaterDutycycle(dutyCycle);
}

/** PID controller */
Pid_T<getTemperature, setHeater, pid_pit_channel> pid(kp, ki, kd, pidInterval, 0, 100);

/** Runs a SMT profile */
RunProfile<profile_pit_channel> runProfile;
