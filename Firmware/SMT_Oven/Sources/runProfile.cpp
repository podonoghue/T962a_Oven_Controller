/*
 * runProfile.cpp
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */
#include <math.h>
#include "hardware.h"
#include "configure.h"
#include "profiles.h"
#include "messageBox.h"

using namespace USBDM;
using namespace std;

namespace RunProfile {

namespace Draw {

static constexpr int X_ORIGIN   = 16; // Pixels from left edge
static constexpr int Y_ORIGIN   = 7;  // Pixels from bottom edge

static constexpr int MIN_TEMP   = 50;   // Minimum temperature to plot (C)
static constexpr int MAX_TEMP   = 280;  // Maximum temperature to plot  (C)
static constexpr int MAX_TIME   = 9*60; // Maximum time to plot (s)
static constexpr int GRID_TIME  = 60;   // Time grid spacing (s) must be 60*N
static constexpr int GRID_TEMP  = 50;   // Temperature grid spacing (C)
static constexpr int SCALE_TIME = 6;    // Time -> pixel scaling (s/pixel)
static constexpr int SCALE_TEMP = 5;    // Temperature -> pixel scaling (C/pixel)

/** State when drawing profile */
static State state;

/** Time when drawing profile */
static int   time;

/** Set-point when drawing profile */
static float setpoint;

/**
 * Plot temperature point on screen.\n
 * Temperatures [51..270]\n
 * Time [0s..540s]
 *
 * @param time        Time for horizontal axis
 * @param temperature Temperature to plot
 */
static void plotPoint(int time, int temperature) {
   // Limit plot range
   if ((temperature<MIN_TEMP)||(temperature>MAX_TEMP)) {
      return;
   }
   if ((time<0)||(time>MAX_TIME)) {
      return;
   }
   int x = X_ORIGIN+(time/SCALE_TIME);
   int y = lcd.LCD_HEIGHT-Y_ORIGIN-((temperature-MIN_TEMP)/SCALE_TEMP);
   lcd.drawPixel(x,y);
}

static void drawAxis(const char *name) {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   // Horizontal axis minute axis ticks
   lcd.drawHorizontalLine(lcd.LCD_HEIGHT-Y_ORIGIN);
   for (int time=60; time<=MAX_TIME; time+=GRID_TIME) {
      lcd.gotoXY(X_ORIGIN+(time/SCALE_TIME)-3, lcd.LCD_HEIGHT-5);
      lcd.putSmallDigit(time/60);
   }
   static uint8_t min[] = {
         209,88,
         168,100,
         171,68,
         169,68,
         169,68,};
   lcd.putSpace(3);
   lcd.putCustomChar(min, 16, 5);

   // Vertical axis
   for (int temp=MIN_TEMP; temp<=MAX_TEMP; temp+=GRID_TEMP) {
      lcd.gotoXY(0, lcd.LCD_HEIGHT-Y_ORIGIN-((temp-MIN_TEMP)/SCALE_TEMP)-2);
      if (temp<100) {
         lcd.putSpace(5);
      }
      else {
         lcd.putSmallDigit(temp/100);
      }
      lcd.putSmallDigit((temp/10)%10);
      lcd.putSmallDigit(temp%10);
      lcd.putSpace(2);
   }
   lcd.drawVerticalLine(X_ORIGIN);
   // Grid
   for (int time=0; time<=MAX_TIME; time += GRID_TIME) {
      for (int temperature=MIN_TEMP; temperature<=MAX_TEMP; temperature+=GRID_TEMP) {
         plotPoint(time, temperature);
      }
   }
   // Menu
   constexpr int xMenuOffset = lcd.LCD_WIDTH-21;
   constexpr int yMenuOffset = 8;
   lcd.gotoXY(xMenuOffset, yMenuOffset);
   lcd.setInversion(true);
   lcd.putSpace(1); lcd.putString("F1"); lcd.putLeftArrow(); lcd.putSpace(2);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*1);
   lcd.putSpace(1); lcd.putString("F2"); lcd.putRightArrow(); lcd.putSpace(2);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*2);
   lcd.putSpace(1); lcd.putString("F3"); lcd.putSpace(2); lcd.putString("E"); lcd.putSpace(1);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*3);
   lcd.putSpace(1); lcd.putString("S "); lcd.putEnter(); lcd.putSpace(2);
   lcd.setInversion(false);

   // Name
   constexpr int xNameOffset = 0;
   constexpr int yNameOffset = 0;
   lcd.gotoXY(xNameOffset, yNameOffset);
   lcd.putString(name);
}

