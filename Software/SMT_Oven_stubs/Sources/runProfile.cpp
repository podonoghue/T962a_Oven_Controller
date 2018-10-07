/**
 * @file    runProfile.cpp
 * @brief   Runs Solder profiles
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */

#include <copyProfile.h>
#include <dataPoint.h>
#include <EditProfile.h>
#include <math.h>
#include <plotting.h>
#include <reporter.h>
#include <RemoteInterface.h>
#include <SolderProfile.h>

#include "hardware.h"
#include "cmsis.h"
#include "configure.h"
#include "messageBox.h"

using namespace USBDM;
using namespace std;

namespace Monitor {
/**
 * Monitor thermocouple status
 * Also allows thermocouples to be disabled
 */
void monitor() {
   static auto prompt = []() {
      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T1");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(6);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T2");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(6);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T3");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(6);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T4");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(6);
      lcd.setInversion(true); lcd.putSpace(4); lcd.putString("Exit"); lcd.putSpace(4); lcd.setInversion(false);
      lcd.gotoXY(0, 12+4*lcd.FONT_HEIGHT+2);
      float temp = temperatureSensors.getLastMeasurement().getAverageTemperature();
      if (!isnan(temp)) {
         lcd.printf("Average T=%0.1f\x7F", temp);
      }
   };

   // Used to report thermocouple status
   Reporter::reset();
   Reporter::setTextPrompt(prompt);
   Reporter::setDisplayFormat(Reporter::DisplayTable);

   // Time in monitor sequence
   int   time  = 0;

   temperatureSensors.updateMeasurements();
   Reporter::displayProfileProgress();

   do {
      // Update display
      temperatureSensors.updateMeasurements();
      Reporter::addLogPoint(time, s_off);
      Reporter::displayProfileProgress();

      // Get key-press
      SwitchValue key = buttons.getButton(100);

      switch(key) {
      case SwitchValue::SW_F1: temperatureSensors.getThermocouple(0).toggleEnable(); break;
      case SwitchValue::SW_F2: temperatureSensors.getThermocouple(1).toggleEnable(); break;
      case SwitchValue::SW_F3: temperatureSensors.getThermocouple(2).toggleEnable(); break;
      case SwitchValue::SW_F4: temperatureSensors.getThermocouple(3).toggleEnable(); break;
      case SwitchValue::SW_S:
         return;
      default:
         break;
      }
   } while (true);
}
};

