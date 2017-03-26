/**
 * @file    Reporter.cpp
 * @brief   Handles reporting of progress
 *
 *  Created on: 18Mar.,2017
 *      Author: podonoghue
 */
#include <dataPoint.h>
#include <math.h>
#include <plotting.h>
#include <reporter.h>
#include <RemoteInterface.h>
#include "configure.h"

namespace Reporter {

/** Dialogue prompt for text mode */
static void (*fTextPrompt)() = nullptr;

/** Dialogue prompt for plot mode*/
static void (*fPlotPrompt)() = nullptr;

/** Indicates whether plot or text report is shown on LCD */
static bool usePlot = false;

/** Profile being used */
static int fProfile;

/**
 * Get state name as string
 *
 * @param state State to get name for
 *
 * @return Pointer to static string
 */
const char *getStateName(State state) {
   switch(state) {
   case s_init      : return "init";
   case s_off       : return "off";
   case s_fail      : return "fail";
   case s_preheat   : return "preheat";
   case s_soak      : return "soak";
   case s_ramp_up   : return "ramp_up";
   case s_dwell     : return "dwell";
   case s_ramp_down : return "ramp_down";
   case s_complete  : return "complete";
   case s_manual    : return "manual";
   }
   return "invalid";
}

/**
 * Reset reporting
 */
void reset() {
   Draw::reset();
}

/**
 * Plot oven profile and history to LCD
 */
static void writePlot() {
   Draw::update();
   if (fPlotPrompt != nullptr) {
      fPlotPrompt();
   }
}

/**
 * Record data point for logging.\n
 * Actual temperature information is obtained from the thermocouples.
 *
 * @param time  Time for report
 * @param state State for report
 */
void addLogPoint(int time, State state) {
   DataPoint dataPoint;
   // Capture temperatures
   dataPoint = temperatureSensors.getLastMeasurement();
   dataPoint.setState(state);
   dataPoint.setTargetTemperature(pid.getSetpoint());
   dataPoint.setHeater(ovenControl.getHeaterDutycycle());
   dataPoint.setFan(ovenControl.getFanDutycycle());
   Draw::addDataPoint(time, dataPoint);
}

/**
 * Writes thermocouple status to LCD buffer
 */
static void writeThermocoupleStatus() {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();
   lcd.gotoXY(0,0);
   lcd.putSpace(14); lcd.putString("Status Oven  ColdJn\n");
   lcd.drawHorizontalLine(9);
   lcd.gotoXY(0, 12);

   // Get temperatures
   const DataPoint &dataPoint = temperatureSensors.getLastMeasurement();

   for (unsigned t=0; t<TemperatureSensors::NUM_THERMOCOUPLES; t++) {
      float temperature, coldReference;
      Max31855::ThermocoupleStatus status = dataPoint.getTemperature(t, temperature);
      coldReference = temperatureSensors.getColdReferences(t);

      lcd.printf("T%d:", t+1); lcd.putSpace(2);
      if (status == Max31855::TH_ENABLED) {
         lcd.printf("%-4s %5.1f\x7F %5.1f\x7F\n", Max31855::getStatusName(status), temperature, coldReference);
      }
      else if (status != Max31855::TH_MISSING) {
         temperature = 0;
         lcd.printf("%-4s  ----  %5.1f\x7F\n", Max31855::getStatusName(status), coldReference);
      }
      else {
         temperature = 0;
         lcd.printf("%-4s\n", Max31855::getStatusName(status));
      }
   }
   if (fTextPrompt != nullptr) {
      fTextPrompt();
   }
}

/**
 * Reports thermocouple status on LCD
 */
void displayThermocoupleStatus() {
   if (usePlot) {
      writePlot();
   }
   else {
      writeThermocoupleStatus();
   }
   lcd.refreshImage();
   lcd.setGraphicMode();
}

/**
 * Set prompt to print for text display
 *
 * @param prompt Prompt to print
 */
void setTextPrompt(void (*prompt)()) {
   fTextPrompt = prompt;
}
/**
 * Set prompt to print in plot mode
 *
 * @param prompt Prompt to print
 */
void setPlotPrompt(void (*prompt)()) {
   fPlotPrompt = prompt;
}

/**
 * Control whether a text or plot display is used on LCD
 *
 * @param value True => Plot display, false => Text display
 */
void setDisplayFormat(bool value) {
   usePlot = value;
}

/**
 * Set profile to use when plotting to LCD
 *
 * @param profile Profile to use
 */
void setProfile(int profile) {
   fProfile = profile;
   Draw::drawProfile(profile);
}

}; // end namespace Reporter
