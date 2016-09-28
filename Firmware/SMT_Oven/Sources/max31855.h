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
   Max31855(USBDM::Spi &spi, int pinNum, USBDM::Nonvolatile<int> &offset)
      : spi(spi), pinNum(pinNum), offset(offset) {
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
      switch (status&0x07) {
      case 0  : return "OK";     // OK!
      case 1  : return "Open";   // No probe or open circuit
      case 2  : return "Gnd";    // Probe short to Gnd
      case 4  : return "Vcc";    // Probe short to Vcc
      case 7  : return "----";   // No response - Max31855 not present at that address
      default : return "???";
      }
   }
   /**
    * Read thermocouple
    *
    * @param temperature   Temperature reading of external probe (.25 degree resolution)
    * @param coldReference Temperature reading of internal cold-junction reference (.0625 degree resolution)
    *
    * @return error flag from sensor @ref getStatusName() \n
    *    0     => OK, \n
    *    0bxx1 => Open circuit, \n
    *    0bx1x => Short to Gnd, \n
    *    0b1xx => Short to Vcc
    *    0b111 => No response - Max31855 not present at that address
    */
   int getReading(float &temperature, float &coldReference) {
      uint8_t data[] = {
            0xFF, 0xFF, 0xFF, 0xFF,
      };
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

      // Return error flag
      return data[3]&0x07;
   }
};

#endif /* SOURCES_MAX31855_H_ */