/*
 * Calculates one step of the profile
 */
void calculate(const NvSolderProfile &profile) {
   static int  startOfSoakTime;
   static int  startOfDwellTime;

   // Advance time
   time++;

   // Handle state
   switch (state) {
   case s_off:
   case s_manual:
   case s_fail:
      return;
   case s_preheat:
      // Heat from ambient to start of soak temperature
      // A -> soakTemp1 @ ramp1Slope
      if (setpoint<profile.soakTemp1) {
         setpoint += profile.ramp1Slope;
      }
      else {
         state = s_soak;
         startOfSoakTime = time;
      }
      break;
   case s_soak:
      // Heat from soak start temperature to soak end temperature over soak time
      // soakTemp1 -> soakTemp2 over soakTime time
      if (setpoint<profile.soakTemp2) {
         setpoint = profile.soakTemp1 + (time-startOfSoakTime)*(profile.soakTemp2-profile.soakTemp1)/profile.soakTime;
      }
      if (time >= (startOfSoakTime+profile.soakTime)) {
         state = s_ramp_up;
      }
      break;
   case s_ramp_up:
      // Heat from soak end temperature to peak at rampUp rate
      // soakTemp2 -> peakTemp @ ramp2Slope
      if (setpoint < profile.peakTemp) {
         setpoint += profile.ramp2Slope;
      }
      else {
         state = s_dwell;
         startOfDwellTime = time;
      }
      break;
   case s_dwell:
      // Remain at peak temperature for dwell time
      // peakTemp for peakDwell
      if (time>(startOfDwellTime+profile.peakDwell)) {
         state = s_ramp_down;
      }
      break;
   case s_ramp_down:
      // Ramp down at rampDown rate
      // peakDwell 50 @ rampDown
      if (setpoint > 50) {
         setpoint += profile.rampDownSlope;
      }
      else {
         state = s_off;
      }
      break;
   }
};

void plot(const NvSolderProfile &profile) {
   state    = s_preheat;
   time     = 0;
   setpoint = 50;
   while (state != s_off) {
      calculate(profile);
      Draw::plotPoint(time, setpoint);
   }
   lcd.setGraphicMode();
   lcd.refreshImage();
}

}; // end namespace Draw

void drawProfile(const NvSolderProfile &profile) {
   Draw::drawAxis(profile.description);
   Draw::plot(profile);
   lcd.refreshImage();
   lcd.setGraphicMode();
}

/** Profile being run */
static const NvSolderProfile *currentProfile;

/** Time in the sequence (seconds) */
static volatile int time;

/** Heater set-point */
static volatile float setpoint;

/** Profile complete flag */
static volatile bool complete;

/** Used to record ambient temperature at start (Celsius) */
static float ambient;


State state = s_off;

static const char *getStateName(State state) {
   switch(state) {
   case s_off       : return "off";
   case s_fail      : return "fail";
   case s_preheat   : return "preheat";
   case s_soak      : return "soak";
   case s_ramp_up   : return "ramp_up";
   case s_dwell     : return "dwell";
   case s_ramp_down : return "ramp_down";
   case s_manual    : return "manual";
   }
   return "invalid";
}
/*
 * Handles the call-back from the PIT to step through the sequence
 */
