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
 * Class to hold information about how to modify a non-volatile setting
 */
class Setting {
public:
   /** Associated non-volatile variable */
   Nonvolatile<int> &nvVariable;

   /** Description of variable */
   const char *description;

   /** Minimum allowed value */
   const int min;

   /** Maximum allowed value */
   const int max;

   /** Increment to use when changing value */
   const int incr;

   /** Default value used when resetting variable */
   const int defaultValue;

   /** Function called to test function associated with variable */
   void (*func)();

public:

   /*
    * Get variable value
    *
    * @return Value as integer
    */
   int get() const {
      return nvVariable;
   }

   /**
    * Set value of variable
    *
    * @param value Value to set
    *
    * @note limits are applied
    */
   int set(int value) const {
      if (value < min) {
         value = min;
      }
      if (value > max) {
         value = max;
      }
      if (this->nvVariable != value) {
         // Only update if actually changed (to avoid unnecessary EEPROM update)
         this->nvVariable = value;
      }
      return nvVariable;
   }

   /**
    * Increment variable
    *
    * @note limits are applied
    */
   int increment() const {
      return set(nvVariable + incr);
   }

   /**
    * Increment variable
    *
    * @note limits are applied
    */
   int decrement() const {
      return set(nvVariable - incr);
   }

   /**
    * Reset variable to default value
    */
   int reset() const {
      return set(defaultValue);
   }

   /**
    * Get description of variable
    *
    * @return Description
    */
   const char* getDescription() const {
      return description;
   }

   /**
    * Carry out action associated with variable\n
    * e.g. Sound beeper
    */
   void action() const {
      if (func != nullptr) {
         func();
      }
   }
};

/**
 * Display testing screen
 *
 * @param title   Title string to display
 * @param params  Parameter values to display
 */
void Settings::testingScreen(const char *title, const char *params) {
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

/**
 * Test Beeper
 */
void Settings::testBeep() {
   char buff[lcd.LCD_WIDTH/lcd.FONT_WIDTH+2];
   snprintf(buff, sizeof(buff), "Duration = %ds", (int)beepTime);
   testingScreen("Test Beep", buff);
   Buzzer::high();
   static const auto abort = []() { return buttons.getButton() != SW_NONE; };
   USBDM::wait((float)beepTime, abort);
   Buzzer::low();
}

/**
 * Test Fan operation
 */
void Settings::testFan() {
   char buff[lcd.LCD_WIDTH/lcd.FONT_WIDTH+2];
   snprintf(buff, sizeof(buff), "kick=%dcy speed=%d%%", (int)fanKickTime, (int)minimumFanSpeed);
   testingScreen("Fan Test", buff);
   ovenControl.setFanDutycycle(minimumFanSpeed);
   static const auto abort = []() { return buttons.getButton() != SW_NONE; };
   USBDM::wait(100, abort);
   ovenControl.setFanDutycycle(0);
}
/**
 * Describes the menu
 */
static const Setting menu[] = {
      {minimumFanSpeed, "Reflow fan speed %3d%%",     5, 100, 5,   MIN_FAN_SPEED,   Settings::testFan},
      {fanKickTime,     "Fan Kick Cycles  %3d",       0,  50, 1,   FAN_KICK_CYCLES, Settings::testFan},
      {t1Offset,        "Thermo 1 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {t2Offset,        "Thermo 2 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {t3Offset,        "Thermo 3 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {t4Offset,        "Thermo 4 Offset  %3d\x7F", -30,  30, 1,   0,               nullptr},
      {beepTime,        "Beep time        %3ds",      0,  30, 1,   BEEP_TIME,       Settings::testBeep},
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
         if (!buttons.isRepeating()) {
            menu[selection].action();
         }
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
