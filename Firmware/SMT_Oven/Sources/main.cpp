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
#include "editProfile.h"

class profilesMenu {

public:
   static void run() {
      unsigned profileIndex = ::profileIndex;
      bool needUpdate = true;

      for(;;) {
         if (needUpdate) {
            RunProfile::drawProfile(profiles[profileIndex]);
            lcd.refreshImage();
            lcd.setGraphicMode();
            needUpdate = false;
         }
         switch(buttons.getButton()) {
         case SW_F1:
            if (profileIndex>0) {
               profileIndex--;
               needUpdate = true;
            }
            break;
         case SW_F2:
            if ((profileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
               profileIndex++;
               needUpdate = true;
            }
            break;
         case SW_F3:
            EditProfile::run(profiles[profileIndex]);
            needUpdate = true;
            break;
         case SW_S:
            ::profileIndex.operator =(profileIndex);
            return;
         default:
            break;
         }
         __WFI();
      };
   }
};

class MainMenu {

private:
   int selection = 0;
   int offset    = 0;

   static constexpr int MAX_VISIBLE_ITEMS = 6;
   static constexpr int NUM_ITEMS = 6;

   void drawScreen() {
      static const char *menu[NUM_ITEMS] = {
            "Thermocouple check",
            "Manual Mode",
            "Settings",
            "Select Profile",
            "Run Profile",
            "Edit Current Profile",
      };

      // Adjust so selected item is visible
      if ((selection-offset) >= MAX_VISIBLE_ITEMS) {
         offset++;
      }
      if ((selection < offset)) {
         offset--;
      }
      lcd.setInversion(false);  lcd.clearFrameBuffer();
      lcd.setInversion(true);   lcd.putString("  Main Menu\n"); lcd.setInversion(false);
      for (int item=0; item<NUM_ITEMS; item++) {
         if (item<offset) {
            continue;
         }
         if (item>(offset+MAX_VISIBLE_ITEMS)) {
            continue;
         }
         lcd.setInversion(item == selection);
         lcd.gotoXY(0, (item-offset+1)*lcd.FONT_HEIGHT);
         lcd.putString(menu[item]);
      }
      lcd.setInversion(false);
      //      lcd.gotoXY(0, lcd.LCD_HEIGHT-2*lcd.FONT_HEIGHT);
      //      lcd.putString(profiles[profileIndex].description);
      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true);  lcd.putString(" ");     lcd.putUpArrow();   lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true);  lcd.putString(" ");     lcd.putDownArrow(); lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(false); lcd.putSpace(48);
      lcd.setInversion(true);  lcd.putString(" SEL "); lcd.setInversion(false);            lcd.putSpace(3);

      lcd.refreshImage();
      lcd.setGraphicMode();
   }

public:
   void run() {
      bool changed = true;

      for(;;) {
         if (changed) {
            drawScreen();
            changed = false;
         }
         switch(buttons.getButton()) {
         case SW_F1:
            if (selection>0) {
               selection--;
               changed = true;
            }
            break;
         case SW_F2:
            if (selection<(NUM_ITEMS-1)) {
               selection++;
               changed = true;
            }
            break;
         case SW_S:
            switch(selection) {
            case 0:
               RunProfile::monitor();
               changed = true;
               break;
            case 1:
               RunProfile::manualMode();
               changed = true;
               break;
            case 2:
               settings.runMenu();
               changed = true;
               break;
            case 3:
               profilesMenu::run();
               changed = true;
               break;
            case 4:
               RunProfile::run(profiles[profileIndex]);
               changed = true;
               break;
            case 5:
               EditProfile::run(profiles[profileIndex]);
               changed = true;
               break;
//            case 6:
//               stepResponse.run();
//               changed = true;
//               break;
            default:
               break;
            }
            break;
            default:
               break;
         }
         __WFI();
      }
   }
};

MainMenu mainMenu;

void initialise() {
   Buzzer::init();
   OvenFanLed::setOutput();
   OvenFanLed::low();
   HeaterLed::setOutput();
   HeaterLed::low();
   Spare::enable();
   Spare::setDutyCycle(0);
}

int main() {
   printf("Starting\n");

   USBDM::mapAllPins();

   if (USBDM::getError() != USBDM::E_NO_ERROR) {
      char buff[100];
      lcd.clear();
      lcd.printf("Error in initialisation \n  %s\n", USBDM::getErrorMessage());
      lcd.putString(buff);
      printf("Error in initialisation \n  %s\n", USBDM::getErrorMessage());
   }
   initialise();

   lcd.clear();

   pid.setSetpoint(0);

   for (;;) {
      mainMenu.run();
      __WFI();
   }
   return 0;
}
