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


constexpr uint16_t AMBIENT_TEMP = 0;  // Alias for starting temperature

/**
 * Used to represent a solder profile in ROM
 */
class SolderProfile {
public:
   /** Point in solder profile */
   struct Point {
      uint16_t time;          // Seconds
      uint16_t temperature:9; // Degrees Celsius
      uint16_t fanSpeed:2;    // Fan speed
      uint16_t stop:1;        // End of sequence
   };

   /** Description of the profile */
   char    description[40];

   /** Profile steps */
   Point profile[10];
};

/**
 * Used to represent a solder profile in nonvolatile memory
 */
class NvSolderProfile {
public:
   /** Description of the profile */
   USBDM::NonvolatileArray<char, 40> description;

   /** Profile steps */
   USBDM::NonvolatileArray<SolderProfile::Point, 10> profile;

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
      for (unsigned index=0; index<(sizeof(NvSolderProfile::profile)/sizeof(NvSolderProfile::profile[0])); index++) {
         printf("{%3d, %3d}", profile[index].time, profile[index].time);
      }
      printf("}\n");
   }
};

// Predefined profiles
extern const SolderProfile am4300profile;
//extern const SolderProfile nc31profile;
//extern const SolderProfile syntechlfprofile;
#ifdef DEBUG_BUILD
extern const SolderProfile short_testprofile;
extern const SolderProfile rampspeed_testprofile;
extern const SolderProfile pidcontrol_testprofile;
#endif

/** Maximum number of profiles support in NV memory */
constexpr unsigned MAX_PROFILES = 10;

extern NvSolderProfile profiles[MAX_PROFILES];

#endif /* SOURCES_PROFILES_H_ */
