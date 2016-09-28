/*
 * profiles.cpp
 *
 * The following profiles are from:
 *
 * https://github.com/UnifiedEngineering/T-962-improvements
 *
 */
#include "profiles.h"
// Amtech 4300 63Sn/37Pb leaded profile
const SolderProfile am4300profile = {
   "4300 63SN/37PB", {
       50, 50, 50, 60, 73, 86,100,113,126,140,143,147,150,154,157,161, // 0-150s
      164,168,171,175,179,183,195,207,215,207,195,183,168,154,140,125, // Adjust peak from 205 to 220C
      111, 97, 82, 68, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
   }
};
// NC-31 low-temp lead-free profile
const SolderProfile nc31profile = {
   "NC-31 LOW-TEMP LF", {
       50, 50, 50, 50, 55, 70, 85, 90, 95,100,102,105,107,110,112,115, // 0-150s
      117,120,122,127,132,138,148,158,160,158,148,138,130,122,114,106, // Adjust peak from 158 to 165C
       98, 90, 82, 74, 66, 58,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
   }
};

// SynTECH-LF normal temp lead-free profile
const SolderProfile syntechlfprofile = {
   "AMTECH SYNTECH-LF", {
       50, 50, 50, 50, 60, 70, 80, 90,100,110,120,130,140,149,158,166, // 0-150s
      175,184,193,201,210,219,230,240,245,240,230,219,212,205,198,191, // Adjust peak from 230 to 249C
      184,177,157,137,117, 97, 77, 57,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
   }
};

// Ramp speed test temp profile
const SolderProfile rampspeed_testprofile = {
   "RAMP TEST", {
       50, 50, 50, 50,245,245,245,245,245,245,245,245,245,245,245,245, // 0-150s
      245,245,245,245,245,245,245,245,245, 50, 50, 50, 50, 50, 50, 50, // 160-310s
       50, 50, 50, 50, 50, 50, 50, 50,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
   }
};

// PID gain adjustment test profile (5% setpoint change)
const SolderProfile pidcontrol_testprofile = {
   "PID TEST",  {
      171,171,171,171,171,171,171,171,171,171,171,171,171,171,171,171, // 0-150s
      180,180,180,180,180,180,180,180,171,171,171,171,171,171,171,171, // 160-310s
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
   }
};

__attribute__ ((section(".flexRAM")))
NvSolderProfile profiles[MAX_PROFILES];