namespace RunProfile {

/** Profile being run */
static const NvSolderProfile *currentProfile;

/** Time in the sequence (seconds) */
static volatile int time;

/** Heater set-point */
static volatile float setpoint;

/** Used to record ambient temperature at start (Celsius) */
static float ambient;

/** State in the profile sequence */
static State state = s_off;

/**
 * Call-back from the timer to step through the profile state-machine
 *                      .--.
 *                     /    \
 *                    /      \
 *               .....        \
 *          ....               \
 *      ....                    \
 *     /                         \
 *    /                           \
 *   /                             \
 *  /                               \
 * /                   dwell         \
 * preheat  soak   ramp_up  ramp_down
 */
static void handler(const void *) {

   /* Records start of soak time */
   static int startOfSoakTime;

   /* Records start of dwell time */
   static int startOfDwellTime;

   /* Used for timeout for profile changes */
   static int timeout;

   /* Tolerance of temperature checks - Celsius */
   constexpr int DELTA = 2;

   // Get current temperature (NAN on thermocouple failure)
   float currentTemperature = temperatureSensors.getTemperature();

   if (std::isnan(currentTemperature)) {
      state = s_fail;
   }

#ifdef DEBUG_BUILD
   if ((state != s_init) && (setpoint>(ambient+2))) {
      // Dummy for testing sequence without oven
      currentTemperature = setpoint-2+(rand()%4);
   }
#endif

   PRINTF("%3d, %10s: TO=%3d, T=%4.1f, SP=%4.1f\n", time, Reporter::getStateName(state), timeout, currentTemperature, setpoint);

   // Handle state
   switch (state) {
      case s_complete:
      case s_fail:
         // Not operating
         pid.setSetpoint(0);
         pid.enable(false);
         ovenControl.setHeaterDutycycle(0);
         ovenControl.setFanDutycycle(0);
         return;
      case s_off:
      case s_manual:
         return;
      case s_init:
         /*
          * Startup
          */
         time     = 0;
         state    = s_preheat;

         // Use starting temperature as ambient reference
         ambient  = currentTemperature;

         pid.setTunings(pidKp, pidKi, pidKd);
         pid.setSetpoint(ambient);
         pid.enable();

         // Calculate maximum time for preheat ramp to complete (10% over)
         timeout = (int)round(1.1*currentProfile->preheatTime);

         PRINTF("Starting sequence, Ta=%f\n", ambient);
         // no break
      case s_preheat:
         /*
          * Set-point follows profile temperature from ambient to start of soak temperature/time
          * It will delay for a short while (10% of preheat time) if soakTemp1 not yet reached
          *
          * Ambient -> soakTemp1 @ ramp1Slope
          */
         if (time<currentProfile->preheatTime) {
            // Following profile
            setpoint = ambient + (time/(float)currentProfile->preheatTime)*(currentProfile->soakTemp1-ambient);
            pid.setSetpoint(setpoint);
         }
         else if (currentTemperature>=(currentProfile->soakTemp1-DELTA)) {
            // Reached end of preheat time
            // Move on if nearly reached soak start temperature
            // This allows for tolerances in the PID controller
            state = s_soak;
            startOfSoakTime = time;

            // Calculate timeout for soak ramp (20% over)
            timeout = startOfSoakTime+(int)round(1.2*currentProfile->soakTime);
         }
         else if (time>timeout) {
            // Timeout
            state = s_fail;
         }
         break;
      case s_soak:
         /*
          * Heat from soak start temperature to soak end temperature over soak time
          * It will delay for a while (20% of soak time) if soakTemp2 temperature not yet reached
          *
          * soakTemp1 -> soakTemp2 over soakTime time
          */
         if (time<(startOfSoakTime+currentProfile->soakTime)) {
            // Following profile
            setpoint = currentProfile->soakTemp1 +
                  (time-startOfSoakTime)*(currentProfile->soakTemp2-currentProfile->soakTemp1)/currentProfile->soakTime;
            pid.setSetpoint(setpoint);
         }
         else if (currentTemperature>=(currentProfile->soakTemp2-DELTA)) {
            // Reached end of soak time
            // Move on if nearly reached final soak temperature
            // This allows for tolerances in the PID controller
            state = s_ramp_up;

            // Calculate timeout for ramp up to peak ramp (100% over - THE OVEN REALLY CAN'T COPE WITH THE RAMP)
            timeout = time + (int)round(2*(currentProfile->peakTemp-setpoint)/currentProfile->rampUpSlope);
         }
         else if (time>timeout) {
            // Timeout
            state = s_fail;
         }
         break;
      case s_ramp_up:
         /*
          * Heat from soak end temperature to peak at rampUp rate
          * Will wait until reached T~peakTemp with timeout
          *
          * soakTemp2 -> peakTemp @ ramp2Slope
          */
         if (setpoint < currentProfile->peakTemp) {
            // Following profile
            setpoint += currentProfile->rampUpSlope;
            pid.setSetpoint(setpoint);
         }
         else if (currentTemperature >= (currentProfile->peakTemp-DELTA)) {
            state = s_dwell;
            startOfDwellTime = time;

            // No timeout
            timeout = 0;
         }
         else {
            if (time>timeout) {
               state = s_fail;
            }
         }
         break;
      case s_dwell:
         /*
          * Remain at peak temperature for dwell time
          *
          * peakTemp for peakDwell
          */
         if (time >= (startOfDwellTime+currentProfile->peakDwell)) {
            state = s_ramp_down;
         }
         break;
      case s_ramp_down:
         /* Ramp down at rampDown rate without timeout
          *
          * peakTemp -> ambient @ rampDown
          */
         if (setpoint > ambient) {
            setpoint += currentProfile->rampDownSlope;
         }
         pid.setSetpoint(setpoint);
         if (currentTemperature<=ambient+20) {
            state = s_complete;
         }
         break;
   }
   // Add data point to record
   Reporter::addLogPoint(time, state);

   // Advance time
   time++;
};

/** Timer used to run a profile */
CMSIS::Timer timer{handler};

/**
 * Start running a profile.
 * This will:
 * - Set profiles to use
 * - Set initial state
 * - Start timer
 *
 * @param[in] profile Profile to run
 *
 * @return true  Successfully started
 *
 * @return false Failed
 */
bool startRunProfile(NvSolderProfile &profile) {

   // Clear data
   Draw::reset();

   // Check if thermocouples can measure temperature
   if (std::isnan(getTemperature())) {
      state = s_fail;
      return false;
   }
   currentProfile = &profile;
   state          = s_init;

   // Start Timer callback
   timer.create();
   timer.start(1.0);

   return true;
}

/**
 * Complete the current sequence
 */
void completeRunProfile() {
   // Stop timer callback
   timer.stop();
   timer.destroy();

   // Stop PID controller
   pid.enable(false);
   pid.setSetpoint(0);

   Reporter::addLogPoint(time, state);

   ovenControl.setHeaterDutycycle(0);
   ovenControl.setFanDutycycle(100);
}

/**
 * Abort the current sequence
 */
void abortRunProfile() {

   completeRunProfile();
   state = s_fail;

}

/**
 * Run the current profile
 *
 * @return true Successfully started
 * @return false Failed to start
 */
bool remoteStartRunProfile() {
   return startRunProfile(profiles[currentProfileIndex]);
}

/**
 * Run the current profile
 *
 * @return State of profile state machine
 */
State remoteCheckRunProfile() {
   return (state);
}

/**
 * Run the current profile
 */
void runProfile() {

   if (!checkThermocouples()) {
      return;
   }

   char buff[100];
   snprintf(buff, sizeof(buff), "%d:%s\n\nRun Profile?", (int)currentProfileIndex, (const volatile char *)profiles[currentProfileIndex].description);
   MessageBoxResult rc = messageBox("Run Profile", buff, MSG_YES_NO);
   if (rc != MSG_IS_YES) {
      return;
   }

   startRunProfile(profiles[currentProfileIndex]);

   // Menu for thermocouple screen
   static auto textPrompt = []() {
      lcd.gotoXY(lcd.LCD_WIDTH-lcd.FONT_WIDTH*10-6, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Plot");  lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(6);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Stop");  lcd.putSpace(3); lcd.setInversion(false);

      lcd.gotoXY(0, 12+4*lcd.FONT_HEIGHT+2);
      lcd.printf("%2ds", (int)round(pid.getElapsedTime()));
      lcd.gotoXY(5*lcd.FONT_WIDTH+1, 12+4*lcd.FONT_HEIGHT+2);
      lcd.printf("T=%5.1f\x7F", pid.getInput());
      lcd.gotoXY(13*lcd.FONT_WIDTH+2, 12+4*lcd.FONT_HEIGHT+2);
      lcd.printf("Set=%3d\x7F", (int)round(pid.getSetpoint()));

      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.putString(Reporter::getStateName(state));
   };

   // Menu for plot screen
   static auto graphicPrompt = []() {
      constexpr int xTimeOffset = 17;
      constexpr int yTimeOffset = 8;
      constexpr int xMenuOffset = lcd.LCD_WIDTH-26;
      constexpr int yMenuOffset = 8;

      lcd.setInversion(true);
      lcd.gotoXY(xTimeOffset, yTimeOffset);
      lcd.printf("%3ds", (int)round(pid.getElapsedTime()));
      lcd.gotoXY(xMenuOffset, yMenuOffset);
      lcd.putSpace(1); lcd.putString("F4"); lcd.putSpace(1); lcd.putString("Th");
      lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*1);
      lcd.putSpace(1); lcd.putString("S");  lcd.putSpace(1); lcd.putString("Stp");
      lcd.setInversion(false);
   };

