/**
 * @file    runProfile.cpp
 * @brief   Runs Solder profiles
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */

#include <math.h>

#include "dataPoint.h"
#include "solderProfiles.h"
#include "hardware.h"
#include "configure.h"
#include "messageBox.h"
#include "editProfile.h"
#include "SCPIInterface.h"

using namespace USBDM;
using namespace std;

namespace RunProfile {

static constexpr int MAX_PROFILE_TIME   = 9*60; // Maximum time for profile

/**
 * Represents an entire plot of a profile and profile run
 */
struct DataPoints {

private:
   using ThermocoupleStatus = Max31855::ThermocoupleStatus;

   DataPoint data[MAX_PROFILE_TIME];   // Oven results
   int       lastValid;                // Index of last valid point
   int       marker;                   // Marker
   bool      liveDataPresent;          // Indicates if measure thermocouple data is present

public:
   /**
    * Add target temperature point to plot
    *
    * @param time Time index for data point
    * @param temp Target temperature for above time index
    */
   void addTargetPoint(int time, float temp) {
      if (time>=MAX_PROFILE_TIME) {
         return;
      }
      if (time>lastValid) {
         lastValid = time;
      }
      data[time].setTarget(temp);
   }

   /**
    * Add thermocouple temperature points to plot
    *
    * @param time    Time index for data point
    * @param temp    Target thermocouple temperatures for above time index
    * @param active  Mask indicating active thermocouples for above time index
    */
   void addThermocouplePoints(int time, float temps[], ThermocoupleStatus status[]) {
      if (time>=MAX_PROFILE_TIME) {
         return;
      }
      liveDataPresent = true;
      if (time>lastValid) {
         lastValid = time;
      }
      data[time].setThermocouplePoint(temps, status);
   }

   /**
    * Add temperature points to plot
    *
    * @param time       Time index for data point
    * @param dataPoint  Data for the point
    */
   void addThermocouplePoints(int time, DataPoint &dataPoint) {
      if (time>=MAX_PROFILE_TIME) {
         return;
      }
      liveDataPresent = true;
      if (time>lastValid) {
         lastValid = time;
      }
      //TODO - Keep old target temp??
      data[time] = dataPoint;
   }

   /**
    * Clear plot points
    */
   void reset() {
      memset(data, 0, sizeof(data));
      lastValid       = 0;
      marker          = 0;
      liveDataPresent = false;
   }

   /**
    * Return data point
    *
    * @param index Index of point to retrieve
    *
    * @return Point retrieved.
    */
   const DataPoint &getDataPoint(int index) const {
      return data[index];
   }

   /**
    * Indicates if the plot contains oven data
    *
    * @return true  Oven data present
    * @return false No oven data
    */
   bool isLiveDataPresent() const {
      return liveDataPresent;
   }

   /**
    * Get marker
    *
    * @return marker value
    */
   int getMarker() const {
      return marker;
   }

   /**
    * Set marker
    *
    * @param marker Marker value to set
    */
   void setMarker(int marker) {
      this->marker = marker;
   }

   /**
    * Get index of last value
    */
   int getLastValid() const {
      return lastValid;
   }

   /**
    * Get data points
    *
    * @return Data points
    */
   const DataPoint *getData() const {
      return data;
   }
   /**
    * Get data points
    *
    * @return Data points
    */
   const DataPoint &getDataPoint(unsigned index) const {
      return data[index];
   }
};

/** Current Oven temperature profile/plot */
DataPoints dataPoints;

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
static constexpr int GRID_TIME  = 60;   // Time grid spacing (s) must be 60*N
static constexpr int GRID_TEMP  = 50;   // Temperature grid spacing (C)

// These stop the plot resizing when too small
static constexpr int MIN_SCALE_TEMP  = 150;   // Minimum temperature for scaling (C)
static constexpr int MIN_SCALE_TIME  = 200;   // Minimum time for scaling (s)

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
   for (int time=0; time<=RunProfile::dataPoints.getLastValid(); time++) {
      float pointTemp = RunProfile::dataPoints.getDataPoint(time).maximum();
      if (pointTemp>maxTemperature) {
         maxTemperature = pointTemp;
      }
   }
   temperatureScale = (maxTemperature-MIN_TEMP)/(float)(lcd.LCD_HEIGHT-lcd.FONT_HEIGHT-10);
   timeScale        = ((RunProfile::dataPoints.getLastValid()<MIN_SCALE_TIME)?MIN_SCALE_TIME:RunProfile::dataPoints.getLastValid())/(float)(lcd.LCD_WIDTH-12-24);
}
/**
 * Clears the dataPoints
 */
static void reset() {
   RunProfile::dataPoints.reset();
   calculateScales();
}
/**
 * Plot temperature point on screen.\n
 * Temperatures [MIN_TEMP..MAX_TEMP] C\n
 * Time [0s..MAX_PROFILE_TIME] s
 *
 * @param time        Time for horizontal axis
 * @param temperature Temperature to plot
 */
