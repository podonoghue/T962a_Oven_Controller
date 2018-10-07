/**
 * @file    cdc.cpp
 * @brief   CDC interface for USB
 *
 *  Created on: Nov 6, 2012
 *      Author: podonoghue
 */
#include <string.h>
#include "derivative.h" /* include peripheral declarations */
#include "utilities.h"
#include "system.h"
#include "cdc.h"

#define CDC_TX_BUFFER_SIZE (16)  // Should equal end-point buffer size
static char txBuffer[CDC_TX_BUFFER_SIZE];
static uint8_t txHead        = 0;
static uint8_t txBufferCount = 0;

#define CDC_RX_BUFFER_SIZE (16)  // Should less than or equal to end-point buffer size
static char *rxBuffer = nullptr;
static uint8_t rxBufferCount = 0;

/*
 * Simple double-buffering for Rx (in conjunction with USB buffer)
 */

/** 
 * Add a char to the CDC-Rx buffer
 *
 * @param ch - char to add
 *
 * @return true => success
 */
bool cdc_putRxBuffer(char ch) {

   // Lock while changes made
   IrqProtect protect;

   if (rxBufferCount >= CDC_RX_BUFFER_SIZE) {
      // Silently drop characters
      return false;
   }
   rxBuffer[rxBufferCount++] = ch;
   return true;
}

/**
 * Set current Rx Buffer
 *
 * @param buffer Buffer address, new data is written to this buffer
 * 
 * @return Number of characters in existing buffer
 *
 * @note Assumed called while interrupts blocked
 */
uint8_t cdc_setRxBuffer(char *buffer) {
   uint8_t temp;
   rxBuffer = buffer;
   temp = rxBufferCount;
   rxBufferCount = 0;
   return temp;
}

/**
 *  RxBufferEmpty() - Check if Rx buffer is empty
 *
 * @return -  >0 => buffer is not empty
 *            0  => buffer is empty
 */
uint8_t cdc_rxBufferItemCount(void) {
   return rxBufferCount;
}

/*
 * Simple double-buffering for CDC-Tx (in conjunction with USB end-point buffer)
 */
 
/**
 * Add data to Tx Buffer (from USB)
 *
 * @param source Source buffer to copy from
 * @param size   Number of bytes to copy
 *
 *  @return true => OK, false => Buffer is busy (overrun)
 */
bool cdc_putTxBuffer(char *source, uint8_t size) {
   if (txBufferCount > 0) {
      return false; // Busy
   }
   (void)memcpy(txBuffer, source, size);
   txHead        = 0;
   txBufferCount = size;
   return true;
}

/** getTx() -  Gets a character from the CDC-Tx queue.
 *
 * @return
 *  -  -ve => queue is empty \n
 *  -  +ve => char from queue
 */
int cdc_getTxBuffer() {
   uint8_t ch;
   if (txBufferCount == 0) {
      // Check data in USB buffer & restart USB Out if needed
      checkUsbCdcTxData();
   }
   // Need to re-check as above may have copied data
   if (txBufferCount == 0) {
      return -1;
   }
   ch = txBuffer[txHead++];
   if (txHead >= txBufferCount)
      txBufferCount = 0;
   return ch;
}

/**
 *  cdcTxSpace - check if CDC-Tx buffer is free
 *
 * @return 0 => buffer is occupied
 *         1 => buffer is free
 */
uint8_t cdc_txBufferIsFree() {
   return (txBufferCount == 0);
}

/*
 * Following are dummy routines as there is no actual serial communication
 */

static uint8_t cdcStatus = SERIAL_STATE_CHANGE;

static LineCodingStructure lineCoding = {nativeToLe32(9600UL),0,1,8};

uint8_t cdc_getSerialState() {
   uint8_t t = cdcStatus;
   cdcStatus = 0;
   return t;
}

/**
 *  Set CDC communication characteristics\n
 *  Dummy routine
 *
 * @param lineCodingStructure - Structure describing desired settings
 *
 */
void cdc_setLineCoding(const LineCodingStructure *lineCodingStructure) {
   cdcStatus  = SERIAL_STATE_CHANGE;

   (void)memcpy(&lineCoding, lineCodingStructure, sizeof(LineCodingStructure));

}

/**
 *  Get CDC communication characteristics\n
 *  Dummy routine
 *
 *  @return Structure describing current settings
 */
const LineCodingStructure *cdc_getLineCoding(void) {
   return &lineCoding;
}

/**
 *  Set CDC Line values
 *  Dummy routine
 *
 * @param value - Describing desired settings
 */
void cdc_setControlLineState(uint8_t value) {
   (void)value;
}

/**
 *  Send CDC break\n
 *  Dummy routine
 *
 * @param length - length of break in milliseconds (see note)\n
 *  - 0x0000 => End BREAK
 *  - 0xFFFF => Start indefinite BREAK
 *  - else   => Send a break of 10 chars
 */
void cdc_sendBreak(uint16_t length) {
   (void)length;
}
