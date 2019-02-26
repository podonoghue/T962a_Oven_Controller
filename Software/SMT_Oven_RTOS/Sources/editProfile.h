/**
 * @file    editProfile.h
 * @brief   Profile Editor
 *
 *  Created on: 8Oct.,2016
 *      Author: podonoghue
 */

#ifndef SOURCES_EDITPROFILE_H_
#define SOURCES_EDITPROFILE_H_

#include <SolderProfile.h>
#include "stringFormatter.h"

class ProfileSetting {
public:
   /**
    * Get description of setting for menu (includes value)
    *
    * @return Descriptive string e.g. rampDown 3.2 C/s
    */
   virtual const char *getDescription() const = 0;
   /**
    * Increment value\n
    * Applies limits
    *
    * @return true if value actually changed
    */
   virtual bool increment() = 0;
   /**
    * Decrement value\n
    * Applies limits
    *
    * @return true if value actually changed
    */
   virtual bool decrement() = 0;
   /**
    * Resets value
    *
    * @return true if value actually changed
    */
   virtual bool reset() = 0;
   /**
    * Destructor
    */
   virtual ~ProfileSetting() {}
};

template<typename T>
class ProfileSetting_T : public ProfileSetting {

private:
   /** Variable value */
   T     &value;
   /** Description format string  as for printf() */
   const char * const description;
   /** Increment/decrement size */
   const char * const unit;
   /** Increment/decrement size */
   const T delta;
   /** Default value for reset */
   const T defaultValue;
   /** Minimum value */
   const T min;
   /** Maximum value */
   const T max;
   /** Maximum length of completed description string */
   static constexpr int STRING_LENGTH = 30;
   /** Buffer for description string */
   static char buff[STRING_LENGTH];

public:
   /**
    * Constructor
    */
   ProfileSetting_T(
         T &value,
         const char *description,
         const char *unit,
         T delta,
         T defaultValue,
         T min,
         T max) :
            value(value), description(description), unit(unit), delta(delta), defaultValue(defaultValue), min(min), max(max) {
      using namespace USBDM;
      set((T)value);
   }
   /**
    * Get description including value
    *
    * @return Pointer to static buffer containing description
    */
   virtual const char *getDescription() const {
      using namespace USBDM;

      StringFormatter sf(buff, STRING_LENGTH);
      sf.setFloatFormat(1, Padding_LeadingSpaces, 3);
      sf.write(description).write(value).write(unit);

      return sf.toString();
   }
   /**
    * Set value
    *
    * @param[in] newValue Value to set
    *
    * @return true if value actually changed
    */
   virtual bool set(T newValue) {
      if (newValue < min) {
         newValue = min;
      }
      if (newValue > max) {
         newValue = max;
      }
      bool changed = (newValue != value);
      value = newValue;
      return changed;
   }
   /**
    * Increment value\n
    * Applies limits
    *
    * @return true if value actually changed
    */
   virtual bool increment() {
      return set(value + delta);
   }
   /**
    * Decrement value\n
    * Applies limits
    *
    * @return true if value actually changed
    */
   virtual bool decrement() {
      return set(value - delta);
   }
   /**
    * Resets value
    *
    * @return true if value actually changed
    */
   virtual bool reset() {
      return set(defaultValue);
   }
   virtual ~ProfileSetting_T() {}
};

class ProfileNameSetting : public ProfileSetting {

private:
   /** Variable value */
   char *name;

   /** Maximum length of completed description string */
   static constexpr int STRING_LENGTH = sizeof(SolderProfile::description);

   char nameBuffer[STRING_LENGTH+1];

   unsigned editPosition;
   unsigned letterPosition;

public:
   /**
    * Constructor
    *
    * @param name Name being edited
    */
   ProfileNameSetting(char *name) : name(name) {
      reset();
   }
   void draw();
   bool edit();

