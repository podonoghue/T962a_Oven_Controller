/*
 * configure.cpp
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#include "configure.h"
#include "settings.h"

// Note: Most objects are stateless static objects declared in Configure.h

// SPI used for LCD and Thermocouples
USBDM::Spi0 spi;

// LCD
LCD_ST7920 lcd(spi, lcd_cs_num);

// Temperature sensors
Max31855 temperatureSensors[4] = {
   Max31855(spi, t1_cs_num, t1Offset),
   Max31855(spi, t2_cs_num, t2Offset),
   Max31855(spi, t3_cs_num, t3Offset),
   Max31855(spi, t4_cs_num, t4Offset),
};

// PWM for heater & oven fan
ZeroCrossingPwm <Heater, OvenFan, Vmains> ovenControl(fanKickTime);
