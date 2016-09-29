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
 * Class to hold information about how to modify a non-volatile setting
 */
class Setting {
public:
   /** Associated non-volatile variable */
   USBDM::Nonvolatile<int> &nvVariable;

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
   void (*func)(const Setting *setting);

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
         func(this);
      }
   }
};

/**
 * This class handles initialising all non-volatile storage
 */
class Settings : public USBDM::Flash {

private:
   int selection     = 0;
   int lastSelection = 0;
   int offset        = 0;

   /**
    * Draws settings screen
    */
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
   static void initialiseSettings();

   /**
    * Test Fan operation
    */
   static void testFan(const Setting *setting);

   /**
    * Test Beeper
    */
   static void testBeep(const Setting *setting);

   /**
    * Display testing screen
    *
    * @param title   Title string to display
    * @param params  Parameter values to display
    */
   static void testingScreen(const char *title, const char *params);
};

extern Settings settings;

#endif /* SOURCES_SETTINGS_H_ */