   Reporter::DisplayMode plotDisplay = Reporter::DisplayTable;

   Reporter::reset();
   Reporter::setTextPrompt(textPrompt);
   Reporter::setPlotPrompt(graphicPrompt);
   Reporter::setDisplayFormat(plotDisplay);
   Reporter::setProfile(currentProfileIndex);

   // Wait for completion with update approximately every second
   uint32_t last = osKernelSysTick();
   for(;;) {
      uint32_t now = osKernelSysTick();
      if ((uint32_t)(now - last) >= osKernelSysTickMicroSec(1000000U)) {
         temperatureSensors.updateMeasurements();
         last += osKernelSysTickMicroSec(1000000U);
//         Reporter::addLogPoint(time, state);
      }
      // Update display
      Reporter::displayProfileProgress();

      SwitchValue key = buttons.getButton(10);
      if ((state == s_complete) || (state == s_fail)) {
         completeRunProfile();
         break;
      }
      if (key == SwitchValue::SW_S) {
         abortRunProfile();
         break;
      }
      if (key == SwitchValue::SW_F4) {
         plotDisplay = Reporter::toggle(plotDisplay);
         Reporter::setDisplayFormat(plotDisplay);
      }
   }

   Reporter::setDisplayFormat(Reporter::DisplayTable);

