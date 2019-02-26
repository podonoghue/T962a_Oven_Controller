/**
 * @file    Plotting.cpp
 * @brief   Represents the information required for a plotting to LCD\n
 *          Includes the profile and measured temperatures.
 *
 *  Created on: 17Mar.,2017
 *      Author: podonoghue
 */

#include <algorithm>    // std::max
#include "configure.h"
#include "plotting.h"
#include "TemperaturePlot.h"
#include "lcd_st7920.h"
#include "configure.h"

/**
 * Functions associated with drawing profiles and related
 */
namespace Draw {

// Origin for plot
static constexpr int X_ORIGIN       = 16;    // Pixels from left edge
static constexpr int Y_ORIGIN       = 7;     // Pixels from bottom edge

// Location of profile name
static constexpr int NAME_OFFSET_X  = 17;    // Pixels from left
static constexpr int NAME_OFFSET_Y  = 0;     // Pixels from top

static constexpr int MIN_TEMP       = 50;    // Minimum temperature to plot (C)
static constexpr int MAX_TEMP       = 305;   // Maximum temperature to plot (C)
static constexpr int GRID_TIME      = 60;    // Time grid spacing (s) must be 60*N
static constexpr int GRID_TEMP      = 50;    // Temperature grid spacing (C)

// These stop the plot resizing when too small
static constexpr int MIN_SCALE_TEMP = 150;   // Minimum temperature for scaling (C)
static constexpr int MIN_SCALE_TIME = 200;   // Minimum time for scaling (s)

/**
 * Oven temperature profile and data points for plotting
 */
static TemperaturePlot temperaturePlot;

/**
 *  Calculated time scale (for temperaturePlot[])
 *  Time -> pixel scaling (s/pixel)
 */
static float timeScale        = 4;
/**
 *  Calculated temperature scale (for temperaturePlot[])
 *  Temperature -> pixel scaling (C/pixel)
 */
static float temperatureScale = 4;

/**
 * Determines the plot scaling for temperaturePlot
 */
static void calculateScales() {
   // Maximum temperature found - Don't scale below MIN_SCALE_TEMP
   int maxTemperature = MIN_SCALE_TEMP;
   for (int time=0; time<=temperaturePlot.getLastIndex(); time++) {
      float pointTemp = temperaturePlot.getDataPoint(time).maximum();
      if (pointTemp>maxTemperature) {
         maxTemperature = pointTemp;
      }
      pointTemp = temperaturePlot.getProfilePoint(time);
      if (pointTemp>maxTemperature) {
         maxTemperature = pointTemp;
      }
   }
   temperatureScale = (maxTemperature-MIN_TEMP)/(float)(lcd.LCD_HEIGHT-lcd.FONT_HEIGHT-10);
   timeScale        = std::max(temperaturePlot.getLastIndex(),MIN_SCALE_TIME)/(float)(lcd.LCD_WIDTH-12-24);
}
/**
 * Plot a temperature point into LCD buffer.
 *
 * @param[in] time        Time for horizontal axis [0s..MAX_PROFILE_TIME] s
 * @param[in] temperature Temperature to plot [MIN_TEMP..MAX_TEMP] C
 */
static void plotTemperatureOnLCD(int time, int temperature) {
   // Limit plot range
   if ((temperature<MIN_TEMP)||(temperature>MAX_TEMP)) {
      return;
   }
   if ((time<0)||(time>TemperaturePlot::MAX_PROFILE_TIME)) {
      return;
   }
   int x = (int)(X_ORIGIN+round(time/timeScale));
   int y = (int)round(lcd.LCD_HEIGHT-Y_ORIGIN-round((temperature-MIN_TEMP)/temperatureScale));
   lcd.drawPixel(x,y);
}
/**
 * Plot entire temperaturePlot into LCD buffer\n
 * This includes the profile and average measure temperatures if present.
 */
static void plotProfilePointsOnLCD() {
   for (int time=0; time<=temperaturePlot.getLastIndex(); time++) {
      plotTemperatureOnLCD(time, temperaturePlot.getProfilePoint(time));
      if(temperaturePlot.isLiveDataPresent()) {
         // TODO add x5 temperature factor for debug
         plotTemperatureOnLCD(time, temperaturePlot.getDataPoint(time).getAverageTemperature());
      }
   }
}
/**
 * Draw the axis for the plot into LCD buffer
 *
 * @param[in] profileIndex Index of profile (to get name of profile to print)
 */
static void drawAxis(int profileIndex) {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   // Horizontal axis minute axis ticks
   lcd.drawHorizontalLine(lcd.LCD_HEIGHT-Y_ORIGIN);
   for (int time=60; time<=TemperaturePlot::MAX_PROFILE_TIME; time+=GRID_TIME) {
      lcd.gotoXY((X_ORIGIN+round(time/timeScale)-3), lcd.LCD_HEIGHT-5);
      lcd.putSmallDigit(time/60);
   }
   static uint8_t min[] {
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
   for (int time=0; time<=TemperaturePlot::MAX_PROFILE_TIME; time += GRID_TIME) {
      for (int temperature=MIN_TEMP; temperature<=MAX_TEMP; temperature+=GRID_TEMP) {
         plotTemperatureOnLCD(time, temperature);
      }
   }
   // Name
   lcd.gotoXY(NAME_OFFSET_X, NAME_OFFSET_Y);
   lcd.setInversion(true);
   lcd.write(profileIndex).write(":").write((const char *)(profiles[profileIndex].description));
//   lcd.printf("%d:%s", profileIndex, (const volatile char *)(profiles[profileIndex].description));
   lcd.write('\n');
   lcd.setInversion(false);
}

/**
 * Class used to plot a Profile
 */
class ProfilePlotter {

protected:
   /** Current state of profile */
   State state;

   /** Current time in profile */
   int   time;

   /** Current set-point  */
   float setpoint;

   /*
    * Calculates one step of the profile
    *
    * @param profile Profile to use
    */
   void calculate(const NvSolderProfile &profile) {
      static int  startOfSoakTime;
      static int  startOfDwellTime;
      static constexpr float ambient = 25.0f;

      // Advance time
      time++;

      // Handle state
      switch (state) {
      case s_off:
      case s_manual:
      case s_fail:
      case s_complete:
         return;
      case s_init:
         state = s_preheat;
         /* Fall through - no break */
      case s_preheat:
         // Heat from ambient to start of soak temperature
         // A -> soakTemp1 over preheatTime
         if (setpoint<profile.soakTemp1) {
            setpoint = ambient + (time/(float)profile.preheatTime)*(profile.soakTemp1-ambient);
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
            setpoint = profile.soakTemp1 + (time-startOfSoakTime)*(profile.soakTemp2-profile.soakTemp1)/(float)profile.soakTime;
         }
         if (time >= (startOfSoakTime+profile.soakTime)) {
            state = s_ramp_up;
         }
         break;
      case s_ramp_up:
         // Heat from soak end temperature to peak at rampUp rate
         // soakTemp2 -> peakTemp @ ramp2Slope
         if (setpoint < profile.peakTemp) {
            setpoint += profile.rampUpSlope;
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
    * @param[in] profile Profile to use
    */
   ProfilePlotter(const NvSolderProfile &profile) : state(s_preheat), time(0), setpoint(25.0) {
      while (state != s_off) {
         calculate(profile);
         temperaturePlot.addProfilePoint(time, setpoint);
      }
   }
};

/**
 * Plot the entire profile to the current plot
 *
 * @param[in] profileIndex Index of profile to use
 */
static void plotProfile(int profileIndex) {
   ProfilePlotter plotter(profiles[profileIndex]);
}

/**
 * Clears the plot dataPoints
 */
void reset() {
   temperaturePlot.reset();
}

static int profileIndex = 0;

/**
 * Draw a profile to current plot data\n
 * This clears the plot data and then plots the given profile.
 *
 * @param[in] index Index of profile to draw to plot
 */
void drawProfile(int index) {
   profileIndex = index;
   Draw::reset();
   Draw::plotProfile(index);
}

/**
 * Update the LCD from plot data
 */
void update() {
   Draw::calculateScales();
   Draw::drawAxis(profileIndex);
   Draw::plotProfilePointsOnLCD();
}

/**
 * Add data point to plot
 *
 * @param[in] time Time index of point
 * @param[in] dataPoint Point to add
 */
void addDataPoint(int time, DataPoint dataPoint) {
   temperaturePlot.addDataPoint(time, dataPoint);
}

/**
 * Get data point
 *
 * @param[in] time Time index of point
 *
 * @return dataPoint for time index
 */
const DataPoint &getDataPoint(int time) {
   return  temperaturePlot.getDataPoint(time);
}

/**
 * Get reference to entire plot data
 *
 * @return TemperaturePlot
 */
TemperaturePlot &getData() {
   return  temperaturePlot;
}

}; // end namespace Draw
