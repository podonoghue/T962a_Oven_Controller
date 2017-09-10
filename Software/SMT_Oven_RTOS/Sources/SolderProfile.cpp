/*
 * @file    solderProfiles.cpp
 * @brief   Solder profiles
 *
 * The following profiles are from:
 *
 * https://github.com/UnifiedEngineering/T-962-improvements
 *
 */
#include <SolderProfile.h>

/**
 * Assignment from NvSolderProfile
 *
 * @param[in] other Profile to copy from
 */
void SolderProfile::operator=(const NvSolderProfile &other ) {
   for (unsigned i=0; i<sizeof(description); i++) {
      description[i]   = other.description[i];
   }
   flags         = other.flags;
   liquidus      = other.liquidus;
   preheatTime   = other.preheatTime;
   soakTemp1     = other.soakTemp1;
   soakTemp2     = other.soakTemp2;
   soakTime      = other.soakTime;
   rampUpSlope   = other.rampUpSlope;
   peakTemp      = other.peakTemp;
   peakDwell     = other.peakDwell;
   rampDownSlope = other.rampDownSlope;
}

/**
 * Assignment from SolderProfile
 *
 * @param[in] other Profile to copy from
 */
void SolderProfile::operator=(const SolderProfile &other ) {
   for (unsigned i=0; i<sizeof(description); i++) {
      description[i]   = other.description[i];
   }
   flags         = other.flags;
   preheatTime   = other.preheatTime;
   soakTemp1     = other.soakTemp1;
   soakTemp2     = other.soakTemp2;
   soakTime      = other.soakTime;
   rampUpSlope   = other.rampUpSlope;
   peakTemp      = other.peakTemp;
   peakDwell     = other.peakDwell;
   rampDownSlope = other.rampDownSlope;
}

/**
 * Assignment from SolderProfile
 *
 * This adds a wait for the Flash to be updated after each element is assigned
 *
 * @param other Profile to copy from
 */
void NvSolderProfile::operator=(const SolderProfile &other ) {
   flags         = other.flags;
   description   = other.description;
   liquidus      = other.liquidus;
   preheatTime   = other.preheatTime;
   soakTemp1     = other.soakTemp1;
   soakTemp2     = other.soakTemp2;
   soakTime      = other.soakTime;
   rampUpSlope   = other.rampUpSlope;
   peakTemp      = other.peakTemp;
   peakDwell     = other.peakDwell;
   rampDownSlope = other.rampDownSlope;
}

/**
 * Assignment from NvSolderProfile
 *
 * This adds a wait for the Flash to be updated after each element is assigned
 *
 * @param other Profile to copy from
 */
void NvSolderProfile::operator=(const NvSolderProfile &other ) {
   flags         = other.flags;
   description   = other.description;
   liquidus      = other.liquidus;
   preheatTime   = other.preheatTime;
   soakTemp1     = other.soakTemp1;
   soakTemp2     = other.soakTemp2;
   soakTime      = other.soakTime;
   rampUpSlope   = other.rampUpSlope;
   peakTemp      = other.peakTemp;
   peakDwell     = other.peakDwell;
   rampDownSlope = other.rampDownSlope;
}

///**
// * Prints the profile to stdout
// */
//void NvSolderProfile::print() const {
//   printf("{ \n");
//   printf("name          \"%s\"\n",  (const char *)description );
//   printf("flags         %2.2X\n",   (uint8_t)~(uint8_t)flags  );
//   printf("liquidus      %d\n",      (int)  liquidus           );
//   printf("preheatTime   %d\n",      (int)  preheatTime        );
//   printf("soakTemp1     %d\n",      (int)  soakTemp1          );
//   printf("soakTemp2     %d\n",      (int)  soakTemp2          );
//   printf("soakTime      %d\n",      (int)  soakTime           );
//   printf("ramp2Slope    %.1f\n",    (float)ramp2Slope         );
//   printf("peakTemp      %d\n",      (int)  peakTemp           );
//   printf("peakDwell     %d\n",      (int)  peakDwell          );
//   printf("rampDownSlope %4.1f\n",   (float)rampDownSlope      );
//   printf("};\n");
//}

/** Amtech 4300 63Sn/37Pb leaded profile */
const SolderProfile am4300profileA {
      /* Soak 140-180C/60-90s, above liquidus(183 C) for 30-60s, peak 200-230 C */
      /* flags         */ 0,
      /* description   */ "4300 63SN/37PB-a",
      /* liquidus      */ 183,
      /* preheatTime   */ 115,
      /* soakTemp1     */ 140,
      /* soakTemp2     */ 183,
      /* soakTime      */ 90,
      /* ramp2Slope    */ 1.4,
      /* peakTemp      */ 210,
      /* peakDwell     */ 15,
      /* rampDownSlope */ -3.0,
};

/** Amtech 4300 63Sn/37Pb leaded profile */
const SolderProfile am4300profileB {
      /* Soak 140-180C/90-120s, above liquidus(183 C) for 30-60s, peak 200-230 C */
      /* flags         */ 0,
      /* description   */ "4300 63SN/37PB-b",
      /* liquidus      */ 183,
      /* preheatTime   */ 115,
      /* soakTemp1     */ 140,
      /* soakTemp2     */ 183,
      /* soakTime      */ 120,
      /* ramp2Slope    */ 1.4,
      /* peakTemp      */ 210,
      /* peakDwell     */ 15,
      /* rampDownSlope */ -3.0,
};

/** NC-31 low-temperature lead-free profile */
const SolderProfile nc31profile {
      /* Soak 90-140C/60-90s, above liquidus(138 C) for 60 s, peak 158-165 C */
      /* flags         */ 0,
      /* description   */ "NC-31 LOW-TEMP LF",
      /* liquidus      */ 140,
      /* preheatTime   */ 65,
      /* soakTemp1     */ 90,
      /* soakTemp2     */ 140,
      /* soakTime      */ 75,
      /* ramp2Slope    */ 3,
      /* peakTemp      */ 160,
      /* peakDwell     */ 15,
      /* rampDownSlope */ -3.0,
};

/** SynTECH-LF normal temperature lead-free profile */
const SolderProfile syntechlfprofile {
      /* Soak 140-200C/60-90s ramp@<2C/s, above liquidus(219 C) for 30-60s, peak 230-249 C */
      /* flags         */ 0,
      /* description   */ "AMTECH SYNTECH-LF",
      /* liquidus      */ 219,
      /* preheatTime   */ 115,
      /* soakTemp1     */ 140,
      /* soakTemp2     */ 200,
      /* soakTime      */ 75,
      /* ramp2Slope    */ 3,
      /* peakTemp      */ 240,
      /* peakDwell     */ 20,
      /* rampDownSlope */ -3.0,
};

/** Default for editing - Amtech 4300 63Sn/37Pb leaded profile */
const SolderProfile defaultProfile {
      /* Soak 140-180C/60-90s, above liquidus(183 C) for 30-60s, peak 200-230 C */
      /* flags         */ P_UNLOCKED,
      /* description   */ "Empty",
      /* liquidus      */ 183,
      /* preheatTime   */ 144,
      /* soakTemp1     */ 140,
      /* soakTemp2     */ 183,
      /* soakTime      */ 90,
      /* ramp2Slope    */ 1.4,
      /* peakTemp      */ 220,
      /* peakDwell     */ 10,
      /* rampDownSlope */ -3.0,
};

/** The actual profiles in nonvolatile memory */
__attribute__ ((section(".flexRAM")))
NvSolderProfile profiles[MAX_PROFILES];

