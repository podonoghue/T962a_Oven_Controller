/**
 * @file    runProfile.h
 * @brief   Runs Solder profiles
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_RUNPROFILE_CPP_
#define SOURCES_RUNPROFILE_CPP_

#include "dataPoint.h"

namespace Monitor {
/**
 * Monitor thermocouple status
 * Also allows thermocouples to be disabled
 */
extern void monitor();

}; // namespace Monitor

namespace RunProfile {

/**
 * Draw profile to LCD
 *
 * @param profile The profile to draw
 */
extern void drawProfile(const NvSolderProfile &profile);

/**
 * Start running the current profile remotely
 */
bool remoteStartRunProfile();

/**
 * Abort the current profile sequence
 */
extern void abortRunProfile();

/**
 * Check remote run profile remotely
 */
extern State remoteCheckRunProfile();

/**
 * Run profile interactively\n
 * Doesn't return until complete
 */
extern void runProfile();

/**
 * Manually control oven
 */
extern void manualMode();

/**
 * Display profiles for selection or editing
 *
 * On exit the current profile may be changed
 */
extern void profileMenu();

}; // namespace RunProfile

#endif /* SOURCES_RUNPROFILE_CPP_ */