   /**
    * Get description string
    *
    * @return Pointer to static buffer containing description
    */
   virtual const char *getDescription() const {
      static char buff[STRING_LENGTH+1];
      memset(buff, '\0', sizeof(buff));
      strncpy(buff, nameBuffer, STRING_LENGTH+1);

      // Trim trailing spaces etc.
      int index = STRING_LENGTH;
      while ((buff[index] == '\0')||(buff[index] == ' ')) {
         buff[index] = '\0';
         index--;
      }
      buff[STRING_LENGTH] = '\0';
      return buff;
   }
   /**
    * Increment value\n
    * Applies limits
    *
    * @return true if value actually changed
    */
   virtual bool increment() {
      bool changed = edit();
      if (changed) {
         strcpy(name, getDescription());
      }
      return changed;
   }
   /**
    * Decrement value\n
    * Applies limits
    *
    * @return true if value actually changed
    */
   virtual bool decrement() {
      return increment();
   }
   /**
    * Resets value
    *
    * @return true if value actually changed
    */
   virtual bool reset() {
      strncpy(nameBuffer, name, sizeof(nameBuffer)-1);
      nameBuffer[sizeof(nameBuffer)-1] = '\0';
      unsigned i = strlen(name);
      if (i<sizeof(nameBuffer)) {
         memset(nameBuffer+i, ' ', sizeof(nameBuffer)-i);
      }
      nameBuffer[sizeof(nameBuffer)-1] = '\0';
      editPosition = i;
      return true;
   }
   virtual ~ProfileNameSetting() {}
};

class EditProfile {

private:
   /** Temporary copy of profile being edited */
   SolderProfile &profile;

   /** Current menu selection */
   static int selection;

   /** Offset scrolled for menu items */
   static int offset;

   /** Number of editable items in menu */
   static constexpr int NUM_ITEMS = 10;

   /** Describes the editable items */
   ProfileSetting *items[NUM_ITEMS] = {
         //                             value,                 description          unit     delta  default  minimum   maximum
         new ProfileNameSetting        (profile.description),
         new ProfileSetting_T<uint16_t>(profile.liquidus,      "Liquidus temp.  ",  "\177C",     1,    183,      120,      250),
         new ProfileSetting_T<uint16_t>(profile.preheatTime,   "Preheat Time    ",  "s",         1,     90,       60,      200),
         new ProfileSetting_T<uint16_t>(profile.soakTemp1,     "Soak temp. 1    ",  "\177C",     1,    140,       80,      160),
         new ProfileSetting_T<uint16_t>(profile.soakTemp2,     "Soak temp. 2    ",  "\177C",     1,    183,      150,      250),
         new ProfileSetting_T<uint16_t>(profile.soakTime,      "Soak time       ",  "s",         1,    120,       60,      300),
         new ProfileSetting_T<float>   (profile.rampUpSlope,   "Ramp up    ",       "\177C/s",   0.1f,   3.0f,     0.1f,     6.0f),
         new ProfileSetting_T<uint16_t>(profile.peakTemp,      "Peak temp.      ",  "\177C",     1,    210,      180,      300),
         new ProfileSetting_T<uint16_t>(profile.peakDwell,     "Peak dwell      ",  "s",         1,     20,        1,       30),
         new ProfileSetting_T<float>   (profile.rampDownSlope, "Ramp down  ",       "\177C/s",   0.1f,  -3.0f,    -6.0f,    -0.1f),
   };

   /**
    * Draw screen
    */
   void drawScreen();

   /**
    * Run Edit profile menu
    *
    * @return true => Profile edited and needs update in NV memory
    */
   bool doit();

   /**
    * Constructs an EditProfile for the given profile
    */
   EditProfile(SolderProfile &profile) : profile(profile) {
   }

public:
   /**
    * Allows editing of a Solder profile.\n
    * After editing the user is prompted to save the profile.
    *
    * @param nvProfile The profile to edit
    */
   static void run(NvSolderProfile &nvProfile);
};

#endif /* SOURCES_EDITPROFILE_H_ */
