/**
 * @file    Reporter.cpp
 * @brief   Handles reporting of progress
 *
 *  Created on: 18Mar.,2017
 *      Author: podonoghue
 */
#include "configure.h"
#include "dataPoint.h"
#include "math.h"
#include "plotting.h"
#include "reporter.h"
#include "RemoteInterface.h"

namespace Reporter {

/** Dialogue prompt for text mode */
static void (*fTextPrompt)() = nullptr;

/** Dialogue prompt for plot mode*/
static void (*fPlotPrompt)() = nullptr;

/** Indicates whether plot or text report is shown on LCD */
static DisplayMode usePlot = DisplayTable;

/** Profile being used */
static int fProfile;

/**
 * Get state name as string
 *
 * @param[in] state State to get name for
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
 * @param[in] time  Time for report
 * @param[in] state State for report
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
   lcd.putSpace(14); lcd.write("Status Oven  ColdJn\n");
   lcd.drawHorizontalLine(9);
   lcd.gotoXY(0, 12);

   // Get temperatures
   const DataPoint &dataPoint = temperatureSensors.getLastMeasurement();
   lcd.setWidth(6);
   for (unsigned t=0; t<TemperatureSensors::NUM_THERMOCOUPLES; t++) {
      float temperature, coldReference;
      Max31855::ThermocoupleStatus status = dataPoint.getTemperature(t, temperature);
      coldReference = temperatureSensors.getColdReferences(t);

      lcd.write("T").write(t+1);lcd.putSpace(2);
      if (status == Max31855::TH_ENABLED) {
         lcd.write(Max31855::getStatusName(status)).write(" ").write(temperature).write("\x7F ").write(coldReference).writeln("\x7F");
//         lcd.printf("%-4s %5.1f\x7F %5.1f\x7F\n", Max31855::getStatusName(status), temperature, coldReference);
      }
      else if (status != Max31855::TH_MISSING) {
         temperature = 0;
         lcd.write(Max31855::getStatusName(status)).write("  ----  ").write(coldReference).writeln("\x7F ");
      }
      else {
         temperature = 0;
         lcd.writeln(Max31855::getStatusName(status));
      }
   }
   lcd.setWidth(0);
   if (fTextPrompt != nullptr) {
      fTextPrompt();
   }
}

/**
 * Reports thermocouple status on LCD
 */
void displayProfileProgress() {
   switch(usePlot) {
      case DisplayPlot:
         writePlot();
         break;
      case DisplayTable:
         writeThermocoupleStatus();
         break;
   }
   lcd.refreshImage();
   lcd.setGraphicMode();
}

/**
 * Set prompt to print for text display
 *
 * @param[in] prompt Call-back to obtain prompt to display
 */
void setTextPrompt(void (*prompt)()) {
   fTextPrompt = prompt;
}
/**
 * Set prompt to print in plot mode
 *
 * @param[in] prompt Call-back to obtain prompt to display
 */
void setPlotPrompt(void (*prompt)()) {
   fPlotPrompt = prompt;
}

/**
 * Control whether a text or plot display is used on LCD
 *
 * @param[in] mode Either DisplayPlot or DisplayTable
 */
void setDisplayFormat(DisplayMode mode) {
   usePlot = mode;
}

/**
 * Set profile to use when plotting to LCD
 *
 * @param[in] profileIndex Index of profile to use
 */
void setProfile(int profileIndex) {
   fProfile = profileIndex;
   Draw::drawProfile(profileIndex);
}

}; // end namespace Reporter
