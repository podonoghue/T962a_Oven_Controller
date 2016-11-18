/*
 * runProfile.cpp
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */
#include <math.h>
#include <solderProfiles.h>
#include "hardware.h"
#include "configure.h"
#include "messageBox.h"
#include "editProfile.h"

using namespace USBDM;
using namespace std;

namespace RunProfile {
/**
 * Functions associated with drawing profiles and related
 */
namespace Draw {

// Origin for plot
static constexpr int X_ORIGIN   = 16; // Pixels from left edge
static constexpr int Y_ORIGIN   = 7;  // Pixels from bottom edge

// Location of profile name
static constexpr int NAME_OFFSET_X = 17; // Pixels from left
static constexpr int NAME_OFFSET_Y = 0;  // Pixels from top

static constexpr int MIN_TEMP   = 50;   // Minimum temperature to plot (C)
static constexpr int MAX_TEMP   = 305;  // Maximum temperature to plot  (C)
static constexpr int MAX_TIME   = 9*60; // Maximum time to plot (s)
static constexpr int GRID_TIME  = 60;   // Time grid spacing (s) must be 60*N
static constexpr int GRID_TEMP  = 50;   // Temperature grid spacing (C)

// These stop the plot resizing when too small
static constexpr int MIN_SCALE_TEMP  = 150;   // Minimum temperature for scaling (C)
static constexpr int MIN_SCALE_TIME  = 200;   // Minimum time for scaling (s)

/** Points to plot */
static uint16_t points[MAX_TIME];

/** Maximum time to plot (# of points in points) */
int maxTime     = 0;

/** Maximum temperature to plot */
int maxTemperature    = 0;
/**
 *  Calculated time scale (from points[])
 *  Time -> pixel scaling (s/pixel)
 */
float timeScale        = 4;
/**
 *  Calculated temperature scale (from points[])
 *  Temperature -> pixel scaling (C/pixel)
 */
float temperatureScale = 4;
/**
 * Determines the plot scaling from the points array
 */
static void calculateScales() {
   // Don't scale below this
   maxTemperature = MIN_SCALE_TEMP;
   for (int time=0; time<maxTime; time++) {
      //      printf("points[%d]=%d, maxTemperature=%d\n",
      //            time, points[time], maxTemperature);
      if (points[time]>maxTemperature) {
         maxTemperature = points[time];
      }
   }
   temperatureScale = (maxTemperature-MIN_TEMP)/(float)(lcd.LCD_HEIGHT-lcd.FONT_HEIGHT-10);
   timeScale        = ((maxTime<MIN_SCALE_TIME)?MIN_SCALE_TIME:maxTime)/(float)(lcd.LCD_WIDTH-12-24);
   //   printf("temperatureScale=%f, timeScale=%f\n",
   //         temperatureScale, timeScale);
}
/**
 * Clears the plot
 */
static void reset() {
   maxTime = 0;
   memset(points, 0, sizeof(points));
   calculateScales();
}
/**
 * Adds a point to the plot array
 *
 * @param time        Time in seconds [0..MAX_TIME]
 * @param temperature Temperature in Celsius
 */
static void addPoint(int time, int temperature) {
   //   printf("time=%d, temperature=%d, maxTime=%d\n",
   //         time, temperature, maxTime);
   if (time>MAX_TIME) {
      return;
   }
   if (time>maxTime) {
      maxTime = time;
   }
   points[time] = temperature;
}
/**
 * Plot temperature point on screen.\n
 * Temperatures [MIN_TEMP..MAX_TEMP] C\n
 * Time [0s..MAX_TIME] s
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
   int x = (int)(X_ORIGIN+round(time/timeScale));
   int y = (int)round(lcd.LCD_HEIGHT-Y_ORIGIN-round((temperature-MIN_TEMP)/temperatureScale));
   lcd.drawPixel(x,y);
}
/**
 * Plot points array on screen
 */
static void drawPoints() {
   for (int time=0; time<maxTime; time++) {
      plotPoint(time, points[time]);
   }
}
/**
 * Draw the axis for the plot
 *
 * @param profileIndex Index of profile (to get name of profile to print)
 */
static void drawAxis(int profileIndex) {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   // Horizontal axis minute axis ticks
   lcd.drawHorizontalLine(lcd.LCD_HEIGHT-Y_ORIGIN);
   for (int time=60; time<=MAX_TIME; time+=GRID_TIME) {
      lcd.gotoXY((X_ORIGIN+round(time/timeScale)-3), lcd.LCD_HEIGHT-5);
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
      lcd.gotoXY(0, lcd.LCD_HEIGHT-Y_ORIGIN-round((temp-MIN_TEMP)/temperatureScale)-2);
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
   // Name
   lcd.gotoXY(NAME_OFFSET_X, NAME_OFFSET_Y);
   lcd.setInversion(true);

   lcd.printf("%d:%s", profileIndex, (const volatile char *)(profiles[profileIndex].description));
   lcd.putChar('\n');
   lcd.setInversion(false);
}

static void putProfileMenu() {
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
   lcd.putSpace(1); lcd.putString("F4"); lcd.putSpace(2); lcd.putString("C"); lcd.putSpace(1);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*4);
   lcd.putSpace(1); lcd.putString("S "); lcd.putEnter(); lcd.putSpace(2);
   lcd.setInversion(false);
}

/** State when drawing profile */
static State state;

/** Time when drawing profile */
static int   time;

/** Set-point when drawing profile */
static float setpoint;

/*
 * Calculates one step of the profile
 *
 * @param profile Profile to use
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
   case s_complete:
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
/**
 * Plot the profile to the points buffer
 *
 * @param profileIndex Index of profile to use
 */
static void plot(int profileIndex) {
   const NvSolderProfile &profile = profiles[profileIndex];

   state    = s_preheat;
   time     = 0;
   setpoint = 50;
   while (state != s_off) {
      calculate(profile);
      addPoint(time, setpoint);
   }
}

}; // end namespace Draw

namespace CopyProfile {
static unsigned sourceProfileIndex;
static unsigned destinationProfileIndex;

void draw() {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   lcd.gotoXY(10,0);
   lcd.setInversion(true); lcd.putString(" Copy Profile "); lcd.setInversion(false);
   lcd.gotoXY(0,1*lcd.FONT_HEIGHT+5);
   lcd.setInversion(false);lcd.putString("Copy:");     lcd.setInversion(false);
   lcd.gotoXY(0,2*lcd.FONT_HEIGHT+5);
   lcd.printf("%d:%s", sourceProfileIndex, (const volatile char *)profiles[sourceProfileIndex].description);
   lcd.gotoXY(0,4*lcd.FONT_HEIGHT);
   lcd.setInversion(false);lcd.putString("To:");         lcd.setInversion(false);
   lcd.gotoXY(0,5*lcd.FONT_HEIGHT);
   lcd.printf("%d:%s", destinationProfileIndex, (const volatile char *)profiles[destinationProfileIndex].description);

   lcd.gotoXY(8,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putUpArrow();        lcd.putSpace(4); lcd.setInversion(false); lcd.putSpace(6);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putDownArrow();      lcd.putSpace(4); lcd.setInversion(false); lcd.putSpace(6);
   lcd.gotoXY(lcd.LCD_WIDTH-6*lcd.FONT_WIDTH-22,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   if ((destinationProfileIndex != sourceProfileIndex) && (profiles[destinationProfileIndex].flags&P_UNLOCKED)) {
      lcd.setInversion(true); lcd.putSpace(4); lcd.putString("OK");     lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
   }
   lcd.gotoXY(lcd.LCD_WIDTH-4*lcd.FONT_WIDTH-11,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putString("EXIT"); lcd.putSpace(3); lcd.setInversion(false);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

/**
 * Copy a profile with confirmation dialogue
 *
 * @return true  => Profile copied
 * @return false => Profile not copied (illegal/cancelled)
 */
bool copyProfile(unsigned sourceIndex, unsigned destinationIndex) {
   MessageBoxResult rc;
   char buff[100];
   if ((destinationProfileIndex == sourceProfileIndex) || !(profiles[destinationProfileIndex].flags&P_UNLOCKED)) {
      // Illegal copy - quietly ignore
      return false;
   }

   snprintf(buff, sizeof(buff), "Overwrite:\n%d:%s", destinationIndex, (const volatile char *)profiles[destinationIndex].description );
   rc = messageBox("Overwrite Profile", buff, MSG_YES_NO);
   if (rc == MSG_IS_YES) {
      // Update profile in NV ram
      profiles[destinationIndex] = profiles[sourceIndex];
      profiles[destinationIndex].flags = profiles[destinationIndex].flags | P_UNLOCKED;
      return true;
   }
   return false;
}

void run(int index) {
   sourceProfileIndex      = index;
   destinationProfileIndex = 4; // 1st writable profile

   bool needsUpdate = true;

   do {
      if (needsUpdate) {
         draw();
         needsUpdate = false;
      }
      switch(buttons.getButton()) {
      case SW_F1:
         if (destinationProfileIndex>0) {
            destinationProfileIndex--;
            needsUpdate = true;
         }
         break;
      case SW_F2:
         if ((destinationProfileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
            destinationProfileIndex++;
            needsUpdate = true;
         }
         break;
      case SW_F4:
         if (copyProfile(sourceProfileIndex, destinationProfileIndex)) {
            return;
         }
         needsUpdate = true;
         break;
      case SW_S:
         return;
      default:
         break;
      }
   } while(true);
}
}

/**
 * Draw a profile to LCD
 *
 * @param index Index of profile to draw
 */
void drawProfile(int index) {
   Draw::reset();
   Draw::plot(index);
   Draw::calculateScales();
   Draw::drawAxis(index);
   Draw::putProfileMenu();
   Draw::drawPoints();
   lcd.refreshImage();
   lcd.setGraphicMode();
}
/**
 * Display profiles for selection or editing
 *
 * On exit the current profile may be changed
 */
void profileMenu() {
   unsigned profileIndex = ::profileIndex;
   bool needUpdate = true;

   for(;;) {
      if (needUpdate) {
         RunProfile::drawProfile(profileIndex);
         lcd.refreshImage();
         lcd.setGraphicMode();
         needUpdate = false;
      }
      switch(buttons.getButton()) {
      case SW_F1:
         if (profileIndex>0) {
            profileIndex--;
            needUpdate = true;
         }
         break;
      case SW_F2:
         if ((profileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
            profileIndex++;
            needUpdate = true;
         }
         break;
      case SW_F3:
         EditProfile::run(profiles[profileIndex]);
         needUpdate = true;
         break;
      case SW_F4:
         CopyProfile::run(profileIndex);
         needUpdate = true;
         break;
      case SW_S:
         ::profileIndex.operator =(profileIndex);
         return;
      default:
         break;
      }
      __WFI();
   };
}

/** Profile being run */
static const NvSolderProfile *currentProfile;

/** Time in the sequence (seconds) */
static volatile int time;

/** Heater set-point */
static volatile float setpoint;

/** Used to record ambient temperature at start (Celsius) */
static float ambient;

static State state = s_off;

static const char *getStateName(State state) {
   switch(state) {
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
/*
 * Handles the call-back from the PIT to step through the sequence
 */
static void handler() {
   static int startOfSoakTime;
   static int startOfDwellTime;

   /** Used for timeout for profile changes */
   static volatile int timeout;

   // Advance time
   time++;

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
   case s_preheat:
      // Heat from ambient to start of soak temperature
      // A -> soakTemp1 @ ramp1Slope
      if (setpoint<currentProfile->soakTemp1) {
         setpoint += currentProfile->ramp1Slope;
         timeout = 0;
      }
      else {
         timeout++;
         if (timeout>20) {
            state = s_fail;
         }
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
         timeout = 0;
      }
      pid.setSetpoint(setpoint);
      if (time >= (startOfSoakTime+currentProfile->soakTime)) {
         if (getTemperature()>=currentProfile->soakTemp2) {
            state = s_ramp_up;
         }
         else {
            timeout++;
            if (timeout>40) {
               state = s_fail;
            }
         }
      }
      break;
   case s_ramp_up:
      // Heat from soak end temperature to peak at rampUp rate
      // soakTemp2 -> peakTemp @ ramp2Slope
      if (setpoint < currentProfile->peakTemp) {
         setpoint += currentProfile->ramp2Slope;
         timeout = 0;
      }
      pid.setSetpoint(setpoint);
      if (getTemperature() >= currentProfile->peakTemp) {
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
         state = s_complete;
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
 *
 * @param prompt Method to print the menu prompt
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

/** Flag used to limit reporting to once per second */
static bool doReport = true;

/** Dialogue prompt */
static void (*prompt)();

/**
 * Report oven state
 */
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
void runProfile(const NvSolderProfile &profile) {

   if (!checkThermocouples()) {
      return;
   }
   char buff[100];
   snprintf(buff, sizeof(buff), "%s\n\nRun Profile?", (const volatile char *)profile.description);
   MessageBoxResult rc = messageBox("Run Profile", buff, MSG_YES_NO);
   if (rc != MSG_IS_YES) {
      return;
   }
   state          = s_preheat;
   time           = 0;
   currentProfile = &profile;

   printf("\nProfile\n");
   currentProfile->print();
   /*
    * Obtain ambient temperature as reference
    */
   ambient = getTemperature();
   printf("Ambient, %5.1f\n", ambient);
   printf(title);

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
      lcd.gotoXY(lcd.LCD_WIDTH-4-lcd.FONT_WIDTH*9-5*3, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Plot"); lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Abort"); lcd.putSpace(3); lcd.setInversion(false);
   };
   int lastTime = -2;
   // Wait for completion
   do {
      if (time>lastTime)  {
         // Report every second
         thermocoupleStatus(prompt);
         lastTime = time;
      }
      if ((state == s_complete) || (state == s_fail)) {
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
   if (state != s_complete) {
      state = s_fail;
   }
   prompt = []() {
      lcd.gotoXY(128-4-lcd.FONT_WIDTH*17+2*4, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putSpace(3);
      lcd.putString((state==s_complete)?"Complete - Exit":"Failed   - Exit");
      lcd.putSpace(3); lcd.setInversion(false);
   };
   // Report every second until key-press
   do {
      doReport = true;
   } while (!USBDM::wait(1.0f, report));

   // Discard key
   buttons.getButton();
   ovenControl.setFanDutycycle(0);
   state = s_off;
}
/**
 * Logs oven status to stdout
 */
static void logger() {
   time ++;
   float temperatures[4];
   int measuredValues = 0;
   float averageTemperature = 0.0;
   for (int t=0; t<=3; t++) {
      float temperature, coldReference;
      int status = temperatureSensors[t].getReading(temperature, coldReference);
      if (status == 0) {
         // Enabled and valid measurement
         measuredValues++;
         averageTemperature += temperature;
      }
      temperatures[t] = temperature;
   }
   if (measuredValues>0) {
      averageTemperature /= measuredValues;
   }
   printf(" %9s,  %4d,  %5.1f,  %5.1f,   %4d, %4d,", getStateName(state), time, pid.getSetpoint(), averageTemperature, ovenControl.getHeaterDutycycle(), ovenControl.getFanDutycycle());
   for (int t=0; t<=3; t++) {
      printf("    %5.1f,", temperatures[t]);
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