static void handler() {
   static int startOfSoakTime;
   static int startOfDwellTime;

   if (complete) {
      // Already completed
      return;
   }
   // Advance time
   time++;

   // Handle state
   switch (state) {
   case s_off:
   case s_manual:
   case s_fail:
      // Not operating
      pid.setSetpoint(0);
      pid.enable(false);
      ovenControl.setHeaterDutycycle(0);
      ovenControl.setFanDutycycle(0);
      return;
   case s_preheat:
      // Heat from ambient to start of soak temperature
      // A -> soakTemp1 @ ramp1Slope
      if (setpoint<currentProfile->soakTemp1) {
         setpoint += currentProfile->ramp1Slope;
      }
      pid.setSetpoint(setpoint);
      if (getTemperature()>=currentProfile->soakTemp1) {
         state = s_soak;
         startOfSoakTime = time;
      }
      break;
   case s_soak:
      // Heat from soak start temperature to soak end temperature over soak time
      // soakTemp1 -> soakTemp2 over soakTime time
      if (setpoint<currentProfile->soakTemp2) {
         setpoint = currentProfile->soakTemp1 + (time-startOfSoakTime)*(currentProfile->soakTemp2-currentProfile->soakTemp1)/currentProfile->soakTime;
      }
      pid.setSetpoint(setpoint);
      if ((time >= (startOfSoakTime+currentProfile->soakTime)) && (getTemperature()>=currentProfile->soakTemp2)) {
         state = s_ramp_up;
      }
      break;
   case s_ramp_up:
      // Heat from soak end temperature to peak at rampUp rate
      // soakTemp2 -> peakTemp @ ramp2Slope
      if (setpoint < currentProfile->peakTemp) {
         setpoint += currentProfile->ramp2Slope;
      }
      pid.setSetpoint(setpoint);
      if (getTemperature() >= currentProfile->peakTemp) {
         state = s_dwell;
         startOfDwellTime = time;
      }
      break;
   case s_dwell:
      // Remain at peak temperature for dwell time
      // peakTemp for peakDwell
      if (time>(startOfDwellTime+currentProfile->peakDwell)) {
         state = s_ramp_down;
      }
      break;
   case s_ramp_down:
      // Ramp down at rampDown rate
      // (Tp-5)-> 50 @ rampDown
      if (setpoint > ambient) {
         setpoint += currentProfile->rampDownSlope;
      }
      pid.setSetpoint(setpoint);
      if (getTemperature()<ambient) {
         state = s_off;
         complete = true;
      }
      break;
   }
};

//static const char *title = "     State,  Time, Target, Actual, Heater,  Fan, T1-probe, T1-cold, T2-probe, T2-cold, T3-probe, T3-cold, T4-probe, T4-cold,\n";
static const char *title = "\n     State,  Time, Target, Actual, Heater,  Fan, T1-probe, T2-probe, T3-probe, T4-probe,\n";

/**
 * Writes thermocouple status to LCD buffer and log
 */
static void thermocoupleStatus() {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();
   lcd.gotoXY(0,0);
   lcd.putSpace(14); lcd.putString("Status Oven  ColdJn\n");
   lcd.drawHorizontalLine(9);
   lcd.gotoXY(0, 12+4*lcd.FONT_HEIGHT);
   lcd.printf("%4ds S=%3d T=%0.1f\x7F", time, (int)round(setpoint), pid.getInput());
   printf(" %9s,  %4d,  %5.1f,  %5.1f,   %4d, %4d,", getStateName(state), time, setpoint, pid.getInput(), ovenControl.getHeaterDutycycle(), ovenControl.getFanDutycycle());
   lcd.gotoXY(0, 12);
   for (int t=0; t<=3; t++) {
      float temperature, coldReference;
      int status = temperatureSensors[t].getReading(temperature, coldReference);

      lcd.printf("T%d:", t+1); lcd.putSpace(2);
      if ((status&0x7) == 0) {
         lcd.printf("%-4s %5.1f\x7F %5.1f\x7F\n", Max31855::getStatusName(status), temperature, coldReference);
         //         printf("    %5.1f,   %5.1f,", temperature, coldReference);
         printf("    %5.1f,", temperature);
      }
      else if ((status&0x7) != 7) {
         lcd.printf("%-4s  ----  %5.1f\x7F\n", Max31855::getStatusName(status), coldReference);
         //         printf("    %5.1f,   %5.1f,", 0.0, coldReference);
         printf("    %5.1f,", 0.0);
      }
      else {
         lcd.printf("%-4s\n", Max31855::getStatusName(status));
         //         printf("    %5.1f,   %5.1f,", 0.0, 0.0);
         printf("    %5.1f,", 0.0);
      }
   }
   puts("");
}

