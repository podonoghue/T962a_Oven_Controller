/**
 * @file    Configure.h
 * @brief   Shared configuration
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_CONFIGURE_H_
#define SOURCES_CONFIGURE_H_

#include <solderProfiles.h>
#include <string.h>

#include "derivative.h"
#include "hardware.h"
#include "cmp.h"
#include "delay.h"
#include "spi.h"
#include "lcd_st7920.h"
#include "max31855.h"
#include "fonts.h"
#include "pid.h"
#include "zerocrossing_pwm.h"
#include "switch_debouncer.h"
#include "settings.h"
#include "runProfile.h"

/** Function buttons */
using F1Button = USBDM::GpioB<3, USBDM::ActiveLow>;
using F2Button = USBDM::GpioB<2, USBDM::ActiveLow>;
using F3Button = USBDM::GpioB<1, USBDM::ActiveLow>;
using F4Button = USBDM::GpioB<0, USBDM::ActiveLow>;

/** Select button */
using SButton  = USBDM::GpioB<16, USBDM::ActiveLow>;

/** PCS # for SPI */
constexpr int lcd_cs_num = 4;
constexpr int t1_cs_num  = 2;
constexpr int t2_cs_num  = 3;
constexpr int t3_cs_num  = 1;
constexpr int t4_cs_num  = 0;

/** Case fan PWM output */
using CaseFan  = USBDM::Ftm0Channel<2>;

/** Spare fan PWM output */
using Spare    = USBDM::Ftm0Channel<3>;

/**
 * Oven fan LED - Wrapper for GPIO
 */
class OvenFanLed : public USBDM::GpioC<6> {
public:
   /**
    * Turn on LED
    */
   static void on() {
      high();
   }
   /**
    * Turn off LED
    */
   static void off() {
      low();
   }
   /**
    * Initialise LED
    */
   static void init() {
     setOutput();
     off();
   }
};
/**
 * Oven heater LED - Wrapper for GPIO
 */
class HeaterLed : public USBDM::GpioD<7> {
public:
   /**
    * Turn on LED
    */
   static void on() {
      high();
   }
   /**
    * Turn off LED
    */
   static void off() {
      low();
   }
   /**
    * Initialise LED
    */
   static void init() {
     setOutput();
     off();
   }
};

/**
 * Oven fan GPIO
 */
using OvenFan    = USBDM::GpioC<1>;

/**
 * Oven Heater GPIO
 */
using Heater     = USBDM::GpioC<2>;

/**
 * Main cycle comparator used for Zero-crossing PWM
 */
using Vmains     = USBDM::Cmp0;

/**
 * SPI used for LCD and Thermocouples
 */
extern USBDM::Spi0 spi;

/**
 * LCD
 */
extern LCD_ST7920 lcd;

/** PIT timer channel for PID */
constexpr int pid_pit_channel          = 0;

/** PIT timer channel for button debouncer */
constexpr int button_pit_channel       = 1;

/** PIT timer channel for sequencing a solder profile */
constexpr int profile_pit_channel      = 2;

/** PIT timer channel for case temperature monitor. Controls case fan */
constexpr int caseMonitor_pit_channel  = 3;

/** PWM for heater & oven fan */
extern ZeroCrossingPwm <Heater, HeaterLed, OvenFan, OvenFanLed, Vmains> ovenControl;

/** Switch debouncer for front panel buttons */
extern SwitchDebouncer<F1Button, F2Button, F3Button, F4Button, SButton> buttons;

/** PID controller sample interval - seconds */
constexpr float pidInterval = 0.25f;

/**
 * Buzzer
 */
class Buzzer : private USBDM::GpioC<5> {
public:
   /**
    * Initialise buzzer
    */
   static void init() {
      Buzzer::setOutput();
      Buzzer::low();
   }
   /**
    * Sound buzzer with abort on button press.\n
    * Button press is consumed.
    */
   static void play() {
      high();
      USBDM::wait(beepTime, [](){ return buttons.getButton() != SwitchValue::SW_NONE; });
      low();
   }
};

/**
 * Set heater drive level
 */
extern void outPutControl(float dutyCycle);

/**
 * Thermocouples
 */
#include "temperatureSensors.h"
extern TemperatureSensors temperatureSensors;

/**
 * Monitor case temperature
 */
#include "caseTemperatureMonitor.h"
extern CaseTemperatureMonitor<CaseFan, caseMonitor_pit_channel> caseTemperatureMonitor;

/**
 * Get oven temperature
 * Averages multiple thermocouple inputs
 *
 * @return Averaged oven temperature
 */
extern float getTemperature();

/**
 * PID controller
 */
extern Pid_T<getTemperature, outPutControl> pid;

/**
 * Mutex to protect Interactive and Remote control
 */
extern CMSIS::Mutex *interactiveMutex;

#endif /* SOURCES_CONFIGURE_H_ */