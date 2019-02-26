/**
 * @file    Settings.h
 * @brief   Non-volatile Settings for Oven
 *
 *  Created on: 25 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SETTINGS_H_
#define SOURCES_SETTINGS_H_

#include "flash.h"
#include "SolderProfile.h"
#include "stringFormatter.h"

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

/** Whether thermocouple #1 is enabled */
extern USBDM::Nonvolatile<bool> t1Enable;

/** Whether thermocouple #1 is enabled */
extern USBDM::Nonvolatile<bool> t2Enable;

/** Whether thermocouple #1 is enabled */
extern USBDM::Nonvolatile<bool> t3Enable;

/** Whether thermocouple #1 is enabled */
extern USBDM::Nonvolatile<bool> t4Enable;

/** Index of current profile */
extern USBDM::Nonvolatile<int> currentProfileIndex;

/** Maximum on time for heater in manual mode */
extern USBDM::Nonvolatile<int> maxHeaterTime;

/** PID controller parameter - proportional */
extern USBDM::Nonvolatile<float> pidKp;

/** PID controller parameters - integral */
extern USBDM::Nonvolatile<float> pidKi;

/** PID controller parameters - differential */
extern USBDM::Nonvolatile<float> pidKd;

class Setting {

protected:
   static constexpr int BUF_SIZE = 30;

   /**
    * Gets pointer to static buffer
    *
    * @return Buffer pointer
    */
   static char *getBuff() {
      static char buff[BUF_SIZE];
      return buff;
   }

protected:
   ~Setting() = default;

public:
   constexpr Setting() {
   }

   /**
    * Get description of variable including value
    *
    * @return Description
    */
   virtual const char* getDescription() const = 0;

   /**
    * Increment variable
    *
    * @note limits are applied
    */
   virtual void increment() const = 0;

   /**
    * Increment variable
    *
    * @note limits are applied
    */
   virtual void decrement() const  = 0;

   /**
    * Reset variable to default value
    */
   virtual void reset() const  = 0;

   /**
    * Carry out action associated with variable\n
    * e.g. Sound beeper
    */
   virtual void action() const  = 0;
};

/**
 * Class to hold information about how to modify a non-volatile setting
 */
template<typename T>
class Setting_T : public Setting {

protected:

   /** Associated non-volatile variable */
   USBDM::Nonvolatile<T> &nvVariable;

   /** Description of variable */
   const char *description;

   /** Minimum allowed value */
   const T min;

   /** Maximum allowed value */
   const T max;

   /** Increment/decrement to use when changing value */
   const T delta;

   /** Default value used when resetting variable */
   const T defaultValue;

   /** Unit for variable */
   const char *unit;

   /** Function called to test function associated with variable */
   void (*func)(const Setting *setting);

public:

   virtual ~Setting_T() = default;

   /**
    * Constructor
    *
    * @param[in] nvVariable   Non-volatile variable to manipulate
    * @param[in] desc         Description of setting
    * @param[in] min          Minimum value
    * @param[in] max          Maximum value
    * @param[in] delta        Change size for +/-
    * @param[in] defaultValue Default value for restore default
    * @param[in] unit         Unit for setting
    * @param[in] func         Action function
    */
   constexpr Setting_T(USBDM::Nonvolatile<T> &nvVariable, const char *desc, T min, T max, T delta, T defaultValue, const char *unit, void (*func)(const Setting *setting) ) :
      nvVariable(nvVariable), description(desc), min(min), max(max), delta(delta), defaultValue(defaultValue), unit(unit), func(func)
   {}

   /**
    * Get description of variable including value\n
    *
    * @return Description
    *
    * @note This uses an internal static buffer that is shared by all Settings objects
    */
   virtual const char* getDescription() const {
      USBDM::StringFormatter sf(getBuff(), BUF_SIZE);
      sf.write(description).write((T)nvVariable).write(unit);
      return sf.toString();
   }

   /**
    * Set value of variable
    *
    * @param[in] value Value to set
    *
    * @note limits are applied
    */
   void set(T value) const {
      if (value < min) {
         value = min;
      }
      if (value > max) {
         value = max;
      }
      if (this->nvVariable != value) {
         // Only update if actually changed (to avoid unnecessary FlexRAM update)
         this->nvVariable = value;
      }
   }

   /**
    * Increment variable
    *
    * @note limits are applied
    */
   void increment() const override {
      set(nvVariable + delta);
   }

   /**
    * Increment variable
    *
    * @note limits are applied
    */
   void decrement() const override {
      set(nvVariable - delta);
   }

   /**
    * Reset variable to default value
    */
   void reset() const override {
      set(defaultValue);
   }

   /**
    * Carry out action associated with variable (if any)\n
    * e.g. Sound beeper
    */
   void action() const override {
      if (func != nullptr) {
         func(this);
      }
   }

   /**
    * Get default value
    *
    * @return Default value
    */
   T getDefaultValue() const {
      return defaultValue;
   }
};

/**
 * This class allows editing of Oven settings
 */
class Settings : public USBDM::Flash {

private:
   int selection     = 0;
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
    *
    * @param[in] setting The current setting
    */
   static void testFan(const Setting *setting);

   /**
    * Test Beeper
    *
    * @param[in] setting The current setting
    */
   static void testBeep(const Setting *setting);

   /**
    * Display testing screen
    *
    * @param[in] title   Title string to display
    * @param[in] params  Parameter values to display
    */
   static void testingScreen(const char *title, const char *params);
};

extern Settings settings;

#endif /* SOURCES_SETTINGS_H_ */
