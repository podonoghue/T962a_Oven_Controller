/*
 * profile.h
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_PROFILES_H_
#define SOURCES_PROFILES_H_

#include <stdio.h>
#include <stdint.h>
#include <flash.h>

class SolderProfile {
public:
   /** Number of seconds each step in the sequence represents */
   static constexpr int SECONDS_PER_STEP = 10;

   /** Description of the profile */
   char    description[40];

   /** Profile steps */
   uint8_t profile[48];
};

class NvSolderProfile {
public:
   /** Number of seconds each step in the sequence represents */
   static constexpr int SECONDS_PER_STEP = SolderProfile::SECONDS_PER_STEP;

   /** Description of the profile */
   USBDM::NonvolatileArray<char,    40> description;

   /** Profile steps */
   USBDM::NonvolatileArray<uint8_t, 48> profile;

   /**
    * Assignment from SolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    */
   void operator=(const SolderProfile &other ) {
      description = other.description;
      profile     = other.profile;
   }

   /**
    * Assignment from NvSolderProfile
    *
    * This adds a wait for the Flash to be updated after each element is assigned
    */
   void operator=(const NvSolderProfile &other ) {
      description = other.description;
      profile     = other.profile;
   }

   /**
    * Clear profile i.e. mark as empty
    * Empty is indicated by a zero-length description string
    */
   void reset() {
      description.set(0, '\0');
   }

   /**
    * Prints the profile to stdout
    */
   void print() {
      printf("%s = {\n", (const char *)description);
      for (unsigned time=0; time<(sizeof(SolderProfile::profile)/sizeof(SolderProfile::profile[0])); time++) {
         if ((time%10) == 0) {
            printf("%4d:", time*10);
         }
         printf("%3d, ", profile[time]);
         if ((time%10) == 9) {
            printf("\n");
         }
      }
      printf("}\n");
   }
};

// Predefined profiles
extern const SolderProfile am4300profile;
extern const SolderProfile nc31profile;
extern const SolderProfile syntechlfprofile;
#ifdef DEBUG_BUILD
extern const SolderProfile short_testprofile;
extern const SolderProfile rampspeed_testprofile;
extern const SolderProfile pidcontrol_testprofile;
#endif

/** Maximum number of profiles support in NV memory */
constexpr unsigned MAX_PROFILES = 10;

extern NvSolderProfile profiles[MAX_PROFILES];

#endif /* SOURCES_PROFILES_H_ */
