/**
 * @file    runProfile.cpp
 * @brief   Runs Solder profiles
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */

#include <copyProfile.h>
#include <dataPoint.h>
#include <math.h>
#include <plotting.h>
#include <reporter.h>
#include <RemoteInterface.h>

#include "solderProfiles.h"
#include "hardware.h"
#include "cmsis.h"
#include "configure.h"
#include "messageBox.h"
#include "editProfile.h"

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
   Reporter::setDisplayFormat(false);

   // Time in monitor sequence
   int   time  = 0;

   temperatureSensors.updateMeasurements();
   Reporter::displayThermocoupleStatus();

   do {
      // Update display
      temperatureSensors.updateMeasurements();
      Reporter::addLogPoint(time, s_off);
      Reporter::displayThermocoupleStatus();

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

/*
 * Call-back from the timer to step through the profile statemachine
 */
static void handler(const void *) {

   /* Records start of soak time */
   static int startOfSoakTime;

   /* Records start of dwell time */
   static int startOfDwellTime;

   /* Used for timeout for profile changes */
   static int timeout;

   /* Tolerance of temperature checks */
   constexpr int DELTA = 5;

   // Get current temperature (NAN on thermocouple failure)
   const float currentTemperature = temperatureSensors.getTemperature();

   if (std::isnan(getTemperature())) {
      state = s_fail;
   }

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
      ambient  = currentTemperature;   // Use starting temperature as ambient reference
      time     = 0;
      setpoint = ambient;
      pid.setSetpoint(ambient);
      pid.enable();
      state    = s_preheat;
      // no break
   case s_preheat:
      /*
       * Heat from ambient to start of soak temperature
       * Will wait until reached T~soakTemp1 with 50s timeout
       *
       * Ambient -> soakTemp1 @ ramp1Slope
       */
      if (setpoint<currentProfile->soakTemp1) {
         // Follow profile
         setpoint += currentProfile->ramp1Slope;
         pid.setSetpoint(setpoint);
         timeout = 0;
      }
      else {
         // Reached end of profile
         // Move on if reached soak temperature or been nearly there for a while
         if ((currentTemperature>=currentProfile->soakTemp1) ||
               ((timeout>5)&&(currentTemperature>=(currentProfile->soakTemp1-DELTA)))) {
            // Reach soak temperature - move on
            state = s_soak;
            startOfSoakTime = time;
         }
         // Do timeout
         timeout++;
         if (timeout>50) {
            state = s_fail;
         }
      }
      break;
   case s_soak:
      /*
       * Heat from soak start temperature to soak end temperature over soak time
       * Will wait until reached T~soakTemp2 and minimum soak duration with timeout
       *
       * soakTemp1 -> soakTemp2 over soakTime time
       */
      if (setpoint<currentProfile->soakTemp2) {
         // Follow profile
         setpoint = currentProfile->soakTemp1 + (time-startOfSoakTime)*(currentProfile->soakTemp2-currentProfile->soakTemp1)/currentProfile->soakTime;
         pid.setSetpoint(setpoint);
         timeout = 0;
      }
      if (time >= (startOfSoakTime+currentProfile->soakTime)) {
         // Reached end of soak time
         // Move on if reached soak temperature or been nearly there for a while
         if ((currentTemperature>=currentProfile->soakTemp2) ||
               ((timeout>5)&&(currentTemperature>=(currentProfile->soakTemp2-DELTA)))) {
            // Reach soak temperature 2 - move on
            state = s_ramp_up;
         }
         else {
            // Reached end soak time - do timeout
            timeout++;
            if (timeout>40) {
               state = s_fail;
            }
         }
      }
      break;
   case s_ramp_up:
      /*
       * Heat from soak end temperature to peak at rampUp rate
       * Will wait until reached T~peakTemp with 40s timeout
       *
       * soakTemp2 -> peakTemp @ ramp2Slope
       */
      if (setpoint < currentProfile->peakTemp) {
         setpoint += currentProfile->ramp2Slope;
         pid.setSetpoint(setpoint);
         timeout = 0;
      }
      if (currentTemperature >= (currentProfile->peakTemp-DELTA)) {
         state = s_dwell;
         startOfDwellTime = time;
      }
      else {
         timeout++;
         if (timeout>40) {
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
      if (time>(startOfDwellTime+currentProfile->peakDwell)) {
         state = s_ramp_down;
      }
      break;
   case s_ramp_down:
      /* Ramp down at rampDown rate
       *
       * peakTemp -> 50 @ rampDown
       */
      if (setpoint > ambient) {
         setpoint += currentProfile->rampDownSlope;
      }
      pid.setSetpoint(setpoint);
      if (currentTemperature<ambient) {
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
CMSIS::Timer<osTimerPeriodic> timer{handler};

/**
 * Start running a profile.
 * This will:
 * - Set profiles to use
 * - Set initial state
 * - Start timer
 *
 * @param profile Profile to run
 *
 * @return true  Successfully started
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
   timer.start(1.0);

   return true;
}

/**
 * Abort the current sequence
 */
void abortRunProfile() {
   // Stop timer callback
   timer.stop();

   // Stop PID controller
   pid.enable(false);
   pid.setSetpoint(0);

   state = s_fail;

   Reporter::addLogPoint(time, state);

   ovenControl.setHeaterDutycycle(0);
   ovenControl.setFanDutycycle(100);
}

/**
 * Run the current profile
 *
 * @return true Successfully started
 * @return false Failed to start
 */
bool remoteStartRunProfile() {
   return startRunProfile(profiles[profileIndex]);
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
   snprintf(buff, sizeof(buff), "%d:%s\n\nRun Profile?", (int)profileIndex, (const volatile char *)profiles[profileIndex].description);
   MessageBoxResult rc = messageBox("Run Profile", buff, MSG_YES_NO);
   if (rc != MSG_IS_YES) {
      return;
   }

   startRunProfile(profiles[profileIndex]);

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

   bool plotDisplay = false;

   Reporter::reset();
   Reporter::setTextPrompt(textPrompt);
   Reporter::setPlotPrompt(graphicPrompt);
   Reporter::setDisplayFormat(plotDisplay);
   Reporter::setProfile(profileIndex);

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
      Reporter::displayThermocoupleStatus();

      SwitchValue key = buttons.getButton(10);
      if ((state == s_complete) || (state == s_fail)) {
         break;
      }
      if (key == SwitchValue::SW_S) {
         break;
      }
      if (key == SwitchValue::SW_F4) {
         plotDisplay = !plotDisplay;
         Reporter::setDisplayFormat(plotDisplay);
      }
   }

   abortRunProfile();

   Reporter::setDisplayFormat(false);

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
      Reporter::displayThermocoupleStatus();
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
            pid.enable(true);
            // PID will control fan+heater
         }
         break;
      case SwitchValue::SW_F3:
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
