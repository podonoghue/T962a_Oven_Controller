/*
 * Settings.cpp
 *
 * Looks after the non-volatile settings
 *
 *  Created on: 25 Sep 2016
 *      Author: podonoghue
 */
#include <stdio.h>
#include "settings.h"
#include "lcd_st7920.h"
#include "configure.h"

using namespace USBDM;

/*
 * Allocate settings variables to non-volatile storage
 */
__attribute__ ((section(".flexRAM")))
Nonvolatile<int> beepTime;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> minimumFanSpeed;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> t1Offset;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> t2Offset;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> t3Offset;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> t4Offset;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> profileIndex;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> fanKickTime;

/**
 * Constructor - initialises the non-volatile storage\n
 * Must be a singleton!
 */
Settings::Settings() {
   static int singletonFlag = false;

   assert (!singletonFlag);

   // Initialise EEPROM
   USBDM::FlashDriverError_t rc = initialiseEeprom(USBDM::Flash::eeprom2KBytes);
   if (rc == USBDM::FLASH_ERR_OK) {
      return;
   }
   // Other errors are ignored here but will have already set the USBDM error code

   initialiseSettings();
}

void Settings::initialiseSettings() {
   // Write initial value for non-volatile variables
   profiles[0] = am4300profile;
   profiles[1] = nc31profile;
   profiles[2] = syntechlfprofile;
   profiles[3] = rampspeed_testprofile;
   profiles[4] = pidcontrol_testprofile;
   profiles[5].reset();

   beepTime        = 1;
   minimumFanSpeed = 10;
   t1Offset        = 0;
   t2Offset        = 0;
   t3Offset        = 0;
   t4Offset        = 0;

   profileIndex    = 0;

   fanKickTime     = 10;
}

/**
 * Class to hold information about how to modify a non-volatile setting
 */
class Setting {
public:
   Nonvolatile<int> &nvVariable;
   const char *description;
   const int min;
   const int max;
   const int incr;
   void (*func)();

public:
   int get() const {
      return nvVariable;
   }

   void set(int value) const {
      if (value < min) {
         value = min;
      }
      if (value > min) {
         value = max;
      }
      this->nvVariable = value;
   }

   int increment() const {
      if ((nvVariable + incr) <= max) {
         nvVariable += incr;
      }
      return nvVariable;
   }

   int decrement() const {
      if ((nvVariable - incr) >= min) {
         nvVariable -= incr;
      }
      return nvVariable;
   }

   const char* getDescription() const {
      return description;
   }

   void action() const {
      if (func != nullptr) {
         func();
      }
   }
};

void testingScreen(const char *title, const char *params) {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   lcd.setInversion(true);
   lcd.putSpace(3); lcd.putString(title); lcd.putSpace(3);

   lcd.gotoXY(0, 2*lcd.FONT_HEIGHT);
   lcd.setInversion(false);
   lcd.putSpace(3); lcd.putString(params);

   lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(false); lcd.putSpace(88);
//   lcd.setInversion(true);  lcd.putString(" + ");    lcd.setInversion(false); lcd.putSpace(3);
//   lcd.setInversion(true);  lcd.putString(" - ");    lcd.setInversion(false); lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" Exit "); lcd.setInversion(false); lcd.putSpace(3);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

void testBeep() {
   char buff[lcd.LCD_WIDTH/lcd.FONT_WIDTH+2];
   snprintf(buff, sizeof(buff), "Duration = %ds", (int)beepTime);
   testingScreen("Test Beep", buff);
   Buzzer::high();
   static const auto abort = []() { return Buttons::getButton() != SW_NONE; };
   USBDM::wait((float)beepTime, abort);
   Buzzer::low();
}

void testFan() {
   char buff[lcd.LCD_WIDTH/lcd.FONT_WIDTH+2];
   snprintf(buff, sizeof(buff), "kick=%dcy speed=%d%%", (int)fanKickTime, (int)minimumFanSpeed);
   testingScreen("Fan Test", buff);
   ovenControl.setFanDutycycle(minimumFanSpeed);
   static const auto abort = []() { return Buttons::getButton() != SW_NONE; };
   USBDM::wait(100, abort);
   ovenControl.setFanDutycycle(0);
}
/**
 * Describes the menu
 */
static const Setting menu[] = {
      {minimumFanSpeed, "Reflow fan speed %3d%%",     5, 100, 5, testFan},
      {fanKickTime,     "Fan Kick Cycles  %3d",       0,  50, 1, testFan},
      {t1Offset,        "Thermo 1 Offset  %3d\x7F", -30,  30, 1, nullptr},
      {t2Offset,        "Thermo 2 Offset  %3d\x7F", -30,  30, 1, nullptr},
      {t3Offset,        "Thermo 3 Offset  %3d\x7F", -30,  30, 1, nullptr},
      {t4Offset,        "Thermo 4 Offset  %3d\x7F", -30,  30, 1, nullptr},
      {beepTime,        "Beep time        %3ds",      0,  30, 1, testBeep},
};

static constexpr int NUM_ITEMS         = sizeof(menu)/sizeof(menu[0]);
static constexpr int MAX_VISIBLE_ITEMS = 6;

void Settings::drawScreen() {
   // Adjust so selected item is visible
   if ((selection-offset) >= MAX_VISIBLE_ITEMS) {
      offset++;
   }
   if ((selection < offset)) {
      offset--;
   }
   lcd.clearFrameBuffer();
   lcd.setInversion(true); lcd.putString("Settings Menu"); lcd.setInversion(false);

   for (int item=0; item<NUM_ITEMS; item++) {
      if (item<offset) {
         continue;
      }
      if (item>(offset+MAX_VISIBLE_ITEMS)) {
         continue;
      }
      lcd.setInversion(item == selection);
      lcd.gotoXY(0, (item+1-offset)*8);
      lcd.printf(menu[item].getDescription(), menu[item].get());
   }
   lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(false); lcd.putSpace(4);
   lcd.setInversion(true);  lcd.putString(" ");      lcd.putUpArrow();   lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" ");      lcd.putDownArrow(); lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" + ");    lcd.setInversion(false);            lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" - ");    lcd.setInversion(false);            lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" Exit "); lcd.setInversion(false);            lcd.putSpace(3);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

/**
 * Display menu to modify settings
 */
void Settings::runMenu() {
   bool changed = true;
   for(;;) {
      if (changed) {
         drawScreen();
         changed = false;
      }
      switch(Buttons::getButton()) {
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
      case SW_F3:
         menu[selection].increment();
         drawScreen();
         menu[selection].action();
         changed = true;
         break;
      case SW_F4:
         menu[selection].decrement();
         drawScreen();
         menu[selection].action();
         changed = true;
         break;
      case SW_S:
         return;
      default:
         break;
      }
      __WFI();
   }
}

// Singleton
Settings settings;
