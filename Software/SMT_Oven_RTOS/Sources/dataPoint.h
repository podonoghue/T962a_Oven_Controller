/**
 * @file DataPoint.h
 *
 *  Created on: 12Mar.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_DATAPOINT_H_
#define SOURCES_DATAPOINT_H_

#include <math.h>
#include <Max31855.h>
#include <stdint.h>

enum State {
   s_off,
   s_fail,
   s_init,
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

   uint16_t fState_status;                       // Controller state and thermocouple status (encoded)
   // |15..13|12..10|9..7|6..4|3..0|
   //   Th3    Th2   Th1   Th0 State

   uint8_t  fHeater;                             // Heater duty cycle
   uint8_t  fFan;                                // Fan duty cycle
   uint16_t fTargetTemp;                         // Oven target temperature
   uint16_t fThermocouples[NUM_THERMOCOUPLES];   // Thermocouple values

public:
   /**
    * Constructor
    */
   DataPoint() : fState_status{0}, fHeater{0}, fFan{0}, fTargetTemp{0}, fThermocouples{0,0,0,0} {
   }
   /**
    * Get recorded status of a thermocouple
    *
    * @param index Index of thermocouple
    *
    * @return Status
    */
   ThermocoupleStatus getStatus(unsigned index) const {
      return (ThermocoupleStatus)((fState_status>>(THERMO_STATUS_WIDTH*index+THERMO_STATUS_OFFSET))&THERMO_STATUS_MASK);
   }

   /**
    * Set recorded status of a thermocouple
    *
    * @param index Index of thermocouple
    *
    * @param status Status to set
    */
   void setStatus(unsigned index, ThermocoupleStatus status) {
      fState_status =
            (fState_status & ~(THERMO_STATUS_MASK<<(THERMO_STATUS_WIDTH*index+THERMO_STATUS_OFFSET))) |
            ((status&THERMO_STATUS_MASK)<<(THERMO_STATUS_WIDTH*index+THERMO_STATUS_OFFSET));
   }

   /**
    * Get recorded thermocouple temperature
    *
    * @param index         Index of thermocouple
    * @param temperature   Temperature value
    *
    * @return Status of thermocouple
    */
   ThermocoupleStatus getTemperature(unsigned index, float &temperature) const {
      temperature = fThermocouples[index]/FIXED_POINT_SCALE;
      return getStatus(index);
   }

   /**
    * Set recorded thermocouple temperature
    *
    * @param index       Index of thermocouple
    * @param temperature Temperature to set
    */
   void setTemperature(unsigned index, float temperature) {
      fThermocouples[index] = round(temperature*FIXED_POINT_SCALE);
   }

   /**
    * Calculates the average oven temperature from active recorded thermocouples
    *
    * @return Average value as float or NAN if no thermocouples active
    */
   float getAverageTemperature() const {
      float average   = 0;
      int   numTemps = 0;
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         if (getStatus(index) == Max31855::TH_ENABLED) {
            average += fThermocouples[index];
            numTemps++;
         }
      }
      if (numTemps == 0) {
         return NAN;
      }
      return (average/FIXED_POINT_SCALE)/numTemps;
   }

   /**
    * Determine the maximum of recorded thermocouples and recorded target temperature.\n
    * Used for scaling
    *
    * @return Maximum value as float
    */
   float maximum() const {
      float max = fTargetTemp;
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         if (fThermocouples[index]>max) {
            max = fThermocouples[index];
         }
      }
      return max/FIXED_POINT_SCALE;
   }
   /**
    * Records a set of thermocouple values
    *
    * @param thermoCouple  Thermocouple temperature values to record
    * @param status        Status of thermocouple.
    */
   void setThermocouplePoint(TemperatureArray thermoCouple, StatusArray status) {
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         setTemperature(index, thermoCouple[index]);
         setStatus(index, status[index]);
      }
   }
   /**
    * Get a set of recorded thermocouple values
    *
    * @param[out] thermoCouples   Thermocouple values
    * @param[out] statuses        Statuses of thermocouples.
    */
   void getThermocouplePoint(TemperatureArray &thermoCouples, StatusArray &statuses) const {
      for (unsigned index=0; index<NUM_THERMOCOUPLES; index++) {
         thermoCouples[index]   = fThermocouples[index]/FIXED_POINT_SCALE;
         statuses[index] = getStatus(index);
      }
   }
   /**
    * Get recorded target temperature
    *
    * @return Target temperature in Celsius
    */
   float getTargetTemperature() const {
      return (fTargetTemp/FIXED_POINT_SCALE);
   }
   /**
    * Record target temperature
    *
    * @param temp Temperature to set
    */
   void setTargetTemperature(float temp) {
      fTargetTemp = round(temp * FIXED_POINT_SCALE);
   }
   /**
    * Get recorded state
    *
    * @return state e.g. s_soak
    */
   State getState() const {
      return (State)(fState_status&STATE_MASK);
   }
   /**
    * Recorded state
    *
    * @param state State e.g. s_soak
    */
   void setState(State state) {
      fState_status = (fState_status&~STATE_MASK)|(state&STATE_MASK);
   }
   /**
    * Get recorded heater value
    *
    * @return Heater value in percent
    */
   uint8_t getHeater() const {
      return fHeater;
   }
   /**
    * Record heater value
    *
    * @param percent Value for heater in percentage
    */
   void setHeater(uint8_t percent) {
      fHeater = percent;
   }
   /**
    * Get recorded fan value
    *
    * @return Fan value in percent
    */
   uint8_t getFan() const {
      return fFan;
   }
   /**
    * Record fan value
    *
    * @param percent Value for heater in percentage
    */
   void setFan(uint8_t percent) {
      fFan = percent;
   }
};

#endif /* SOURCES_DATAPOINT_H_ */
