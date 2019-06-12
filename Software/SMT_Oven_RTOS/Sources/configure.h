/**
 * @file    Configure.h
 * @brief   Shared configuration
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_CONFIGURE_H_
#define SOURCES_CONFIGURE_H_

#include <string.h>
#include "derivative.h"
#include "hardware.h"

/** Test point */
using Tp1      = USBDM::GpioB<17, USBDM::ActiveHigh>;

class PulseTp {
public:
   PulseTp(unsigned pulseCount) {
      USBDM::CriticalSection cs;
      Tp1::set();
      pulseCount *= 2;
      while (pulseCount-->0) {
         for (unsigned index=0; index<20; index++) {
            __asm__("nop");
         }
         Tp1::toggle();
      }
      for (unsigned index=0; index<50; index++) {
         __asm__("nop");
      }
   }

   ~PulseTp() {
      Tp1::clear();
   }
};

#include "Max31855.h"
#include "SolderProfile.h"
#include "SwitchDebouncer.h"
#include "ZeroCrossingPwm.h"

#include "cmp.h"
#include "delay.h"
#include "spi.h"
#include "lcd_st7920.h"
#include "fonts.h"
#include "pid.h"
#include "settings.h"
#include "runProfile.h"

/** SPI_PCSx signals for SPI connected to LCD and Thermocouples */
static constexpr USBDM::SpiPeripheralSelect lcd_cs = USBDM::SpiPeripheralSelect_4;
static constexpr USBDM::SpiPeripheralSelect t1_cs  = USBDM::SpiPeripheralSelect_2;
static constexpr USBDM::SpiPeripheralSelect t2_cs  = USBDM::SpiPeripheralSelect_3;
static constexpr USBDM::SpiPeripheralSelect t3_cs  = USBDM::SpiPeripheralSelect_1;
static constexpr USBDM::SpiPeripheralSelect t4_cs  = USBDM::SpiPeripheralSelect_0;

/**
 * SPI used for LCD and Thermocouples
 */
extern USBDM::Spi0 spi;

#include "TemperatureSensors.h"
#include "CaseTemperatureMonitor.h"

/** Function buttons */
using F1Button = USBDM::GpioB<3, USBDM::ActiveLow>;
using F2Button = USBDM::GpioB<2, USBDM::ActiveLow>;
using F3Button = USBDM::GpioB<1, USBDM::ActiveLow>;
using F4Button = USBDM::GpioB<0, USBDM::ActiveLow>;

/** Select button */
using SButton  = USBDM::GpioB<16, USBDM::ActiveLow>;

/** Case fan PWM output */
using CaseFan  = USBDM::Ftm0::Channel<2>;

/** Spare fan PWM output */
using Spare    = USBDM::Ftm0::Channel<3>;

/**
 * Oven fan LED - Wrapper for GPIO
 */
class OvenFanLed : public USBDM::GpioC<6, USBDM::ActiveHigh> {
public:
   /**
    * Initialise LED
    */
   static void init() {
      using namespace USBDM;
      setOutput(PinDriveStrength_Low, PinDriveMode_PushPull, PinSlewRate_Slow);
   }
};
/**
 * Oven heater LED - Wrapper for GPIO
 */
class HeaterLed : public USBDM::GpioD<7, USBDM::ActiveHigh> {
public:
   /**
    * Initialise LED
    */
   static void init() {
      using namespace USBDM;
      setOutput(PinDriveStrength_Low, PinDriveMode_PushPull, PinSlewRate_Slow);
   }
};

/**
 * Oven fan GPIO
 */
using OvenFan    = USBDM::GpioC<1, USBDM::ActiveHigh>;

/**
 * Oven Heater GPIO
 */
using Heater     = USBDM::GpioC<2, USBDM::ActiveHigh>;

/**
 * Main cycle comparator used for Zero-crossing PWM
 */
using Vmains     = USBDM::Cmp0;

/**
 * LCD
 */
extern LCD_ST7920 lcd;

/** PWM for heater & oven fan */
extern ZeroCrossingPwm<Heater, HeaterLed, OvenFan, OvenFanLed, Vmains> ovenControl;

/** Switch debouncer for front panel buttons */
extern SwitchDebouncer<F1Button, F2Button, F3Button, F4Button, SButton> buttons;

/** PID controller sample interval - seconds */
constexpr float pidInterval = 0.25f;

/**
 * Buzzer
 */
class Buzzer : private USBDM::GpioC<5, USBDM::ActiveHigh> {
public:
   /**
    * Initialise buzzer
    */
   static void init() {
      using namespace USBDM;
      setOutput(PinDriveStrength_Low, PinDriveMode_PushPull, PinSlewRate_Slow);
   }
   /**
    * Sound buzzer with abort on button press.\n
    * Button press is consumed.
    */
   static void play() {
      // Test for any button press
      static auto btnTest = []() {
         return buttons.getButton(0) != SwitchValue::SW_NONE;
      };
      // Play buzzer for time or until button press
      on();
      USBDM::wait(beepTime, btnTest);
      off();
   }
};

/**
 * Thermocouples
 */
extern TemperatureSensors temperatureSensors;

/**
 * Monitor case temperature
 */
extern CaseTemperatureMonitor<CaseFan> caseTemperatureMonitor;

/**
 * Get oven temperature (for PID)
 * Averages multiple thermocouple inputs
 *
 * @return Averaged oven temperature
 */
extern float getTemperature();

/**
 * Set heater drive level (for PID)
 */
extern void outPutControl(float dutyCycle);

/**
 * PID controller
 */
extern Pid_T<getTemperature, outPutControl> pid;

/**
 * Mutex to protect Interactive and Remote control
 */
extern CMSIS::Mutex interactiveMutex;

#endif /* SOURCES_CONFIGURE_H_ */