/**
 * Reports thermocouple status to LCD and log
 */
static void thermocoupleStatus(void (*prompt)()) {
   thermocoupleStatus();
   if (state != s_off) {
      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.putString(getStateName(state));
   }
   if (prompt != nullptr) {
      prompt();
   }
   lcd.refreshImage();
   lcd.setGraphicMode();
}

/** Flag used to cause report once per second */
static bool doReport = true;

/** Dialogue prompt */
static void (*prompt)();

static bool report() {
   if (doReport) {
      thermocoupleStatus(prompt);
      doReport = false;
      time++;
   }
   return buttons.peekButton() != SW_NONE;
};

/**
 * Monitor thermocouple status
 * Also allows thermocouples to be disabled
 */
void monitor() {

   time      = 0;
   complete  = false;
   state     = s_off;

   printf(title);

   time = 0;
   prompt = []() {
      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T1");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T2");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T3");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T4");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(4); lcd.putString("Exit"); lcd.putSpace(4); lcd.setInversion(false);
   };

   // Report every second
   do {
      do {
         doReport = true;
      } while (!USBDM::wait(1.0f, report));
      switch(buttons.getButton()) {
      case SW_F1: temperatureSensors[0].enable(!temperatureSensors[0].isEnabled()); break;
      case SW_F2: temperatureSensors[1].enable(!temperatureSensors[1].isEnabled()); break;
      case SW_F3: temperatureSensors[2].enable(!temperatureSensors[2].isEnabled()); break;
      case SW_F4: temperatureSensors[3].enable(!temperatureSensors[3].isEnabled()); break;
      case SW_S:
         return;
      default:
         break;
      }
   } while (true);
}

/**
 * Run profile
 *
 * @param profile The profile to run
 */
