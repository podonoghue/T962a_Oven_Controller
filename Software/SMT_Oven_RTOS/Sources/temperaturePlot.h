/**
 * @file TemperaturePlot.h
 *
 *  Created on: 16Mar.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_TEMPERATUREPLOT_H_
#define SOURCES_TEMPERATUREPLOT_H_

#include <math.h>
#include <string.h>
#include <dataPoint.h>
#include <Max31855.h>
#include <algorithm>    // std::max


/**
 * Represents an entire plot of a profile and profile run
 */
class TemperaturePlot {

public:
   static constexpr int MAX_PROFILE_TIME   = 9*60; // Maximum time for profile

private:
   using ThermocoupleStatus = Max31855::ThermocoupleStatus;

   /** Value used to scale float to scaled integer values => 2 decimal places */
   static constexpr float FIXED_POINT_SCALE    = 100.0;

   DataPoint fThermocouple[MAX_PROFILE_TIME];  // Measured oven results
   uint16_t  fProfile[MAX_PROFILE_TIME];       // Profile being attempted
   int       fLastValid;                       // Index of last valid point
   int       fLastProfile;                     // Index of last profile point


public:
   TemperaturePlot() : fLastValid(0), fLastProfile(0) {
      reset();
   }
   virtual ~TemperaturePlot() {
   }

   /**
    * Clear plot points
    */
   void reset() {
      memset(fThermocouple, 0, sizeof(fThermocouple));
      memset(fThermocouple, 0, sizeof(fProfile));
      fLastValid       = -1;
      fLastProfile     = -1;
   }

public:
   /**
    * Add profile temperature point to plot
    *
    * @param time Time index for point
    * @param temp Profile target temperature for above time index
    */
   void addProfilePoint(int time, float temp) {
      if (time>=MAX_PROFILE_TIME) {
         return;
      }
      if (time>fLastProfile) {
         fLastProfile = time;
      }
      fProfile[time] = round(temp*FIXED_POINT_SCALE);
   }

   /**
    * Get profile temperature point
    *
    * @param time Time index for point
    *
    * @return Profile target temperature for above time index or NAN is out of range
    */
   float getProfilePoint(int time) {
      if (time>=fLastProfile) {
         return NAN;
      }
      return fProfile[time]/FIXED_POINT_SCALE;
   }

   /**
    * Add thermocouple points to plot
    *
    * @param time       Time index for data point
    * @param dataPoint  Data for the point
    */
   void addDataPoint(int time, DataPoint const &dataPoint) {
      if (time>=MAX_PROFILE_TIME) {
         return;
      }
      if (time>fLastValid) {
         fLastValid = time;
      }
      fThermocouple[time] = dataPoint;
   }

   /**
    * Return data point
    *
    * @param index Index of point to retrieve
    *
    * @return Point retrieved.
    */
   const DataPoint &getDataPoint(int index) const {
      static const DataPoint dummy;
      if (index>fLastValid) {
         return dummy;
      }
      return fThermocouple[index];
   }

   /**
    * Indicates if the plot contains oven data
    *
    * @return true  Oven data present
    * @return false No oven data
    */
   bool isLiveDataPresent() const {
      return fLastValid>=0;
   }

   /**
    * Get index of last temperature point
    *
    * @return Index as integer
    */
   int getLastValid() const {
      return fLastValid;
   }

   /**
    * Get index of last profile value
    *
    * @return Index as integer
    */
   int getLastProfile() const {
      return fLastProfile;
   }

   /**
    * Get index of last profile or temperature point
    *
    * @return Index as integer
    */
   int getLastIndex() const {
      return std::max(fLastProfile, fLastValid);
   }

   /**
    * Get data points
    *
    * @return Data points
    */
   const DataPoint *getData() const {
      return fThermocouple;
   }

   /**
    * Get data points
    *
    * @param index Index of value to retrieve
    *
    * @return Data points
    */
   const DataPoint &getDataPoint(unsigned index) const {
      return fThermocouple[index];
   }
};

#endif /* SOURCES_TEMPERATUREPLOT_H_ */
