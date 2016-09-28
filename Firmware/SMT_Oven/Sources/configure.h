/*
 * Configure.h
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_CONFIGURE_H_
#define SOURCES_CONFIGURE_H_

#include "derivative.h"
#include "hardware.h"
#include "delay.h"
#include "spi.h"
#include "fonts.h"
#include "string.h"
#include "profiles.h"
#include "pid.h"
#include "cmp.h"
#include "zerocrossing_pwm.h"
#include "switch_debouncer.h"
#include "lcd_st7920.h"
#include "max31855.h"
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
using OvenFan  = USBDM::GpioC<1>;
using Heater   = USBDM::GpioC<2>;
using CaseFan  = USBDM::Ftm0Channel<2>;
using Spare    = USBDM::Ftm0Channel<3>;

// Simple GPIO outputs
using Buzzer    = USBDM::GpioC<5>;
using LedFan    = USBDM::GpioD<7>;
using LedHeater = USBDM::GpioC<7>;

using Vmains    = USBDM::Cmp0;

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
extern ZeroCrossingPwm <Heater, OvenFan, Vmains> ovenControl;

using Buttons = SwitchDebouncer<button_pit_channel, F1Button, F2Button, F3Button, F4Button, SButton>;

#endif /* SOURCES_CONFIGURE_H_ */
