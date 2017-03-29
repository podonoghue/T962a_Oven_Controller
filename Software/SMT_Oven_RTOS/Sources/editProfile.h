/**
 * @file    editProfile.h
 * @brief   Profile Editor
 *
 *  Created on: 8Oct.,2016
 *      Author: podonoghue
 */

#ifndef SOURCES_EDITPROFILE_H_
#define SOURCES_EDITPROFILE_H_

#include <solderProfiles.h>

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
   ProfileSetting_T(
         T &value,
         const char *description,
         T delta,
         T defaultValue,
         T min,
         T max) :
            value(value), description(description), delta(delta), defaultValue(defaultValue), min(min), max(max) {
      set((T)value);
   }
   virtual const char *getDescription() const {
      snprintf(buff, sizeof(buff), description, (T)value);
      return buff;
   }
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
   virtual bool increment() {
      return set(value + delta);
   }
   virtual bool decrement() {
      return set(value - delta);
   }
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
      strncpy(buff, nameBuffer, STRING_LENGTH);

      // Trim trailing spaces etc.
      int index = STRING_LENGTH;
      while ((buff[index] == '\0')||(buff[index] == ' ')) {
         buff[index] = '\0';
         index--;
      }
      buff[STRING_LENGTH] = '\0';
      return buff;
   }
   virtual bool increment() {
      bool changed = edit();
      if (changed) {
         strcpy(name, getDescription());
      }
      return changed;
   }
   virtual bool decrement() {
      return increment();
   }
   virtual bool reset() {
      strncpy(nameBuffer, name, sizeof(nameBuffer));
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
   static constexpr int NUM_ITEMS = 9;

   /** Describes the editable items */
   ProfileSetting *items[NUM_ITEMS] = {
         //                             value,                 description                  delta default minimum maximum
         new ProfileNameSetting        (profile.description),
         new ProfileSetting_T<float>   (profile.ramp1Slope,    "Ramp 1 up    %3.1f\177C/s", 0.1f,   1.0f,  0.1f,   6.0f),
         new ProfileSetting_T<uint16_t>(profile.soakTemp1,     "Soak temp. 1 %3d\177C",        1,    140,    80,    160),
         new ProfileSetting_T<uint16_t>(profile.soakTemp2,     "Soak temp. 2 %3d\177C",        1,    183,   150,    250),
         new ProfileSetting_T<uint16_t>(profile.soakTime,      "Soak time    %3ds",            1,    120,    60,    300),
         new ProfileSetting_T<float>   (profile.ramp2Slope,    "Ramp 2 up    %3.1f\177C/s",  0.1f,  3.0f,  0.1f,   6.0f),
         new ProfileSetting_T<uint16_t>(profile.peakTemp,      "Peak temp.   %3d\177C",        1,    210,   180,    300),
         new ProfileSetting_T<uint16_t>(profile.peakDwell,     "Peak dwell   %3ds",            1,     20,     1,     30),
         new ProfileSetting_T<float>   (profile.rampDownSlope, "Ramp down    %3.1f\177C/s", 0.1f,  -3.0f, -6.0f,  -0.1f),
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
