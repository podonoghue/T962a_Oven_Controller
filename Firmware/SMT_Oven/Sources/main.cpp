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
#include "system.h"
#include "derivative.h"
#include "hardware.h"
#include "delay.h"
#include "spi.h"
#include "fonts.h"
#include "configure.h"
#include "pid.h"
#include "settings.h"

//void logProgress() {
//   lcd.clearFrameBuffer();
//   lcd.putString(" Oven Status\n");
//   lcd.drawHorizontalLine(lcd.FONT_HEIGHT+1);
//   lcd.gotoXY(0, lcd.FONT_HEIGHT+4);
//
//   unsigned ticks = pid.getTicks();
//   int temperature = (int)pid.getInput();
//   lcd.printf("Time        = %d\nTemperature = %d\n", (int)(ticks*pidInterval), temperature);
//   lcd.refreshImage();
//   lcd.setGraphicMode();
//}

void manualMode() {
   pid.setSetpoint(100);
   pid.enable(false);
   for(;;) {
      lcd.clearFrameBuffer();

      lcd.setInversion(true); lcd.putString("Manual Mode\n"); lcd.setInversion(false);

      lcd.printf("On Time   = %5.1fs\n", pid.getElapsedTime());

      lcd.printf("Set Temp  = %5.1f\x7F\n", pid.getSetpoint());

      lcd.printf("Oven Temp = %5.1f\x7F\n", getTemperature());

      lcd.printf("Heater = %s\n", pid.isEnabled()?"On":"Off");

      if (ovenControl.getFanDutycycle() == 0) {
         lcd.printf("Fan    = Off       \n");
      }
      else if (ovenControl.getFanDutycycle() == 100) {
         lcd.printf("Fan    = On        \n");
      }
      else {
         lcd.printf("Fan    = Slow (%d%%) \n", ovenControl.getFanDutycycle());
      }

      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Fan");  lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Heat"); lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("+");    lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("-");    lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Exit"); lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);

      lcd.refreshImage();
      lcd.setGraphicMode();

      /**
       * Safety check
       * Turn off after 800 seconds of operation
       */
      if (pid.getElapsedTime()>800.0) {
         pid.enable(false);
         ovenControl.setHeaterDutycycle(0);
      }
      switch (buttons.getButton()) {
      case SW_F1:
         // Fan toggle
         if (ovenControl.getFanDutycycle()>0) {
            // Fan on - turn off
            ovenControl.setFanDutycycle(0);
         }
         else {
            // Fan off - turn on
            if (pid.isEnabled()) {
               // Heater on - recirculate fan speed
               ovenControl.setFanDutycycle(minimumFanSpeed);
            }
            else {
               // Heater off - cooling fan speed
               ovenControl.setFanDutycycle(100);
            }
         }
         break;
      case SW_F2:
         // Heater toggle
         if (pid.isEnabled()) {
            // Turn off heater
            pid.enable(false);
            ovenControl.setHeaterDutycycle(0);
            // Leave fan running on low
         }
         else {
            // Turn on heater
            pid.enable(true);
            // Recirculate fan speed
            ovenControl.setFanDutycycle(minimumFanSpeed);
         }
         break;
      case SW_F3:
      {
         // Increase Temp
         int t = pid.getSetpoint();
         if (t<255) {
            pid.setSetpoint(t + 5);
         }
      }
      break;
      case SW_F4:
      {
         // Decrease Temp
         int t = pid.getSetpoint();
         if (t>5) {
            pid.setSetpoint(t - 5);
         }
      }
      break;
      case SW_S:
         // Exit
         pid.enable(false);
         ovenControl.setFanDutycycle(0);
         ovenControl.setHeaterDutycycle(0);
         return;
      default:
         break;
      }
//      __WFI();
   }
}

class profilesMenu {

private:
   static constexpr int xOrigin   = 17; // Pixels from left edge
   static constexpr int yOrigin   = 10; // Pixels from bottom edge
   static constexpr int vGridSize = 10; // 10 pixels = 50 degrees
   static constexpr int hGridSize = 10; // 10 pixels = 1 minute, 1 pixel = 6s

