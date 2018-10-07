/**
 * @file Settings.cpp
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

/** Priority of the FlexRAM initialisation (Settings constructor) */
#define FLEX_RAM_INIT_PRIORITY  (1000)

using namespace USBDM;

/*
 * Allocate settings variables to non-volatile storage
 *
 * Must be allocated to flash.
 * These objects must be initialise by the Settings object
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
Nonvolatile<int> currentProfileIndex;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> fanKickTime;

__attribute__ ((section(".flexRAM")))
Nonvolatile<int> maxHeaterTime;

__attribute__ ((section(".flexRAM")))
USBDM::Nonvolatile<float> pidKp;

__attribute__ ((section(".flexRAM")))
USBDM::Nonvolatile<float> pidKi;

__attribute__ ((section(".flexRAM")))
USBDM::Nonvolatile<float> pidKd;

extern const Setting_T<int> fanSetting;
extern const Setting_T<int> kickSetting;
extern const Setting_T<int> heaterSetting;
extern const Setting_T<int> thermo1Setting;
extern const Setting_T<int> thermo2Setting;
extern const Setting_T<int> thermo3Setting;
extern const Setting_T<int> thermo4Setting;
extern const Setting_T<int> beepSetting;

extern const Setting_T<float> pidKpSetting;
extern const Setting_T<float> pidKiSetting;
extern const Setting_T<float> pidKdSetting;

/**
 * Constructor - initialises the non-volatile storage\n
 * Must be a singleton!
 */
Settings::Settings() : Flash() {
   // Initialise EEPROM
   USBDM::FlashDriverError_t rc = initialiseEeprom();
   if (rc == USBDM::FLASH_ERR_OK) {
      return;
   }
   /*
    * Errors are ignored here but will have already set the USBDM error code.
    * These may be tested later in main()
    */
   initialiseSettings();
}

/**
 * Initialises the non-volatile storage to factory defaults
 */
void Settings::initialiseSettings() {

   // Write initial value for non-volatile variables
   unsigned i=0;
   profiles[i++] = am4300profileA;
   profiles[i++] = am4300profileB;
   profiles[i++] = nc31profile;
   profiles[i++] = syntechlfprofile;
   for (;i<(sizeof(profiles)/sizeof(profiles[0]));i++) {
      profiles[i] = defaultProfile;
   }
   minimumFanSpeed = fanSetting.getDefaultValue();
   fanKickTime     = kickSetting.getDefaultValue();
   t1Offset        = thermo1Setting.getDefaultValue();
   t2Offset        = thermo2Setting.getDefaultValue();
   t3Offset        = thermo3Setting.getDefaultValue();
   t4Offset        = thermo4Setting.getDefaultValue();
   t1Enable        = true;
   t2Enable        = true;
   t3Enable        = true;
   t4Enable        = true;
   beepTime        = beepSetting.getDefaultValue();
   maxHeaterTime   = heaterSetting.getDefaultValue();

   /**
    * PID controller parameters
    */
   pidKp           = pidKpSetting.getDefaultValue(); //20.0;   //4.0f; // 20.0
   pidKi           = pidKiSetting.getDefaultValue(); //0.016;  //0.0f; //  0.016
   pidKd           = pidKdSetting.getDefaultValue(); //62.5;   //0.0f; // 62.5

   currentProfileIndex    = 0;
}

/**
 * Test Beeper
 */
void Settings::testBeep(const Setting *setting) {
   (void)setting;
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
      bool kickMode = (setting == &kickSetting);
      bool changed = true;
      for(;;) {
         bool motorOn = ovenControl.getFanDutycycle()>0;
         if (changed) {
            testFanScreen(kickMode);
         }
         switch(buttons.getButton()) {
         case SwitchValue::SW_F1:
            if (motorOn) {
               ovenControl.setFanDutycycle(0);
            }
            else {
               ovenControl.setFanDutycycle(minimumFanSpeed);
            }
            changed = true;
            break;
         case SwitchValue::SW_F2:
            kickMode = !kickMode;
            changed = true;
            break;
         case SwitchValue::SW_F3F4:
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
         case SwitchValue::SW_F3:
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
         case SwitchValue::SW_F4:
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
         case SwitchValue::SW_S:
            ovenControl.setFanDutycycle(0);
            return;
         default:
            break;
         }
         __WFI();
      }
   }
};

