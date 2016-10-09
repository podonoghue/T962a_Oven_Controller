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
Nonvolatile<bool> t1Enable;

__attribute__ ((section(".flexRAM")))
Nonvolatile<bool> t2Enable;

__attribute__ ((section(".flexRAM")))
Nonvolatile<bool> t3Enable;

__attribute__ ((section(".flexRAM")))
Nonvolatile<bool> t4Enable;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> profileIndex;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> fanKickTime;

__attribute__ ((section(".flexRAM")))
USBDM::Nonvolatile<int> maxHeaterTime;

extern const Setting fanSetting;
extern const Setting kickSetting;
extern const Setting heaterSetting;
extern const Setting thermo1Setting;
extern const Setting thermo2Setting;
extern const Setting thermo3Setting;
extern const Setting thermo4Setting;
extern const Setting beepSetting;

/**
 * Constructor - initialises the non-volatile storage\n
 * Must be a singleton!
 */
Settings::Settings() : Flash() {
   // Initialise EEPROM
   USBDM::FlashDriverError_t rc = initialiseEeprom(USBDM::Flash::eeprom2KBytes);
   if (rc == USBDM::FLASH_ERR_OK) {
      return;
   }
   // Errors are ignored here but will have already set the USBDM error code
   initialiseSettings();
}

void Settings::initialiseSettings() {

   // Write initial value for non-volatile variables
   unsigned i=0;
   profiles[i++] = am4300profileA;
   profiles[i++] = am4300profileB;
   profiles[i++] = nc31profile;
   profiles[i++] = syntechlfprofile;
   for (;i<(sizeof(profiles)/sizeof(profiles[0]));i++) {
      char buff[sizeof(NvSolderProfile::description)];
      snprintf(buff, sizeof(buff), "Profile #%d", i);
      profiles[i] = defaultProfile;
      profiles[i].description = buff;
   }

   minimumFanSpeed = fanSetting.defaultValue;
   fanKickTime     = kickSetting.defaultValue;
   t1Offset        = thermo1Setting.defaultValue;
   t2Offset        = thermo2Setting.defaultValue;
   t3Offset        = thermo3Setting.defaultValue;
   t4Offset        = thermo4Setting.defaultValue;
   t1Enable        = true;
   t2Enable        = true;
   t3Enable        = true;
   t4Enable        = true;
   beepTime        = beepSetting.defaultValue;
   maxHeaterTime   = heaterSetting.defaultValue;

   profileIndex    = 0;
}

/**
 * Test Beeper
 */
void Settings::testBeep(const Setting *setting) {
   (void)setting;
//   char buff[lcd.LCD_WIDTH/lcd.FONT_WIDTH+2];
//   snprintf(buff, sizeof(buff), "Duration = %ds", (int)beepTime);
//   testingScreen("Test Beep", buff);
   Buzzer::play();
}

class FanTest {

private:
   /**
    * Display testing screen
    *
    * @param kickMode  Determines initial setting mode either Kick or Speed
    */
   static void testFanScreen(bool kickMode) {
      bool power = ovenControl.getFanDutycycle()>0;
      lcd.setInversion(false);
      lcd.clearFrameBuffer();

      lcd.setInversion(true);
      lcd.putSpace(3); lcd.putString("Fan Test"); lcd.putSpace(3);

      lcd.gotoXY(0, 2*lcd.FONT_HEIGHT);
      lcd.setInversion(false);
      lcd.putSpace(3); lcd.printf("Speed = %d%%\n\n",  (int)minimumFanSpeed);
      lcd.putSpace(3); lcd.printf("Kick  = %d cycles", (int)fanKickTime);

      lcd.gotoXY(55, lcd.LCD_HEIGHT-2*lcd.FONT_HEIGHT);
      if (kickMode) {
         lcd.setInversion(true);  lcd.putSpace(8); lcd.putString("Kick");  lcd.putSpace(7);
      }
      else {
         lcd.setInversion(true);  lcd.putSpace(5); lcd.putString("Speed");  lcd.putSpace(4);
      }
      lcd.gotoXY(3, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      if (power) {
         lcd.setInversion(true);  lcd.putSpace(3); lcd.putString("Off");  lcd.putSpace(2);
      }
      else {
         lcd.setInversion(true);  lcd.putSpace(6); lcd.putString("On");  lcd.putSpace(5);
      }
      lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true);  lcd.putSpace(3); lcd.putString("Sel");  lcd.putSpace(2);
      lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true);  lcd.putString(" + ");
      lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true);  lcd.putString(" - ");
      lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true);  lcd.putSpace(3); lcd.putString("Exit"); lcd.putSpace(2);

      lcd.refreshImage();
      lcd.setGraphicMode();
   }
