/**
 * @file    Reporter.h
 * @brief   Handles reporting of progress
 *
 *  Created on: 18Mar.,2017
 *      Author: podonoghue
 */

#ifndef REPORTER_H_
#define REPORTER_H_

#include <dataPoint.h>

namespace Reporter {

/** Indicates format shown on LCD  */
enum DisplayMode {
   DisplayPlot,   /** Plot showing temperature and profile hsitory */
   DisplayTable,  /** Current temperatures in table */
};

/**
 * Returns the opposite of the display mode provided
 *
 * @param[in] mode Mode to toggle
 *
 * @return Opposite mode
 */
static inline DisplayMode toggle(DisplayMode mode) {
   return (mode==DisplayPlot)?DisplayTable:DisplayPlot;
}

/**
 * Get state name as string
 *
 * @param[in] state State to get name for
 *
 * @return Pointer to static string
 */
const char *getStateName(State state);

/**
 * Reset reporting
 */
void reset();
/**
 * Set prompt to print for text display
 *
 * @param prompt Prompt to print
 */
void setTextPrompt(void (*prompt)());

/**
 * Control whether a text or plot display is used on LCD
 *
 * @param[in] mode Either DisplayPlot or DisplayTable
 */
void setDisplayFormat(DisplayMode mode);

/**
 * Set prompt to print for text display
 *
 * @param[in] prompt Call-back to obtain prompt to display
 */
void setTextPrompt(void (*prompt)());

/**
 * Set prompt to print in plot mode
 *
 * @param[in] prompt Call-back to obtain prompt to display
 */
void setPlotPrompt(void (*prompt)());

/**
 * Set profile to use when plotting to LCD
 *
 * @param[in] profileIndex Index of profile to use
 */
void setProfile(int profile);

/**
 * Reports thermocouple status on LCD
 */
void displayProfileProgress();

/**
 * Record data point for logging.\n
 * Actual temperature information is obtained from the thermocouples.
 *
 * @param[in] time  Time for report
 * @param[in] state State for report
 */
void addLogPoint(int time, State state);

};

#endif /* REPORTER_H_ */