static void plotTemperatureOnLCD(int time, int temperature) {
   // Limit plot range
   if ((temperature<MIN_TEMP)||(temperature>MAX_TEMP)) {
      return;
   }
   if ((time<0)||(time>MAX_PROFILE_TIME)) {
      return;
   }
   int x = (int)(X_ORIGIN+round(time/timeScale));
   int y = (int)round(lcd.LCD_HEIGHT-Y_ORIGIN-round((temperature-MIN_TEMP)/temperatureScale));
   lcd.drawPixel(x,y);
}
/**
 * Plot all data points on LCD\n
 * This includes the profile and average measure temperatures if present
 */
static void plotProfilePointsOnLCD() {
   for (int time=0; time<RunProfile::dataPoints.getLastValid(); time++) {
      plotTemperatureOnLCD(time, RunProfile::dataPoints.getDataPoint(time).getTargetTemperature());
      if(RunProfile::dataPoints.isLiveDataPresent()) {
         plotTemperatureOnLCD(time, RunProfile::dataPoints.getDataPoint(time).getAverageTemperature());
      }
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
   for (int time=60; time<=MAX_PROFILE_TIME; time+=GRID_TIME) {
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
   for (int time=0; time<=MAX_PROFILE_TIME; time += GRID_TIME) {
      for (int temperature=MIN_TEMP; temperature<=MAX_TEMP; temperature+=GRID_TEMP) {
         plotTemperatureOnLCD(time, temperature);
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

class ProfilePlotter {

protected:
   /** Current state of profile */
   State state;

   /** Current time in profile */
   int   time;

   /** Current set-point  */
   float setpoint;

   /*
    * Calculates one step of the profile\n
    * Used for plotting desired profile
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

public:
   /**
    * Plot the entire profile to the current plot
    *
    * @param profileIndex Index of profile to use
    */
   ProfilePlotter(const NvSolderProfile &profile) : state(s_preheat), time(0), setpoint(0.0) {
      while (state != s_off) {
         calculate(profile);
         RunProfile::dataPoints.addTargetPoint(time, setpoint);
      }
   }
};

/**
 * Plot the entire profile to the current plot
 *
 * @param profileIndex Index of profile to use
 */
static void plotProfile(int profileIndex) {
   ProfilePlotter plotter(profiles[profileIndex]);
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
 * @param sourceIndex      Index of source profile
 * @param destinationIndex Index of destination profile
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
      case SwitchValue::SW_F1:
         if (destinationProfileIndex>0) {
            destinationProfileIndex--;
            needsUpdate = true;
         }
         break;
      case SwitchValue::SW_F2:
         if ((destinationProfileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
            destinationProfileIndex++;
            needsUpdate = true;
         }
         break;
      case SwitchValue::SW_F4:
         if (copyProfile(sourceProfileIndex, destinationProfileIndex)) {
            return;
         }
         needsUpdate = true;
         break;
      case SwitchValue::SW_S:
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
   Draw::plotProfile(index);
   Draw::calculateScales();
   Draw::drawAxis(index);
   Draw::putProfileMenu();
   Draw::plotProfilePointsOnLCD();
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
      case SwitchValue::SW_F1:
         if (profileIndex>0) {
            profileIndex--;
            needUpdate = true;
         }
         break;
      case SwitchValue::SW_F2:
         if ((profileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
            profileIndex++;
            needUpdate = true;
         }
         break;
      case SwitchValue::SW_F3:
         EditProfile::run(profiles[profileIndex]);
         needUpdate = true;
         break;
      case SwitchValue::SW_F4:
         CopyProfile::run(profileIndex);
         needUpdate = true;
         break;
      case SwitchValue::SW_S:
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
static void handler(const void *) {
   /* Records start of soak time */
   static int startOfSoakTime;

   /* Records start of dwell time */
   static int startOfDwellTime;

   /* Used for timeout for profile changes */
   static int timeout;

   /* Tolerance of temperature checks */
   constexpr int delta = 5;

   const float currentTemperature = temperatureSensors.getTemperature();

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
      /*
       * Heat from ambient to start of soak temperature
       * Will wait until reached T~soakTemp1 with 50s timeout
       *
       * A -> soakTemp1 @ ramp1Slope
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
               ((timeout>5)&&(currentTemperature>=(currentProfile->soakTemp1-delta)))) {
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
               ((timeout>5)&&(currentTemperature>=(currentProfile->soakTemp2-delta)))) {
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
      if (currentTemperature >= (currentProfile->peakTemp-delta)) {
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
};

//static const char *title = "     State,  Time, Target, Actual, Heater,  Fan, T1-probe, T1-cold, T2-probe, T2-cold, T3-probe, T3-cold, T4-probe, T4-cold,\n";
//static const char *title = "\nState       Time Target Actual Heater  Fan T1-probe T2-probe T3-probe T4-probe\n";

/**
 * Writes thermocouple status to LCD buffer
 */
static void thermocoupleStatus() {

   lcd.setInversion(false);
   lcd.clearFrameBuffer();
   lcd.gotoXY(0,0);
   lcd.putSpace(14); lcd.putString("Status Oven  ColdJn\n");
   lcd.drawHorizontalLine(9);
   lcd.gotoXY(0, 12+4*lcd.FONT_HEIGHT);
   lcd.printf("%4ds S=%3d T=%0.1f\x7F", time, (int)round(setpoint), pid.getInput());
   lcd.gotoXY(0, 12);

   DataPoint dataPoint;
   // Capture temperatures
   dataPoint = temperatureSensors.getCurrentDataPoint();
   dataPoint.setState(state);
   dataPoint.setTarget(pid.getSetpoint());
   dataPoint.setHeater(ovenControl.getHeaterDutycycle());
   dataPoint.setFan(ovenControl.getFanDutycycle());
   RunProfile::dataPoints.addThermocouplePoints(time, dataPoint);

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
}

/**
 * Writes thermocouple status to log
 */
static void logThermocoupleStatus(int time, const DataPoint &point) {
   char buff[120];
   sprintf(buff, "%-11s %4d  %5.1f  %5.1f   %4d %4d",
         getStateName(state), time, setpoint, pid.getInput(),
         ovenControl.getHeaterDutycycle(), ovenControl.getFanDutycycle());
   DataPoint::TemperatureArray temperatures;
   DataPoint::StatusArray      status;
   point.getThermocouplePoint(temperatures, status);
   for (unsigned t=0; t<DataPoint::NUM_THERMOCOUPLES; t++) {
      char buff2[20];
      sprintf(buff2, "    %5.1f", temperatures[t]);
      strcat(buff, buff2);
   }
   strcat(buff, "  \n\r");
   SCPI_Interface::send(buff);
}

/**
 * Reports thermocouple status to LCD and log
 *
 * @param prompt Method to print the menu prompt
 */
static void displayThermocoupleStatus(void (*prompt)()) {
   thermocoupleStatus();
   logThermocoupleStatus(time, dataPoints.getDataPoint(time));

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
      displayThermocoupleStatus(prompt);
      doReport = false;
      time++;
   }
   return buttons.peekButton() != SwitchValue::SW_NONE;
};
/**
 * Monitor thermocouple status
 * Also allows thermocouples to be disabled
 */
void monitor() {
   time      = 0;
   state     = s_off;
   prompt = []() {
      lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T1");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T2");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T3");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(3); lcd.putString("T4");   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(4);
      lcd.setInversion(true); lcd.putSpace(4); lcd.putString("Exit"); lcd.putSpace(4); lcd.setInversion(false);
   };
   // Report approximately every second
   do {
      do {
         doReport = true;
      } while (!USBDM::wait(1.0f, report));

      switch(buttons.getButton()) {
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
/**
 * Run the current profile
 */
void runProfile() {

   CMSIS::Timer<osTimerPeriodic> timer{handler};

   const NvSolderProfile &profile = profiles[profileIndex];

   if (!checkThermocouples()) {
      return;
   }
   char buff[100];
   snprintf(buff, sizeof(buff), "%d:%s\n\nRun Profile?", (int)profileIndex, (const volatile char *)profile.description);
   MessageBoxResult rc = messageBox("Run Profile", buff, MSG_YES_NO);
   if (rc != MSG_IS_YES) {
      return;
   }
   state          = s_preheat;
   time           = 0;
   currentProfile = &profile;

   currentProfile->print();
   /*
    * Obtain ambient temperature as reference
    */
   ambient = temperatureSensors.getTemperature();

   setpoint = ambient;
   pid.setSetpoint(ambient);
   pid.enable();

   // Start Timer callback
   timer.create();
   timer.start(1.0f);

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
         displayThermocoupleStatus(prompt);
         lastTime = time;
      }
      if ((state == s_complete) || (state == s_fail)) {
         break;
      }
   } while (buttons.getButton() != SwitchValue::SW_S);

   pid.setSetpoint(0);
   pid.enable(false);
   ovenControl.setHeaterDutycycle(0);

   timer.stop();
   timer.destroy();
   //   USBDM::Pit::enableInterrupts(profile_pit_channel, false);
   //   USBDM::Pit::enableChannel(profile_pit_channel, false);
   //   USBDM::Pit::setCallback(profile_pit_channel, nullptr);

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
static void logger(const void *) {
   DataPoint dataPoint;
   dataPoint = temperatureSensors.getCurrentDataPoint();
   dataPoint.setState(state);
   dataPoint.setTarget(pid.getSetpoint());
   dataPoint.setHeater(ovenControl.getHeaterDutycycle());
   dataPoint.setFan(ovenControl.getFanDutycycle());
   time++;
   RunProfile::logThermocoupleStatus(time, dataPoint);
}

/**
 * Manually control oven
 */
void manualMode() {

   if (!checkThermocouples()) {
      return;
   }

   int fanSpeed = 100;

   state = s_off;

   time = 0;

   // Timer callback used to log thermocouple values
   CMSIS::Timer<osTimerPeriodic> timer{logger};
   timer.create();
   timer.start(1.0f);

   pid.setSetpoint(100);
   pid.enable(false);
   for(;;) {
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
         timer.stop();
         timer.destroy();
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
