/*
 * DataPoint.h
 *
 *  Created on: 12Mar.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_DATAPOINT_H_
#define SOURCES_DATAPOINT_H_

#include <math.h>
#include <stdint.h>
#include "max31855.h"

enum State {
   s_off,
   s_fail,
   s_preheat,
   s_soak,
   s_ramp_up,
   s_dwell,
   s_ramp_down,
   s_complete,
   s_manual,
};

/**
 * Represents a data point for plotting etc.
 */
class DataPoint {

public:
   using ThermocoupleStatus = Max31855::ThermocoupleStatus;
   static constexpr unsigned NUM_THERMOCOUPLES = 4;
   using TemperatureArray = float[NUM_THERMOCOUPLES];
   using StatusArray      = ThermocoupleStatus[NUM_THERMOCOUPLES];

private:
   /** Value used to scale float to scaled integer values => 2 decimal places */
   static constexpr float FIXED_POINT_SCALE    = 100.0;
   static constexpr int   THERMO_STATUS_OFFSET = 4;
   static constexpr int   THERMO_STATUS_WIDTH  = 3;
   static constexpr int   THERMO_STATUS_MASK   = (1<<3)-1;
   static constexpr int   STATE_MASK           = 0xF;

   uint16_t state_status;                       // Controller state and thermocouple status (encoded)
   // |15..13|12..10|9..7|6..4|3..0|
   //   Th3    Th2   Th1   Th0 State

//   State    state:8;                            // State for this point
//   uint8_t  activeThermocouples;                // Mask indicating active thermocouples
   uint8_t  heater;                             // Heater duty cycle
   uint8_t  fan;                                // Fan duty cycle
   uint16_t target;                             // Oven target temperature
   uint16_t thermocouples[NUM_THERMOCOUPLES];   // Thermocouple values

public:
   /**
    * Get status of a thermocouple
    *
    * @param index Index of thermocouple
    *
    * @return Status
    */
   ThermocoupleStatus getStatus(unsigned index) const {
      return (ThermocoupleStatus)((state_status>>(THERMO_STATUS_WIDTH*index+THERMO_STATUS_OFFSET))&THERMO_STATUS_MASK);
   }

   /**
    * Set status of a thermocouple
    *
    * @param index Index of thermocouple
    *
    * @param status Status to set
    */
   void setStatus(unsigned index, ThermocoupleStatus status) {
      state_status =
            (state_status & ~(THERMO_STATUS_MASK<<(THERMO_STATUS_WIDTH*index+THERMO_STATUS_OFFSET))) |
            ((status&THERMO_STATUS_MASK)<<(THERMO_STATUS_WIDTH*index+THERMO_STATUS_OFFSET));
   }

   /**
    * Get thermocouple temperature
    *
    * @param index         Index of thermocouple
    * @param temperature   Temperature value
    *
    * @return Status of thermocouple
    */
   ThermocoupleStatus getTemperature(unsigned index, float &temperature) const {
      temperature = thermocouples[index]/FIXED_POINT_SCALE;
      return getStatus(index);
   }

   /**
    * Set thermocouple temperature
    *
    * @param index       Index of thermocouple
    * @param temperature Temperature to set
    */
   void setTemperature(unsigned index, float temperature) {
      thermocouples[index] = round(temperature*FIXED_POINT_SCALE);
   }

   /**
    * Calculates the average oven temperature from active thermocouples
    *
    * @return Average value as float or NAN if no thermocouples active
    */
   float getAverageTemperature() const {
      float average   = 0;
      int   numTemps = 0;
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         if (getStatus(index) == Max31855::TH_ENABLED) {
            average += thermocouples[index];
            numTemps++;
         }
      }
      if (numTemps == 0) {
         return NAN;
      }
      return (average/FIXED_POINT_SCALE)/numTemps;
   }

   /**
    * Determine the maximum of thermocouples and target temperature.\n
    * Used for scaling
    *
    * @return Maximum value as float
    */
   float maximum() const {
      float max = target;
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         if (thermocouples[index]>max) {
            max = thermocouples[index];
         }
      }
      return max/FIXED_POINT_SCALE;
   }
   /**
    * Adds a set of thermocouple values
    *
    * @param temp   Thermocouple values to add
    * @param active Bit fields indicating active thermocouples.
    */
   void setThermocouplePoint(TemperatureArray temp, StatusArray status) {
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         setTemperature(index, temp[index]);
         setStatus(index, status[index]);
      }
   }
   /**
    * Get a set of thermocouple values
    *
    * @param[out] temp   Thermocouple values
    * @param[out] active Mask indicating active thermocouples.
    */
   void getThermocouplePoint(TemperatureArray &temp, StatusArray &status) const {
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         temp[index]   = thermocouples[index]/FIXED_POINT_SCALE;
         status[index] = getStatus(index);
      }
   }
   /**
    * Set target temperature
    *
    * @param temp Temperature to set
    */
   void setTarget(float temp) {
      target = round(temp * FIXED_POINT_SCALE);
   }
   /**
    * Get target temperature
    *
    * @param temp Temperature to set
    */
   float getTargetTemperature() const {
      return (target/FIXED_POINT_SCALE);
   }

   /**
    * Get state
    *
    * @return state e.g. s_soak
    */
   State getState() const {
      return (State)(state_status&STATE_MASK);
   }

   /**
    * Set state
    *
    * @param state State e.g. s_soak
    */
   void setState(State state) {
      state_status = (state_status&~STATE_MASK)|(state&STATE_MASK);
   }
   /**
    * Set heater value
    *
    * @param value Value for heater in percentage
    */
   void setHeater(uint8_t percent) {
      heater = percent;
   }
   /**
    * Set fan value
    *
    * @param value Value for heater in percentage
    */
   void setFan(uint8_t percent) {
      fan = percent;
   }
};

#endif /* SOURCES_DATAPOINT_H_ */
