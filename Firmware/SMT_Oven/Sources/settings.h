/*
 * Settings.h
 *
 *  Created on: 25 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SETTINGS_H_
#define SOURCES_SETTINGS_H_

#include "flash.h"
#include "profiles.h"

/** Length of beep in seconds */
extern USBDM::Nonvolatile<int> beepTime;

/** Minimum fan speed to used when oven is heating (%) */
extern USBDM::Nonvolatile<int> minimumFanSpeed;

/** Time to operate the fan at full speed before PWM starts (mains half-cycles) */
extern USBDM::Nonvolatile<int> fanKickTime;

/** Offset added to thermocouple #1 */
extern USBDM::Nonvolatile<int> t1Offset;
/** Offset added to thermocouple #2 */
extern USBDM::Nonvolatile<int> t2Offset;
/** Offset added to thermocouple #3 */
extern USBDM::Nonvolatile<int> t3Offset;
/** Offset added to thermocouple #4 */
extern USBDM::Nonvolatile<int> t4Offset;

/** Index of current profile */
extern USBDM::Nonvolatile<int> profileIndex;

/**
 * This class handles initialising all non-volatile storage
 */
class Settings : public USBDM::Flash {

private:
   int selection     = 0;
   int lastSelection = 0;
   int offset        = 0;

   void drawScreen();

public:
   /**
    * Constructor - initialises the non-volatile storage\n
    * Must be a singleton!
    */
   Settings();

   /**
    * Display menu to modify settings
    */
   void runMenu();

   /*
    * Initialise settings to default values
    */
   void initialiseSettings();

};

extern Settings settings;

#endif /* SOURCES_SETTINGS_H_ */
