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
Settings::Settings() : Flash() {

   // Initialise EEPROM
   USBDM::FlashDriverError_t rc = initialiseEeprom(USBDM::Flash::eeprom2KBytes);
   if (rc == USBDM::FLASH_ERR_OK) {
      return;
   }
   // Errors are ignored here but will have already set the USBDM error code
   initialiseSettings();
}

static constexpr int MIN_FAN_SPEED   = 10;
static constexpr int FAN_KICK_CYCLES = 10;
static constexpr int BEEP_TIME       = 1;

void Settings::initialiseSettings() {

   // Write initial value for non-volatile variables
   int i=0;
   profiles[i++] = am4300profile;
   profiles[i++] = nc31profile;
   profiles[i++] = syntechlfprofile;
#ifdef DEBUG_BUILD
   profiles[i++] = rampspeed_testprofile;
   profiles[i++] = pidcontrol_testprofile;
#endif
   profiles[i++].reset();

   minimumFanSpeed = MIN_FAN_SPEED;
   fanKickTime     = FAN_KICK_CYCLES;
   t1Offset        = 0;
   t2Offset        = 0;
   t3Offset        = 0;
   t4Offset        = 0;
   beepTime        = BEEP_TIME;

   profileIndex    = 0;
}

/**
 * Test Beeper
 */
void Settings::testBeep(const Setting *setting) {
   (void)setting;
   char buff[lcd.LCD_WIDTH/lcd.FONT_WIDTH+2];
   snprintf(buff, sizeof(buff), "Duration = %ds", (int)beepTime);
//   testingScreen("Test Beep", buff);
   Buzzer::play();
}

extern const Setting &fanSetting;
extern const Setting &kickSetting;

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
 * Describes the menu
 */
static const Setting menu[] = {
      {minimumFanSpeed, "Reflow fan speed %3d%%",     5, 100, 5,   MIN_FAN_SPEED,   FanTest::testFan},
      {fanKickTime,     "Fan Kick Cycles  %3d",       0,  50, 1,   FAN_KICK_CYCLES, FanTest::testFan},
      {t1Offset,        "Thermo 1 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {t2Offset,        "Thermo 2 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {t3Offset,        "Thermo 3 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {t4Offset,        "Thermo 4 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {beepTime,        "Beep time        %3ds",      0,  30, 1,   BEEP_TIME,       Settings::testBeep},
};

const Setting &fanSetting  = menu[0];
const Setting &kickSetting = menu[1];

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
   lcd.setInversion(true);  lcd.putString("Settings Menu");

   lcd.setInversion(false);
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
         menu[selection].reset();
         changed = true;
         break;
      case SW_F3:
         menu[selection].increment();
         if (!buttons.isRepeating()) {
            menu[selection].action();
         }
         changed = true;
         break;
      case SW_F4:
         menu[selection].decrement();
         if (!buttons.isRepeating()) {
            menu[selection].action();
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
