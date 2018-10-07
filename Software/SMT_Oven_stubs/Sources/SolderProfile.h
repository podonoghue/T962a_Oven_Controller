/**
 * @file    SolderProfile.h
 * @brief   Solder profiles
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_SOLDERPROFILE_H_
#define SOURCES_SOLDERPROFILE_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "flash.h"

enum {
   P_UNLOCKED = (1<<0),
   P_LEADFREE = (1<<1),
};

class NvSolderProfile;

/**
 * Used to represent a solder profile in ROM
 */
struct SolderProfile {
public:
   uint8_t  flags;            // Properties of the profile
   char     description[39];  // Description of the profile
   uint16_t liquidus;         // Liquidus temperature
   uint16_t preheatTime;      // Time to reach soakTemp1
   uint16_t soakTemp1;        // Temperature for start of soak
   uint16_t soakTemp2;        // Temperature for end of soak
   uint16_t soakTime;         // Length of soak
   float    rampUpSlope;      // Slope up to peakTemp
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
    * @param[in] other Profile to copy from
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
   NvSolderProfile (const NvSolderProfile &obj) = delete;

public:
   USBDM::Nonvolatile<float>         rampUpSlope;    // Slope up to peakTemp
   USBDM::Nonvolatile<float>         rampDownSlope;  // Slope down after peakTemp
   USBDM::Nonvolatile<uint16_t>      liquidus;       // Liquidus temperature
   USBDM::Nonvolatile<uint16_t>      preheatTime;    // Time to reach soakTemp1
   USBDM::Nonvolatile<uint16_t>      soakTemp1;      // Temperature for start of soak
   USBDM::Nonvolatile<uint16_t>      soakTemp2;      // Temperature for end of soak
   USBDM::Nonvolatile<uint16_t>      soakTime;       // Length of soak
   USBDM::Nonvolatile<uint16_t>      peakTemp;       // Peak reflow temperature
   USBDM::Nonvolatile<uint16_t>      peakDwell;      // How long to remain at peakTemp
   USBDM::Nonvolatile<uint8_t>       flags;          // Properties of the profile
   USBDM::NonvolatileArray<char, sizeof(SolderProfile::description)>
                                     description;    // Description of the profile
   USBDM::Nonvolatile<uint16_t>      reserved;       // Pad to 4-bye alignment

   NvSolderProfile () {
   }

   /**
    * Assignment from SolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    *
    * @param[in] other Profile to copy from
    */
   void operator=(const SolderProfile &other );

   /**
    * Assignment from NvSolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    *
    * @param[in] other Profile to copy from
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

/** The actual profiles in nonvolatile memory */
extern NvSolderProfile profiles[MAX_PROFILES];

#endif /* SOURCES_SOLDERPROFILE_H_ */