public:
   /**
    * Test Fan operation
    */
   static void testFan(const Setting *setting) {
      bool kickMode = setting == &kickSetting;
      bool changed = true;
      for(;;) {
         bool motorOn = ovenControl.getFanDutycycle()>0;
         if (changed) {
            testFanScreen(kickMode);
         }
         switch(buttons.getButton()) {
         case SW_F1:
            if (motorOn) {
               ovenControl.setFanDutycycle(0);
            }
            else {
               ovenControl.setFanDutycycle(minimumFanSpeed);
            }
            changed = true;
            break;
         case SW_F2:
            kickMode = !kickMode;
            changed = true;
            break;
         case SW_F3F4:
            if (kickMode) {
               kickSetting.reset();
            }
            else {
               fanSetting.reset();
            }
            if (motorOn) {
               ovenControl.setFanDutycycle(minimumFanSpeed);
            }
            changed = true;
            break;
         case SW_F3:
            if (kickMode) {
               kickSetting.increment();
            }
            else {
               fanSetting.increment();
            }
            if (motorOn) {
               ovenControl.setFanDutycycle(minimumFanSpeed);
            }
            changed = true;
            break;
         case SW_F4:
            if (kickMode) {
               kickSetting.decrement();
            }
            else {
               fanSetting.decrement();
            }
            if (motorOn) {
               ovenControl.setFanDutycycle(minimumFanSpeed);
            }
            changed = true;
            break;
         case SW_S:
            return;
         default:
            break;
         }
         __WFI();
      }
      ovenControl.setFanDutycycle(0);
   }
};

/**
 * Describes the various settings for the menu
 * Also controls range and default values etc.
 *
 *                              NV variable       Description                 Min  Max  Inc  Default   Test function
 */
const Setting fanSetting     = {minimumFanSpeed, "Reflow fan speed %3d%%",     5,  100,  5,   30,      FanTest::testFan};
const Setting kickSetting    = {fanKickTime,     "Fan Kick Cycles  %3d",       0,   50,  1,   10,      FanTest::testFan};
const Setting thermo1Setting = {t1Offset,        "Thermo 1 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting thermo2Setting = {t2Offset,        "Thermo 2 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting thermo3Setting = {t3Offset,        "Thermo 3 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting thermo4Setting = {t4Offset,        "Thermo 4 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting heaterSetting  = {maxHeaterTime,   "Max heater time %4d",       10, 1000, 10, 600,       nullptr};
const Setting beepSetting    = {beepTime,        "Beep time        %3ds",      0,   30,  1,   0,       Settings::testBeep};

/**
 * Describes the settings and limits for same
 */
static const Setting * const menu[] = {
      &fanSetting,
      &kickSetting,
      &thermo1Setting,
      &thermo2Setting,
      &thermo3Setting,
      &thermo4Setting,
      &heaterSetting,
      &beepSetting,
};

static constexpr int NUM_ITEMS         = sizeof(menu)/sizeof(menu[0]);
static constexpr int MAX_VISIBLE_ITEMS = 6;

/**
 * Draws settings screen
 */
void Settings::drawScreen() {
   // Adjust so selected item is visible
   if ((selection-offset) >= MAX_VISIBLE_ITEMS) {
      offset++;
   }
   if ((selection < offset)) {
      offset--;
   }
   lcd.setInversion(false); lcd.clearFrameBuffer();
   lcd.setInversion(true);  lcd.putString("  Settings Menu\n"); lcd.setInversion(false);
   for (int item=0; item<NUM_ITEMS; item++) {
      if (item<offset) {
         continue;
      }
      if (item>(offset+MAX_VISIBLE_ITEMS)) {
         continue;
      }
      lcd.setInversion(item == selection);
      lcd.gotoXY(0, (item+1-offset)*lcd.FONT_HEIGHT);
      lcd.printf(menu[item]->getDescription(), menu[item]->get());
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
      case SW_F3F4:
         menu[selection]->reset();
         changed = true;
         break;
      case SW_F3:
         menu[selection]->increment();
         if (!buttons.isRepeating()) {
            menu[selection]->action();
         }
         changed = true;
         break;
      case SW_F4:
         menu[selection]->decrement();
         if (!buttons.isRepeating()) {
            menu[selection]->action();
         }
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