/**
 * Describes the various settings for the menu
 * Controls range and default values etc.
 * Also used by the Settings object to initialise FlexRAM objects
 */
//                                      nvVariable        description                min   max  incr  default  test function
const Setting_T<int> fanSetting      {minimumFanSpeed, "Reflow fan speed %3d%%",     5,  100,  5,   30,      FanTest::testFan};
const Setting_T<int> kickSetting     {fanKickTime,     "Fan Kick Cycles  %3d",       0,   50,  1,   10,      FanTest::testFan};
const Setting_T<int> thermo1Setting  {t1Offset,        "Thermo 1 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting_T<int> thermo2Setting  {t2Offset,        "Thermo 2 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting_T<int> thermo3Setting  {t3Offset,        "Thermo 3 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting_T<int> thermo4Setting  {t4Offset,        "Thermo 4 Offset  %3d\x7F", -30,   30,  1,   0,       nullptr};
const Setting_T<int> heaterSetting   {maxHeaterTime,   "Max heater time %4d",       10, 1000, 10, 600,       nullptr};
const Setting_T<int> beepSetting     {beepTime,        "Beep time        %3ds",      0,   30,  1,   0,       Settings::testBeep};

const Setting_T<float> pidKpSetting  {pidKp,           "PID Kp      %6.1f",        0.5,  60.00,  0.1,  40.0f,   nullptr};
const Setting_T<float> pidKiSetting  {pidKi,           "PID Ki        %6.3f",      0.0,   1.00,  0.001, 0.050f, nullptr};
const Setting_T<float> pidKdSetting  {pidKd,           "PID Kd      %6.1f",        0.0, 200.00,  0.1,  62.5f,   nullptr};

/**
 * Describes the settings and limits for same
 */
const Setting *const menu[] {
      &fanSetting,
      &kickSetting,
      &thermo1Setting,
      &thermo2Setting,
      &thermo3Setting,
      &thermo4Setting,
      &heaterSetting,
      &beepSetting,
      &pidKpSetting,
      &pidKiSetting,
      &pidKdSetting,
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
      lcd.putString(menu[item]->getDescription());
   }
   lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true);  lcd.putString(" ");      lcd.putUpArrow();   lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(5);
   lcd.setInversion(true);  lcd.putString(" ");      lcd.putDownArrow(); lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(5);
   lcd.setInversion(true);  lcd.putString(" + ");    lcd.setInversion(false);            lcd.putSpace(5);
   lcd.setInversion(true);  lcd.putString(" - ");    lcd.setInversion(false);            lcd.putSpace(5);
   lcd.setInversion(true);  lcd.putString(" Exit "); lcd.setInversion(false);

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
      SwitchValue button = buttons.getButton();
      switch(button) {
      case SwitchValue::SW_F1:
         if (selection>0) {
            selection--;
            changed = true;
         }
         break;
      case SwitchValue::SW_F2:
         if (selection<(NUM_ITEMS-1)) {
            selection++;
            changed = true;
         }
         break;
      case SwitchValue::SW_F3F4:
         menu[selection]->reset();
         changed = true;
         break;
      case SwitchValue::SW_F3:
         menu[selection]->increment();
         if (!button.isRepeating()) {
            menu[selection]->action();
         }
         changed = true;
         break;
      case SwitchValue::SW_F4:
         menu[selection]->decrement();
         if (!button.isRepeating()) {
            menu[selection]->action();
         }
         changed = true;
         break;
      case SwitchValue::SW_S:
         return;
      default:
         break;
      }
      __WFI();
   }
}

// Singleton
Settings settings __attribute__ ((init_priority (FLEX_RAM_INIT_PRIORITY)));
