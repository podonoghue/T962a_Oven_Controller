/**
 * @file TemperatureSensors.h
 *
 *  Created on: 12Mar.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_TEMPERATURESENSORS_H_
#define SOURCES_TEMPERATURESENSORS_H_

#include <dataPoint.h>
#include "cmsis.h"
#include "max31855.h"

class TemperatureSensors {

public:
   using ThermocoupleStatus = Max31855::ThermocoupleStatus;

   /** Temperature sensors */
   Max31855 temperatureSensors[4] = {
      Max31855(spi, t1_cs_num, t1Offset, t1Enable),
      Max31855(spi, t2_cs_num, t2Offset, t2Enable),
      Max31855(spi, t3_cs_num, t3Offset, t3Enable),
      Max31855(spi, t4_cs_num, t4Offset, t4Enable),
   };

public:
   static constexpr unsigned NUM_THERMOCOUPLES = sizeof(temperatureSensors)/sizeof(temperatureSensors[0]);
   static constexpr int      OVERSAMPLES       = 4;

   /** Last measurement */
   DataPoint currentMeasurements;

   /** Cold junction references from last measurement */
   float coldReferences[4];

   TemperatureSensors() {}
   virtual ~TemperatureSensors() {}

   CMSIS::Mutex mutex;
   float fAverageTemperature = 0;

   void initialise() {
      for (unsigned index=0; index<(sizeof(temperatureSensors)/sizeof(temperatureSensors[0])); index++) {
         temperatureSensors[index].initialise();
      }
   }

   /**
    * Update current thermocouple readings
    */
   void updateMeasurements() {
      // Lock while changes made
      mutex.wait();
      float temperatures[NUM_THERMOCOUPLES];
      ThermocoupleStatus status[NUM_THERMOCOUPLES];
      int   foundSensorCount   = 0;
      float averageTemperature = 0;
      for (unsigned t=0; t<NUM_THERMOCOUPLES; t++) {
         temperatures[t]   = 0;
         coldReferences[t] = 0;
         for (int overSample=0; overSample<OVERSAMPLES; overSample++) {
            float temperature;
            float coldReference;
            status[t] = temperatureSensors[t].getReading(temperature, coldReference);
            temperatures[t]   += temperature;
            coldReferences[t] += coldReference;
            if (status[t] == Max31855::TH_ENABLED) {
               foundSensorCount++;
               averageTemperature +=temperature;
            }
         }
         // Scale for average
         temperatures[t]    /= OVERSAMPLES;
         coldReferences[t]  /= OVERSAMPLES;
      }
      if (foundSensorCount==0) {
         // Safe value to return!
         averageTemperature = NAN;
      }
      else {
         averageTemperature /= foundSensorCount;
      }
      fAverageTemperature = averageTemperature;
      currentMeasurements.setState(s_off);
      currentMeasurements.setTargetTemperature(0);
      currentMeasurements.setFan(0);
      currentMeasurements.setHeater(0);
      currentMeasurements.setThermocouplePoint(temperatures, status);
      mutex.release();
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
    * Get last measured themocouple values
    *
    * @return Reference to set of measurements (DataPoint)
    * @return This will be incomplete as only the thermocouple information is present e.g.
    *         state etc is not valid.
    */
   const DataPoint &getLastMeasurement() {
      return currentMeasurements;
   }
   /**
    * Return the cold reference temperature from the last call
    * to measureNewDataPoint for given thermocouple
    *
    * @param index Index of thermocouple
    *
    * @return Cold reference temperature
    */
   float getColdReferences(int index) {
      return coldReferences[index];
   }
   /**
    * Get the thermocouple sensor
    *
    * @param  index Index of sensor to retrieve
    *
    * @return Reference to sensor
    */
   Max31855 &getThermocouple(int index) {
      return temperatureSensors[index];
   }
   /**
    * Get case temperature \n
    * This is actually the cold reference temperature for one of the internal Max31855s
    */
   float getCaseTemperature() {
      float temperature, coldReference;
      ThermocoupleStatus status = temperatureSensors[0].getReading(temperature, coldReference);
      if (status == Max31855::TH_MISSING) {
         // No MAX31855!
         return 50.0;
      }
      return coldReference;
   }
};

#endif /* SOURCES_TEMPERATURESENSORS_H_ */
