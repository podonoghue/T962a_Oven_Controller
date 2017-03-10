/**
 * @file    solderProfiles.h
 * @brief   Solder profiles
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SOLDERPROFILES_H_
#define SOURCES_SOLDERPROFILES_H_

#include <stdio.h>
#include <stdint.h>
#include <flash.h>
#include <string.h>

enum State {
   s_off,
   s_fail,
   s_preheat,
   s_soak,
   s_ramp_up,
   s_dwell,
   s_ramp_down,
   s_complete,
   s_manual,
};

enum {
   P_UNLOCKED = (1<<0),
};

class NvSolderProfile;

/**
 * Used to represent a solder profile in ROM
 */
struct SolderProfile {
public:
   uint8_t  flags;            // Various flags
   char     description[39];  // Description of the profile
   float    ramp1Slope;       // Slope up to soakTemp1
   uint16_t soakTemp1;        // Temperature for start of soak
   uint16_t soakTemp2;        // Temperature for end of soak
   uint16_t soakTime;         // Length of soak
   float    ramp2Slope;       // Slope up to peakTemp
   uint16_t peakTemp;         // Peak reflow temperature
   uint16_t peakDwell;        // How long to remain at peakTemp
   float    rampDownSlope;    // Slope down after peakTemp

   /**
    * Assignment from SolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    *
    * @param other Profile to copy from
    */
   void operator=(const SolderProfile &other );

   /**
    * Assignment from NvSolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    *
    * @param other Profile to copy from
    */
   void operator=(const NvSolderProfile &other );

   bool isValid() {
      if (this->soakTemp2<this->soakTemp1) {
         return false;
      }
      if (this->peakTemp<this->soakTemp2) {
         return false;
      }
      if (this->peakTemp<this->soakTemp2) {
         return false;
      }
      return true;
   }
};

/**
 * Used to represent a solder profile in nonvolatile memory
 */
class NvSolderProfile {

private:
   // Can't create these
   NvSolderProfile (const NvSolderProfile &obj) {
      (void) obj;
   }

public:
   USBDM::Nonvolatile<uint8_t>       flags;          // Description of the profile
   USBDM::NonvolatileArray<char, sizeof(SolderProfile::description)>
                                     description;    // Description of the profile
   USBDM::Nonvolatile<float>         ramp1Slope;     // Slope up to soakTemp1
   USBDM::Nonvolatile<uint16_t>      soakTemp1;      // Temperature for start of soak
   USBDM::Nonvolatile<uint16_t>      soakTemp2;      // Temperature for end of soak
   USBDM::Nonvolatile<uint16_t>      soakTime;       // Length of soak
   USBDM::Nonvolatile<float>         ramp2Slope;     // Slope up to peakTemp
   USBDM::Nonvolatile<uint16_t>      peakTemp;       // Peak reflow temperature
   USBDM::Nonvolatile<uint16_t>      peakDwell;      // How long to remain at peakTemp
   USBDM::Nonvolatile<float>         rampDownSlope;  // Slope down after peakTemp

   NvSolderProfile () {
   }

   /**
    * Assignment from SolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    *
    * @param other Profile to copy from
    */
   void operator=(const SolderProfile &other );

   /**
    * Assignment from NvSolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    *
    * @param other Profile to copy from
    */
   void operator=(const NvSolderProfile &other ) ;

   /**
    * Clear profile i.e. mark as empty
    * Empty is indicated by a zero-length description string
    */
   void reset();
   /**
    * Prints the profile to stdout
    */
   void print() const;
};

// Predefined profiles
extern const SolderProfile am4300profileA;
extern const SolderProfile am4300profileB;
extern const SolderProfile nc31profile;
extern const SolderProfile syntechlfprofile;
extern const SolderProfile defaultProfile;

/** Maximum number of profiles supported in NV memory */
constexpr unsigned MAX_PROFILES = 10;

/** The actual profile in nonvolatile memory */
extern NvSolderProfile profiles[MAX_PROFILES];

#endif /* SOURCES_SOLDERPROFILES_H_ */
