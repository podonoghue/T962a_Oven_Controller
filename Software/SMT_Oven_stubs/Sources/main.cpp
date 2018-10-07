/*
 ============================================================================
 * @file    main.cpp (180.ARM_Peripherals)
 * @brief   Basic C++ demo using GPIO class
 *
 *  Created on: 10/1/2016
 *      Author: podonoghue
 ============================================================================
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "cmsis.h"
#include "usb_cdc_interface.h"
#include "system.h"
#include "derivative.h"
#include "hardware.h"
#include "delay.h"
#include "spi.h"
#include "fonts.h"
#include "configure.h"
#include "pid.h"
#include "settings.h"
#include "messageBox.h"
#include "mainMenu.h"
#include "usb.h"
#include "utilities.h"
#include "EditProfile.h"
#include "i2c.h"

class profilesMenu {

public:
   static void run() {
      unsigned profileIndex = ::currentProfileIndex;
      bool needUpdate = true;

      for(;;) {
         if (needUpdate) {
            RunProfile::drawProfile(profiles[profileIndex]);
            lcd.refreshImage();
            lcd.setGraphicMode();
            needUpdate = false;
         }
         switch(buttons.getButton()) {
         case SwitchValue::SW_F1:
            if (profileIndex>0) {
               profileIndex--;
               needUpdate = true;
            }
            break;
         case SwitchValue::SW_F2:
            if ((profileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
               profileIndex++;
               needUpdate = true;
            }
            break;
         case SwitchValue::SW_F3:
            EditProfile::run(profiles[profileIndex]);
            needUpdate = true;
            break;
         case SwitchValue::SW_S:
            ::currentProfileIndex.operator =(profileIndex);
            return;
         default:
            break;
         }
         __WFI();
      };
   }
};

using namespace USBDM;

void initialise() {
   Buzzer::init();
   OvenFanLed::init();
   HeaterLed::init();
   Spare::enable();
   Spare::setDutyCycle(0);
   ovenControl.initialise();
}

int main() {

   console.writeln("Starting Oven");

   initialise();

   Usb0::initialise();

   mapAllPins();

   if (getError() != E_NO_ERROR) {
      char buff[100];
      lcd.clear();
      lcd.printf("Error in initialisation \n  %s\n", getErrorMessage());
      console.write(buff);
   }

   MainMenu::run();

   // Should not reach here
   for (;;) {
      __BKPT();
   }
   return 0;
}
