/*
 * TemperatureSensors.h
 *
 *  Created on: 12Mar.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_TEMPERATURESENSORS_H_
#define SOURCES_TEMPERATURESENSORS_H_

#include "max31855.h"
#include "dataPoint.h"

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

   TemperatureSensors() {}
   virtual ~TemperatureSensors() {}

   float coldReferences[4];

   /**
    * Get current thermocouple readings
    */
   DataPoint getCurrentDataPoint() {
      DataPoint dataPoint;

      dataPoint.setState(s_off);
      dataPoint.setTarget(0);
      float temperatures[NUM_THERMOCOUPLES];
      ThermocoupleStatus status[NUM_THERMOCOUPLES];
      for (unsigned t=0; t<NUM_THERMOCOUPLES; t++) {
         float temperature;
         status[t]       = temperatureSensors[t].getReading(temperature, coldReferences[t]);
         temperatures[t] = temperature;
      }
      dataPoint.setThermocouplePoint(temperatures, status);
      return dataPoint;
   }
   /**
    * Return the cold reference temperature from the last measurement on that thermocouple
    *
    * @param index Index of thermocouple
    *
    * @return Cold reference temperature
    */
   float getColdReferences(int index) {
      return coldReferences[index];
   }
   /**
    * Get current temperature\n
    * This is an average of the active thermocouples
    *
    * @return Averaged oven temperature
    */
   float getTemperature() {
      int foundSensorCount = 0;
      float value = 0;
      for (int overSample=0; overSample<OVERSAMPLES; overSample++) {
         for (unsigned t=0; t<NUM_THERMOCOUPLES; t++) {
            float temperature, coldReference;
            int status = temperatureSensors[t].getEnabledReading(temperature, coldReference);
            if (status == 0) {
               foundSensorCount++;
               value += temperature;
            }
         }
      }
      if (foundSensorCount==0) {
         // Safe value to return!
         return NAN;
      }
      return value/foundSensorCount;
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
      int status = temperatureSensors[0].getEnabledReading(temperature, coldReference);
      if ((status&7)==7) {
         return 0.0;
      }
      return coldReference;
   }
};

#endif /* SOURCES_TEMPERATURESENSORS_H_ */
