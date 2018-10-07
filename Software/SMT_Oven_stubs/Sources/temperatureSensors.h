/**
 * @file TemperatureSensors.h
 *
 *  Created on: 12Mar.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_TEMPERATURESENSORS_H_
#define SOURCES_TEMPERATURESENSORS_H_

#include <dataPoint.h>
#include <Max31855.h>
#include "cmsis.h"

class TemperatureSensors {

public:
   static constexpr unsigned NUM_THERMOCOUPLES = 4;

private:
   using ThermocoupleStatus = Max31855::ThermocoupleStatus;

   /** Temperature sensors */
   Max31855 fTemperatureSensors[NUM_THERMOCOUPLES] = {
      Max31855(spi, t1_cs, t1Offset, t1Enable),
      Max31855(spi, t2_cs, t2Offset, t2Enable),
      Max31855(spi, t3_cs, t3Offset, t3Enable),
      Max31855(spi, t4_cs, t4Offset, t4Enable),
   };

   /** The thermocouples are averaged this many times on reading. */
   static constexpr int OVERSAMPLES = 4;

   /** Last measurement */
   DataPoint fCurrentMeasurements;

   /** Cold junction references from last measurement */
   float fColdReferences[4];

   /** Mutex used to protect accesses */
   CMSIS::Mutex fMutex;

   /** Average of temperatures */
   float fAverageTemperature = 0;

public:
   /**
    * Constructor
    */
   TemperatureSensors() {
   }

   /**
    * Destructor
    */
   virtual ~TemperatureSensors() {}

   /**
    * Update current readings from thermocouples
    */
   void updateMeasurements() {
      // Lock while changes made
      fMutex.wait();
      float temperatures[NUM_THERMOCOUPLES];
      ThermocoupleStatus status[NUM_THERMOCOUPLES];
      int   foundSensorCount   = 0;
      float averageTemperature = 0;
      for (unsigned t=0; t<NUM_THERMOCOUPLES; t++) {
         // Average each thermocouple
         temperatures[t]    = 0;
         fColdReferences[t] = 0;
         for (int overSample=0; overSample<OVERSAMPLES; overSample++) {
            float temperature;
            float coldReference;
            status[t] = fTemperatureSensors[t].getReading(temperature, coldReference);
            temperatures[t]   += temperature;
            fColdReferences[t] += coldReference;
            if (status[t] == Max31855::TH_ENABLED) {
               foundSensorCount++;
               averageTemperature +=temperature;
            }
         }
         // Scale for average
         temperatures[t]    /= OVERSAMPLES;
         fColdReferences[t]  /= OVERSAMPLES;
      }
      if (foundSensorCount==0) {
         // Safe value to return!
         averageTemperature = NAN;
      }
      else {
         averageTemperature /= foundSensorCount;
      }
      fAverageTemperature = averageTemperature;
      fCurrentMeasurements.setState(s_off);
      fCurrentMeasurements.setTargetTemperature(0);
      fCurrentMeasurements.setFan(0);
      fCurrentMeasurements.setHeater(0);
      fCurrentMeasurements.setThermocouplePoint(temperatures, status);
      fMutex.release();
   }
   /**
    * Get current temperature\n
    * This is an average of the active thermocouples\n
    * This does a new set of measurements
    *
    * @return Averaged oven temperature
    */
   float getTemperature() {
      updateMeasurements();
      return fAverageTemperature;
   }
   /**
    * Get last measured thermocouple values
    *
    * @return Reference to set of measurements (DataPoint)
    * @return This will be incomplete as only the thermocouple information is present e.g.
    *         state etc is not valid.
    */
   const DataPoint &getLastMeasurement() {
      return fCurrentMeasurements;
   }
   /**
    * Return the cold reference temperature from the last call
    * to measureNewDataPoint for given thermocouple
    *
    * @param[in] index Index of thermocouple
    *
    * @return Cold reference temperature
    */
   float getColdReferences(int index) {
      return fColdReferences[index];
   }
   /**
    * Get the thermocouple sensor
    *
    * @param[in]  index Index of sensor to retrieve
    *
    * @return Reference to sensor
    */
   Max31855 &getThermocouple(int index) {
      return fTemperatureSensors[index];
   }
   /**
    * Get case temperature \n
    * This is actually the cold reference temperature for one of the internal Max31855s
    */
   float getCaseTemperature() {
      float temperature, coldReference;
      ThermocoupleStatus status = fTemperatureSensors[0].getReading(temperature, coldReference);
      if (status == Max31855::TH_MISSING) {
         // No MAX31855!
         return 50.0;
      }
      return coldReference;
   }
};

#endif /* SOURCES_TEMPERATURESENSORS_H_ */
