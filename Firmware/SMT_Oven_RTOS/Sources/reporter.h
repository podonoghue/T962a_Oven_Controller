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
/**
 * Get state name as string
 *
 * @param state State to get name for
 *
 * @return Pointer to static string
 */
const char *getStateName(State state);

/**
 * Set prompt to print for text display
 *
 * @param prompt Prompt to print
 */
void setTextPrompt(void (*prompt)());

/**
 * Control whether a text or plot display is used on LCD
 *
 * @param value True => Plot display, false => Text display
 */
void setDisplayFormat(bool value);

/**
 * Set prompt to print for text display
 *
 * @param prompt Prompt to print
 */
void setTextPrompt(void (*prompt)());

/**
 * Set prompt to print in plot mode
 *
 * @param prompt Prompt to print
 */
void setPlotPrompt(void (*prompt)());

/**
 * Set profile to use when plotting to LCD
 *
 * @param profile Profile to use
 */
void setProfile(int profile);

/**
 * Reports thermocouple status on LCD
 */
void displayThermocoupleStatus();

/**
 * Record data point for logging.\n
 * Actual temperature information is obtained from the thermocouples.
 *
 * @param time  Time for report
 * @param state State for report
 */
void addLogPoint(int time, State state);

/**
 * Writes thermocouple status to log
 */
void logThermocoupleStatus(int time, State state, const DataPoint &point);

};

#endif /* REPORTER_H_ */