   // Sound buzzer
   Buzzer::play();
   static auto completedPrompt = []() {
      lcd.gotoXY(0, 12+4*lcd.FONT_HEIGHT+2);
      lcd.printf("%4ds", (int)round(pid.getElapsedTime()));
      lcd.gotoXY(5*lcd.FONT_WIDTH+2, 12+4*lcd.FONT_HEIGHT+2);
      lcd.printf("T=%0.1f\x7F Set=%3d\x7F", pid.getInput(), (int)round(pid.getSetpoint()));

      lcd.gotoXY(128-4-lcd.FONT_WIDTH*17+2*4, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putSpace(3);
      lcd.putString((state==s_complete)?"Complete - Exit":"Failed   - Exit");
      lcd.putSpace(3); lcd.setInversion(false);
   };

   // Used to report thermocouple status
   Reporter::setTextPrompt(completedPrompt);

   // Report every second until key-press
   do {
      Reporter::displayProfileProgress();
   } while (buttons.getButton(1000) == SwitchValue::SW_NONE);

   ovenControl.setFanDutycycle(0);
   state = s_off;
}

/**
 * Draws the screen for manual mode
 */
void drawManualScreen() {
   lcd.clearFrameBuffer();

   lcd.setInversion(true); lcd.putString("  Manual Mode\n"); lcd.setInversion(false);

   lcd.printf("On Time   = %5.1fs\n", pid.getElapsedTime());

   lcd.printf("Set Temp  = %5.1f\x7F\n", pid.getSetpoint());

   lcd.printf("Oven Temp = %5.1f\x7F\n", temperatureSensors.getTemperature());

   if (ovenControl.getHeaterDutycycle() == 0) {
      lcd.printf("Heater = off\n");
   }
   else {
      lcd.printf("Heater = on (%d%%)\n", ovenControl.getHeaterDutycycle());
   }
   if (ovenControl.getFanDutycycle() == 0) {
      lcd.printf("Fan    = off\n");
   }
   else {
      lcd.printf("Fan    = on (%d%%) \n", ovenControl.getFanDutycycle());
   }
   lcd.gotoXY(7*lcd.FONT_WIDTH+22, lcd.LCD_HEIGHT-2*lcd.FONT_HEIGHT);
   if (state == s_manual) {
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Temp");  lcd.putSpace(2);
   }
   else {
      lcd.setInversion(true); lcd.putSpace(6); lcd.putString("Fan");  lcd.putSpace(5);
   }
   if (state != s_manual) {
      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Fan");  lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
   }
   lcd.gotoXY(0+3*lcd.FONT_WIDTH+6+5, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Heat"); lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(5);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putString("+");    lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(5);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putString("-");    lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(5);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Exit"); lcd.putSpace(3); lcd.setInversion(false);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

/**
 * Manually control oven
 */
void manualMode() {

   if (!checkThermocouples()) {
      // No thermocouples
      return;
   }

   int fanSpeed = 100;

   state = s_off;

   time = 0;
   pid.setSetpoint(100);
   pid.enable(false);

   // Wait for completion with update approximately every second
   int      time = 0;
   uint32_t last = osKernelSysTick();
   for(;;) {
      /**
       * Safety check
       * Turn off after 800 seconds of operation
       */
      if (pid.getElapsedTime()>=maxHeaterTime) {
         pid.enable(false);
         ovenControl.setHeaterDutycycle(0);
         state = s_off;
      }
      uint32_t now = osKernelSysTick();
      if ((uint32_t)(now - last) >= osKernelSysTickMicroSec(1000000U)) {
         temperatureSensors.updateMeasurements();
         last += osKernelSysTickMicroSec(1000000U);
//         logger(++time);
         Reporter::addLogPoint(++time, state);
      }
      // Update display
      drawManualScreen();
      switch (buttons.getButton(10)) {
      case SwitchValue::SW_F1:
         // Fan toggle
         if (state == s_off) {
            // Heater not operating - Fan control enabled
            if (ovenControl.getFanDutycycle()>0) {
               // Fan on - turn off
               ovenControl.setFanDutycycle(0);
            }
            else {
               // Fan off - turn on
               ovenControl.setFanDutycycle(fanSpeed);
            }
         }
         break;
      case SwitchValue::SW_F2:
         // Heater toggle
         if (state == s_manual) {
            // Turn off heater
            state = s_off;
            fanSpeed = ovenControl.getFanDutycycle();
            pid.enable(false);
            ovenControl.setHeaterDutycycle(0);
            // Leave fan running at current speed
         }
         else {
            // Turn on heater
            state = s_manual;
            pid.setTunings(pidKp, pidKi, pidKd);
            pid.enable(true);
            // PID will control fan+heater
         }
         break;
      case SwitchValue::SW_F3:
         // Increment
         if (state == s_manual) {
            // Oven operating
            // Increase temperature set-point
            int t = pid.getSetpoint();
            if (t<255) {
               pid.setSetpoint(t + 5);
            }
         }
         else {
            // Oven not operating
            // Increase Fan speed
            if (fanSpeed<100) {
               fanSpeed++;
               ovenControl.setFanDutycycle(fanSpeed);
            }
         }
         break;
      case SwitchValue::SW_F4:
         // Decrement
         if (state == s_manual) {
            // Oven operating
            // Decrease temperature set-point
            int t = pid.getSetpoint();
            if (t>0) {
               pid.setSetpoint(t - 5);
            }
         }
         else {
            // Oven not operating
            // Decrease Fan speed
            if (fanSpeed>0) {
               fanSpeed--;
               ovenControl.setFanDutycycle(fanSpeed);
            }
         }
         break;
      case SwitchValue::SW_S:
         // Exit
         pid.setSetpoint(0);
         pid.enable(false);
         ovenControl.setHeaterDutycycle(0);
         ovenControl.setFanDutycycle(0);
         return;
      default:
         break;
      }
   }
}
}; // namespace RunProfile