   static void drawAxis(const char *name) {
      lcd.setInversion(false);
      lcd.clearFrameBuffer();

      // Vertical axis
      lcd.drawVerticalLine(xOrigin);
      for (int temp=50; temp<300; temp+=50) {
         lcd.gotoXY(0, lcd.LCD_HEIGHT-yOrigin-((temp*vGridSize)/50)-2);
         if (temp<100) {
            lcd.putSpace(5);
         }
         else {
            lcd.putSmallDigit(temp/100);
         }
         lcd.putSmallDigit((temp/10)%10);
         lcd.putSmallDigit(temp%10);
      }
      // Horizontal axis
      lcd.drawHorizontalLine(lcd.LCD_HEIGHT-yOrigin);
      for (int time=1; time<=8; time++) {
         lcd.gotoXY(xOrigin+(time*hGridSize)-2, lcd.LCD_HEIGHT-8);
         lcd.putChar('0'+time);
      }
      lcd.putSpace(4);
      lcd.putString("min");
      // Grid
      for (int y=lcd.LCD_HEIGHT-yOrigin; y>0; y-=vGridSize) {
         for (int x=xOrigin; x<lcd.LCD_WIDTH-2; x+=hGridSize) {
            lcd.drawPixel(x,y);
         }
      }
      // Menu
      constexpr int xMenuOffset = xOrigin+90;
      constexpr int yMenuOffset = 11;
      lcd.gotoXY(xMenuOffset, yMenuOffset);
      lcd.setInversion(true);
      lcd.putString("F1"); lcd.putRightArrow(); lcd.putSpace(2);
      lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*1);
      lcd.putString("F2"); lcd.putLeftArrow(); lcd.putSpace(2);
      lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*2);
      lcd.putString("S "); lcd.putEnter(); lcd.putSpace(2);
      lcd.setInversion(false);

      // Name
      constexpr int xNameOffset = xOrigin+2;
      constexpr int yNameOffset = 0;
      lcd.gotoXY(xNameOffset, yNameOffset);
      lcd.putString(name);
   }

   static void drawProfile(const NvSolderProfile &profile) {
      drawAxis(profile.description);
      for (unsigned int index=0; index<(sizeof(SolderProfile::profile)/sizeof(SolderProfile::profile[0])); index++) {
         int x = xOrigin+(index*hGridSize)/6;
         int y = lcd.LCD_HEIGHT-yOrigin-(profile.profile[index]*vGridSize/50);
         lcd.drawPixel(x,y);
      }
      lcd.refreshImage();
      lcd.setGraphicMode();
   }

public:
   static void run() {
      unsigned profileIndex = ::profileIndex;
      bool needUpdate = true;

      for(;;) {
         if (needUpdate) {
            drawAxis(profiles[profileIndex].description);
            drawProfile(profiles[profileIndex]);
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
            if (((profileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) &&
                  (profiles[profileIndex+1].description[0] != '\0')) {
               profileIndex++;
               needUpdate = true;
            }
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

void thermocoupleCheck() {
   do {
      runProfile.thermocoupleStatus();
      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(false); lcd.putSpace(94);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Exit"); lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);

      lcd.refreshImage();
      lcd.setGraphicMode();
      __WFI();
   } while (buttons.getButton() != SW_S);
}


class MainMenu {

private:
   uint selection     = 0;
   uint lastSelection = 0;

   void drawScreen() {
      static const char *menu[] = {
            "Thermocouple check",
            "Manual Mode",
            "Settings",
            "Select Profile",
            "Run Profile",
      };
      lcd.clearFrameBuffer();
      lcd.setInversion(true); lcd.putString("Main Menu"); lcd.setInversion(false);
      for (uint item=0; item<(sizeof(menu)/sizeof(menu[0])); item++) {
         lcd.setInversion(item == selection);
         lcd.gotoXY(0, (item+1)*lcd.FONT_HEIGHT);
         lcd.putString(menu[item]);
      }
      lcd.setInversion(false);
      lcd.gotoXY(0, lcd.LCD_HEIGHT-2*lcd.FONT_HEIGHT);
      lcd.putString(profiles[profileIndex].description);
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
            if (selection<4) {
               selection++;
               changed = true;
            }
            break;
         case SW_S:
            switch(selection) {
            case 0:
               thermocoupleCheck();
               changed = true;
               break;
            case 1:
               manualMode();
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
               runProfile.run();
               changed = true;
               break;
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
   Buzzer::setOutput();
   Buzzer::low();
   OvenFanLed::setOutput();
   OvenFanLed::low();
   HeaterLed::setOutput();
   HeaterLed::low();
   CaseFan::enable();
   CaseFan::setDutyCycle(0);
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
