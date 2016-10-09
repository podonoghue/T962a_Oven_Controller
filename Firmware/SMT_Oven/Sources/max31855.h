/*
 * max31855.h
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_MAX31855_H_
#define SOURCES_MAX31855_H_

#include "settings.h"

/**
 * Class representing an MAX31855 connected over SPI
 *
 * @tparam pinNum Pin number for PCSn signal
 */
class Max31855 {
protected:

   /** SPI CTAR value */
   uint32_t spiCtarValue = 0;

   /** SPI used for LCD */
   USBDM::Spi &spi;

   /** Number of PCS signal to use */
   const int pinNum;

   /** Offset to add to reading from probe */
   USBDM::Nonvolatile<int> &offset;

   /** Used to disable sensor */
   USBDM::Nonvolatile<bool> &enabled;

   /**
    * Initialise the LCD
    */
   void initialise() {
      USBDM::waitMS(200);

      spi.setPcsPolarity(pinNum, false);

      IrqProtect protect;
      spi.setSpeed(5000000);
      spi.setMode(USBDM::SPI_MODE0);
      spi.setDelays(0.1*USBDM::us, 0.1*USBDM::us, 0.1*USBDM::us);
      spi.setFrameSize(8);

      // Record CTAR value in case SPI shared
      spiCtarValue = spi.getCTAR0Value();
   }

public:
   /**
    * Constructor
    *
    * @param spi     The SPI to use to communicate with MAX31855
    * @param pinNum  Number of PCS to use
    * @param offset  Offset to add to reading from probe
    */
   Max31855(USBDM::Spi &spi, int pinNum, USBDM::Nonvolatile<int> &offset, USBDM::Nonvolatile<bool> &enabled) :
      spi(spi), pinNum(pinNum), offset(offset), enabled(enabled) {
      initialise();
   }

   /**
    * Convert status to string
    *
    * @param status 3-bit status value from thermocouple
    *
    * @return Short string representing status
    */
   static const char *getStatusName(int status) {
      if (status & 0x08) {
         return "Dis.";
      }
      switch (status) {
      case   0  : return "OK";     // OK!
      case   1  : return "Open";   // No probe or open circuit
      case   2  : return "Gnd";    // Probe short to Gnd
      case   4  : return "Vcc";    // Probe short to Vcc
      case   7  : return "----";   // No response - Max31855 not present at that address
      default   : return "????";   // Unknown
      }
   }
   /**
    * Enables/disables the sensor
    *
    * @param enable True to enable sensor
    */
   void enable(bool enable=true) {
      enabled =enable;
   }

   /**
    * Check if sensor is enabled
    *
    * @return true => enabled
    */
   bool isEnabled() {
      return enabled;
   }
   /**
    * Read thermocouple
    *
    * @param temperature   Temperature reading of external probe (.25 degree resolution)
    * @param coldReference Temperature reading of internal cold-junction reference (.0625 degree resolution)
    *
    * @return error flag from sensor @ref getStatusName() \n
    *    0     => OK, \n
    *    0bxxx1 => Open circuit, \n
    *    0bxx1x => Short to Gnd, \n
    *    0bx1xx => Short to Vcc
    *    0bx111 => No response - Max31855 not present at that address
    *    0b1xxx => Max31855 is disabled (values MAY be valid)
    */
   int getReading(float &temperature, float &coldReference) {
      uint8_t data[] = {
            0xFF, 0xFF, 0xFF, 0xFF,
      };
      int status;
      {
         IrqProtect protect;
         spi.setCTAR0Value(spiCtarValue);
         spi.setPushrValue(SPI_PUSHR_CTAS(0)|SPI_PUSHR_PCS(1<<pinNum));
         spi.txRxBytes(sizeof(data), nullptr, data);
      }
      // Temperature = sign-extended 14-bit value
      temperature = (((int16_t)((data[0]<<8)|data[1]))>>2)/4.0;

      // Add manual offset
      temperature += offset;

      // Cold junction = sign-extended 12-bit value
      coldReference = (((int16_t)((data[2]<<8)|data[3]))>>4)/16.0;

      status = data[3]&0x07;

      if (status == 7) {
         temperature   = NAN;
         coldReference = NAN;
      }
      else if (status != 0) {
         // Temperature = sign-extended 14-bit value
         temperature = NAN;
      }
      if (!enabled) {
         status |= 8;
      }
      // Return error flag
      return status;
   }
   /**
    * Read thermocouple
    *
    * @param temperature   Temperature reading of external probe (.25 degree resolution)
    * @param coldReference Temperature reading of internal cold-junction reference (.0625 degree resolution)
    *
    * @return error flag from sensor @ref getStatusName() \n
    *    0     => OK, \n
    *    0b0xx1 => Open circuit, \n
    *    0b0x1x => Short to Gnd, \n
    *    0b01xx => Short to Vcc
    *    0b0111 => No response - Max31855 not present or disabled
    */
   int getEnabledReading(float &temperature, float &coldReference) {
      if (!enabled) {
         temperature   = 0;
         coldReference = 0;
         return 7;
      }
      return getReading(temperature, coldReference);
   }
};

#endif /* SOURCES_MAX31855_H_ */
