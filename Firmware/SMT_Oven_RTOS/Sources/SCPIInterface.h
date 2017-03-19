/**
 * @file SCPIInterface.h
 * @brief   SCPI (Very incomplete)
 *
 *  Created on: 26Feb.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_SCPIINTERFACE_H_
#define SOURCES_SCPIINTERFACE_H_

#include <algorithm>
#include "cmsis.h"
#include "CDCInterface.h"
#include "plotting.h"
#include "reporter.h"

/**
 *    USB CDC receive ISR ----> Command Queue -----> SCPI thread
 *                                                     ...
 *                                                     ...
 *    USB CDC send ISR <------- Response Queue <---- SCPI thread
 */
class SCPI_Interface: public CDC_Interface {

public:
   /** Structure holding a command */
   using Command  = struct {uint8_t data[100];  unsigned size; };

   /** Structure holding (part of) a response */
   using Response = struct {uint8_t data[1000]; unsigned size; };

protected:
   SCPI_Interface() {}
   virtual ~SCPI_Interface() {};

   /** Queue of received commands */
   static CMSIS::MailQueue<Command, 4>  commandQueue;

   /** Queue of sent responses */
   static CMSIS::MailQueue<Response, 4> responseQueue;

   /** Current command being assembled by USB receive ISR */
   static Command  *command;

   /** Current response being assembled by SCPI thread */
   static Response *response;

   /** Thread to handle CDC commands */
   static CMSIS::Thread handlerThread;

   /** Identification string */
   static const char *IDN;

   /**
    * Writes thermocouple status to log
    *
    * @param time  Time to use with log entry
    */
   static void logThermocoupleStatus(int time);

   /**
    * Thread handling CDC traffic
    */
   static void commandThread(const void *);

public:
   /**
    * Get response
    *
    * @return osEvent
    */
   static SCPI_Interface::Response *getResponse() {
      osEvent status = SCPI_Interface::responseQueue.getISR();
      if (status.status != osEventMail) {
         // No messages waiting
         return nullptr;
      }
      // Set up new message
      return (SCPI_Interface::Response*)status.value.p;
   }

   /**
    * Used to free response buffer
    *
    * @param response Buffer to free
    */
   static void freeResponseBuffer(SCPI_Interface::Response *&response) {
      SCPI_Interface::responseQueue.free(response);
      response = nullptr;
   }

   /**
    * Allocate send buffer
    */
   static Response *allocResponseBuffer() {
      return responseQueue.alloc();
   }

   /**
    * Set response over CDC
    *
    * @param text Response text to send
    */
   static bool send(Response *response);

   /**
    * Initialise
    */
   static void initialise();

   /**
    * Process data received from host\n
    * The data is collected into a command and then added to command queue
    *
    * @param size Amount of data
    * @param buff Buffer for data
    *
    * @note the Data is volatile and is processed or saved immediately.
    */
   static void putData(int size, const uint8_t *buff);
};

#endif /* SOURCES_SCPIINTERFACE_H_ */
