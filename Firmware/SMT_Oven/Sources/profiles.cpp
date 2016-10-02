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
            /**           Initial      Ramp Up      Peak Up        Dwell    Ramp Down   Final
             *         <-----------><-----------><----------><----------><----------><--------->   */
            /* */ {  0,  AMBIENT_TEMP}, { 90,  140}, {210, 183}, {230, 210}, {250, 210}, {360,  AMBIENT_TEMP}, {361, STOP_TEMP},
      }
};

//// NC-31 low-temperature lead-free profile
//const SolderProfile nc31profile = {
//      "NC-31 LOW-TEMP LF", {
//            /*     0   10   20   30   40   50   60   70   80   90 */
//            /**/  50,  50,  50,  50,  55,  70,  85,  90,  95, 100,
//            /*   100  110  120  130  140  150  160  170  180  190 */
//            /**/ 102, 105, 107, 110, 112, 115, 117, 120, 122, 127,
//            /*   200  210  220  230  240  250  260  270  280  290 */
//            /**/ 132, 138, 148, 158, 160, 158, 148, 138, 130, 122,
//            /*   300  310  320  330  340  350  360  370  380  390 */
//            /**/ 114, 106,  98,  90,  82,  74,  66,  58,   0,   0,
//            /*   400  410  420  430  440  450  460  470  480  490 */
//            /**/   0,   0,   0,   0,   0,   0,   0,   0
//      }
//};

//// SynTECH-LF normal temperature lead-free profile
//const SolderProfile syntechlfprofile = {
//      "AMTECH SYNTECH-LF", {
//            /*     0   10   20   30   40   50   60   70   80   90 */
//            /**/  50,  50,  50,  50,  60,  70,  80,  90, 100, 110,
//            /*   100  110  120  130  140  150  160  170  180  190 */
//            /**/ 120, 130, 140, 149, 158, 166, 175, 184, 193, 201,
//            /*   200  210  220  230  240  250  260  270  280  290 */
//            /**/ 210, 219, 230, 240, 245, 240, 230, 219, 212, 205,
//            /*   300  310  320  330  340  350  360  370  380  390 */
//            /**/ 198, 191, 184, 177, 157, 137, 117,  97,  77,  57,
//            /*   400  410  420  430  440  450  460  470  480  490 */
//            /**/   0,   0,   0,   0,   0,   0,   0,   0
//      }
//};

#ifdef DEBUG_BUILD
// Short test temperature profile
const SolderProfile short_testprofile = {
      "SHORT TEST", {
            /**           Initial      Soak       Ramp Up      Peak Up        Dwell    Ramp Down   Final
             *         <-----------><----------><-----------><----------><----------><----------><--------->   */
            /* */ {  0,  AMBIENT_TEMP}, { 20,  140}, {40, 183}, {600, 210}, {80, 210}, {100,  AMBIENT_TEMP}, {101, STOP_TEMP},
      }
};
// Ramp speed test temperature profile
const SolderProfile rampspeed_testprofile = {
      "SHORT TEST", {
            /* */ {  0,  AMBIENT_TEMP}, {100, 240}, {140, 240}, {240, AMBIENT_TEMP}, {241, STOP_TEMP},
      }
};
// Small step change
const SolderProfile pidcontrol_testprofile = {
      "SHORT TEST", {
            /* */ {  0,  AMBIENT_TEMP}, {100, 200}, {101, 220}, {140, 220}, {141, 200}, {240, AMBIENT_TEMP}, {241, STOP_TEMP},
      }
};
#endif

__attribute__ ((section(".flexRAM")))
NvSolderProfile profiles[MAX_PROFILES];