void run(const NvSolderProfile &profile) {

   if (!checkThermocouples()) {
      return;
   }

   char buff[100];
   snprintf(buff, sizeof(buff), "%s\n\nRun Profile?", (const char *)profile.description);
   MessageBoxResult rc = messageBox("Run Profile", buff, MSG_YES_NO);
   if (rc != MSG_IS_YES) {
      return;
   }

   state          = s_preheat;
   time           = 0;
   complete       = false;
   currentProfile = &profile;

   if (isnan(getTemperature())) {
      messageBox("Error", "No thermocouples");
      return;
   }
   printf("\nProfile\n");
   currentProfile->print();

   /*
    * Obtain ambient temperature as reference
    */
   ambient = getTemperature();
   printf("Ambient, %5.1f\n", ambient);
   printf(title);

   if (isnan(ambient)) {
      // Dummy value for testing without probes
      ambient = 25.0f;
   }
   setpoint = ambient;
   pid.setSetpoint(ambient);
   pid.enable();

   // Using PIT
   USBDM::Pit::enable();
   USBDM::Pit::setCallback(profile_pit_channel, handler);
   USBDM::Pit::setPeriod(profile_pit_channel, 1.0f);
   USBDM::Pit::enableChannel(profile_pit_channel);
   USBDM::Pit::enableInterrupts(profile_pit_channel);

   prompt = []() {
      lcd.gotoXY(128-4-lcd.FONT_WIDTH*7, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putString(" Abort "); lcd.setInversion(false);
   };
   int lastTime = -2;
   // Wait for completion
   do {
      if (time>lastTime)  {
         // Report every second
         thermocoupleStatus(prompt);
         lastTime = time;
      }
      if (complete) {
         break;
      }
   } while (buttons.getButton() != SW_S);

   pid.setSetpoint(0);
   pid.enable(false);
   ovenControl.setHeaterDutycycle(0);
   USBDM::Pit::enableInterrupts(profile_pit_channel, false);
   USBDM::Pit::enableChannel(profile_pit_channel, false);
   USBDM::Pit::setCallback(profile_pit_channel, nullptr);

   ovenControl.setFanDutycycle(100);

   // Sound buzzer
   Buzzer::play();

   prompt = []() {
      lcd.gotoXY(128-4-lcd.FONT_WIDTH*17, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putString(complete?" Complete - Exit ":" Aborted  - Exit "); lcd.setInversion(false);
   };

   state = s_off;

   // Report every second until key-press
   do {
      doReport = true;
   } while (!USBDM::wait(1.0f, report));

   // Discard key
   buttons.getButton();
   ovenControl.setFanDutycycle(0);
}

static void logger() {
   time ++;
   printf(" %9s,  %4d,  %5.1f,  %5.1f,   %4d, %4d,", getStateName(state), time, pid.getSetpoint(), pid.getInput(), ovenControl.getHeaterDutycycle(), ovenControl.getFanDutycycle());
   for (int t=0; t<=3; t++) {
      float temperature, coldReference;
      int status = temperatureSensors[t].getReading(temperature, coldReference);

      if (status == 0) {
         printf("    %5.1f,", temperature);
      }
      else if (status != 7) {
         printf("    %5.1f,", 0.0);
      }
      else {
         printf("    %5.1f,", 0.0);
      }
   }
   puts("");
}

/**
 * Manually control oven
 */
void manualMode() {

   if (!checkThermocouples()) {
      return;
   }

   int fanSpeed = 100;

   state= s_off;

   printf(title);

   time = 0;

   // Using PIT
   USBDM::Pit::enable();
   USBDM::Pit::setCallback(profile_pit_channel, logger);
   USBDM::Pit::setPeriod(profile_pit_channel, 1.0f);
   USBDM::Pit::enableChannel(profile_pit_channel);
   USBDM::Pit::enableInterrupts(profile_pit_channel);

   pid.setSetpoint(100);
   pid.enable(false);
   for(;;) {
      lcd.clearFrameBuffer();

      lcd.setInversion(true); lcd.putString("  Manual Mode\n"); lcd.setInversion(false);

      lcd.printf("On Time   = %5.1fs\n", pid.getElapsedTime());

      lcd.printf("Set Temp  = %5.1f\x7F\n", pid.getSetpoint());

      lcd.printf("Oven Temp = %5.1f\x7F\n", getTemperature());

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
      lcd.gotoXY(4+7*lcd.FONT_WIDTH+21, lcd.LCD_HEIGHT-2*lcd.FONT_HEIGHT);
      if (state == s_manual) {
         lcd.setInversion(true); lcd.putSpace(2); lcd.putString("Temp");  lcd.putSpace(1);
      }
      else {
         lcd.setInversion(true); lcd.putSpace(5); lcd.putString("Fan");  lcd.putSpace(4);
      }
      if (state != s_manual) {
         lcd.gotoXY(4, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
         lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Fan");  lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      }
      lcd.gotoXY(4+3*lcd.FONT_WIDTH+12, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Heat"); lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("+");    lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("-");    lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Exit"); lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);

      lcd.refreshImage();
      lcd.setGraphicMode();

      /**
       * Safety check
       * Turn off after 800 seconds of operation
       */
      if (pid.getElapsedTime()>=maxHeaterTime) {
         pid.enable(false);
         ovenControl.setHeaterDutycycle(0);
      }
      switch (buttons.getButton()) {
      case SW_F1:
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
      case SW_F2:
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
      case SW_F3:
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
      case SW_F4:
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
      case SW_S:
         // Exit
         USBDM::Pit::enableInterrupts(profile_pit_channel, false);
         USBDM::Pit::enableChannel(profile_pit_channel, false);
         USBDM::Pit::setCallback(profile_pit_channel, nullptr);

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
