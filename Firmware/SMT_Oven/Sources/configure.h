/*
 * Configure.h
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_CONFIGURE_H_
#define SOURCES_CONFIGURE_H_

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
#include "profiles.h"
#include "zerocrossing_pwm.h"
#include "switch_debouncer.h"
#include "settings.h"

// Function buttons
using F1Button = USBDM::GpioB<3>;
using F2Button = USBDM::GpioB<2>;
using F3Button = USBDM::GpioB<1>;
using F4Button = USBDM::GpioB<0>;

// Select button
using SButton  = USBDM::GpioB<16>;

// PCS # for SPI
constexpr int lcd_cs_num = 4;
constexpr int t1_cs_num  = 2;
constexpr int t2_cs_num  = 3;
constexpr int t3_cs_num  = 0;
constexpr int t4_cs_num  = 1;

// PWM outputs
using CaseFan  = USBDM::Ftm0Channel<2>;
using Spare    = USBDM::Ftm0Channel<3>;

// Simple GPIO outputs
using OvenFan    = USBDM::GpioC<1>;
using OvenFanLed = USBDM::GpioD<7>;
using Heater     = USBDM::GpioC<2>;
using HeaterLed  = USBDM::GpioC<6>;

//using Buzzer     = USBDM::GpioC<5>;

using Vmains     = USBDM::Cmp0;

// SPI used for LCD and Thermocouples
extern USBDM::Spi0 spi;

// LCD
extern LCD_ST7920 lcd;

// Thermocouples
extern Max31855 temperatureSensors[4];

// PIT timer channels
constexpr int pid_pit_channel     = 0;
constexpr int button_pit_channel  = 1;
constexpr int profile_pit_channel = 2;

// PWM for heater & oven fan
extern ZeroCrossingPwm <Heater, HeaterLed, OvenFan, OvenFanLed, Vmains> ovenControl;

// Switch debouncer for front panel buttons
extern SwitchDebouncer<button_pit_channel, F1Button, F2Button, F3Button, F4Button, SButton> buttons;

/** PID controller sample interval */
constexpr float pidInterval = 1000 * USBDM::ms;

/**
 * Get oven temperature
 * Averages multiple thermocouple inputs
 */
extern float getTemperature();
/*
 * Set heater drive level
 */
extern void setHeater(float dutyCycle);
/**
 * PID controller
 */
extern Pid_T<getTemperature, setHeater, pid_pit_channel> pid;

/**
 * Buzzer
 */
class Buzzer : public USBDM::GpioC<5> {
public:
   // Sound buzzer with abort on button press
   static void play() {
      high();
      USBDM::wait(beepTime, [](){ return buttons.getButton() != SW_NONE; });
      low();
   }
};

#include "runProfile.h"

/** Runs a SMT profile */
extern RunProfile<profile_pit_channel> runProfile;

#endif /* SOURCES_CONFIGURE_H_ */

