/**
 * @file     usb_cdc_interface.h
 * @brief    USB-CDC interface
 *
 * This module handles the UART interface for USB CDC
 *
 * @version  V4.12.1.80
 * @date     13 April 2016
 */

#ifndef SOURCES_USB_CDC_INTERFACE_H_
#define SOURCES_USB_CDC_INTERFACE_H_

#include <stdint.h>
#include <string.h>
#include "usb_defs.h"

namespace USBDM {

class CDC_Interface {

private:
   static constexpr uint8_t CDC_STATE_DCD_MASK        = 1<<0;
   static constexpr uint8_t CDC_STATE_DSR_MASK        = 1<<1;
   static constexpr uint8_t CDC_STATE_BREAK_IN_MASK   = 1<<2;
   static constexpr uint8_t CDC_STATE_RI_MASK         = 1<<3;
   static constexpr uint8_t CDC_STATE_FRAME_MASK      = 1<<4;
   static constexpr uint8_t CDC_STATE_PARITY_MASK     = 1<<5;
   static constexpr uint8_t CDC_STATE_OVERRUN_MASK    = 1<<6;
   static constexpr uint8_t CDC_STATE_CHANGE_MASK     = 1<<7;

   static constexpr uint8_t CDC_LINE_CONTROL_DTR_MASK = 1<<0;
   static constexpr uint8_t CDC_LINE_CONTROL_RTS_MASK = 1<<1;

protected:
   using simpleCallbak = bool (*)();

   /**
    * Wrapper for initialised static variable
    *
    * @return Reference to notifyUsbIn function pointer
    */
   static simpleCallbak &notifyUsbInPtr() {
      static simpleCallbak cb = nullptr;
      return cb;
   }

   /**
    * Wrapper for initialised static variable
    *
    * @return Reference to notifyUsbIn function pointer
    */
   static void notifyUsbIn() {
      simpleCallbak cb = notifyUsbInPtr();
      if (cb != nullptr) {
         cb();
      }
   }

protected:
   CDC_Interface() {}
   virtual ~CDC_Interface() {}

public:
   /**
    * Initialise class
    */
   static void initialise() {
   }

   /**
    * Set USB notify function
    *
    * @param cb The function to call to notify the USB In interface that new data is available
    */
   static void setUsbInNotifyCallback(simpleCallbak cb) {
      notifyUsbInPtr() = cb;
   }

   /**
    * Get state of serial interface
    *
    * @return Bit mask value
    */
   static CdcLineState getSerialState() {
      // Assume DCD & DSR
      static constexpr CdcLineState state = {CDC_STATE_DCD_MASK|CDC_STATE_DSR_MASK};
      return state;
   }

   /**
    * Process data received from host
    *
    * @param size Amount of data
    * @param buff Buffer for data
    *
    * @note the Data is volatile so should be processed or saved immediately.
    */
   static void putData(int size, const uint8_t *buff) {
      (void)size;
      (void)buff;
   }

   /**
    * Get data to transmit to host
    *
    * @param bufSize Size of buffer
    * @param buff    Buffer for data
    *
    * @return Amount of data placed in buffer
    */
   static int getData(int bufSize, uint8_t *buff) {
      (void)bufSize;
      (void)buff;
      return 0;
   }

   /**
    *  Get CDC communication characteristics\n
    *
    *  @return lineCodingStructure - Static structure describing current settings
    */
   static LineCodingStructure &getLineCoding() {
      static LineCodingStructure currentLineCoding = {0, 0, 0, 0};
      return currentLineCoding;
   }

public:
   /**
    * Set line coding
    *
    * @param lineCoding Line coding information
    */
   static void setLineCoding(LineCodingStructure * const lineCoding) {
      getLineCoding() = *lineCoding;
   }

   /**
    *  Set CDC Line values
    *
    * @param value - Describes desired settings
    */
   static void setControlLineState(uint8_t value) {
      (void) value;
      // Not implemented as no control signals
   }

   /**
    *  Send CDC break\n
    *
    * @param length Length of break in milliseconds (see note)\n
    *  - 0x0000 => End BREAK
    *  - 0xFFFF => Start indefinite BREAK
    *  - else   => Send a break of 10 chars

    * @note - only partially implemented
    *       - breaks are sent after currently queued characters
    */
   static void sendBreak(uint16_t length) {
      (void)length;
   }

}; // class CDC_interface

}; // end namespace USBDM

#endif /* SOURCES_USB_CDC_INTERFACE_H_ */
