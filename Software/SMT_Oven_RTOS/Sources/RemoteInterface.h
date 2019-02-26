/**
 * @file  RemoteInterface.h
 * @brief Oven Remote control
 *
 *  Created on: 26Feb.,2017
 *      Author: podonoghue
 */
#ifndef SOURCES_REMOTEINTERFACE_H_
#define SOURCES_REMOTEINTERFACE_H_

#include <usb_cdc_interface.h>
#include <algorithm>
#include "cmsis.h"
#include "configure.h"
#include "plotting.h"
#include "reporter.h"

/**
 *    USB CDC receive ISR ----> Command Queue -----> Remote thread
 *                                                     ...
 *                                                     ...
 *    USB CDC send ISR <------- Response Queue <---- Remote thread
 */
class RemoteInterface: public USBDM::CDC_Interface {

public:
   /** Structure holding a command */
   struct Command {
      uint8_t data[100];
      unsigned size;
   };

   /** Structure holding (part of) a response */
   struct Response{
      uint8_t  data[1000];
      unsigned size;
   };

protected:
   RemoteInterface() {}
   virtual ~RemoteInterface() {};

   /** Queue of received commands */
   static CMSIS::MailQueue<Command, 4>  commandQueue;

   /** Queue of sent responses */
   static CMSIS::MailQueue<Response, 4> responseQueue;

   /** Current command being assembled by USB receive ISR */
   static Command  *command;

   /** Current response being assembled by Remote thread */
   static Response *response;

   /** Thread to handle CDC commands */
   static CMSIS::Thread handlerThread;

   /** Identification string */
   static const char *IDN;

   /**
    * Writes thermocouple status to log
    *
    * @param[in] time  Time of log entry to send
    * @param[in] lastEntry Indicates this is the last entry so append "\n\r"
    */
   static void logThermocoupleStatus(int time, bool lastEntry=false);

   /**
    * Try to lock the Interactive mutex so that the remote session has ownership
    *
    * @param[out] response Buffer to use for response if needed.
    *
    * @return true  => success
    * @return false => failed (A fail response has been sent to the remote and response has been consumed)
    */
   static bool getInteractiveMutex(RemoteInterface::Response *response);

   /**
    * Execute remote command
    *
    * @param command Command string from remote
    *
    * @return true  => success
    * @return false => failed (A fail response has been sent to the remote)
    */
   static bool doCommand(Command *command);

   /**
    * Thread handling CDC traffic
    */
   static void commandThread(const void *);

public:
   /**
    * Get response
    *
    * @return Response or nullptr if none available
    */
   static RemoteInterface::Response *getResponse() {
      osEvent status = RemoteInterface::responseQueue.getISR();
      if (status.status != osEventMail) {
         // No messages waiting
         return nullptr;
      }
      // Set up new message
      return (RemoteInterface::Response*)status.value.p;
   }

   /**
    * Used to free response buffer
    *
    * @param[in] response Response buffer to free
    */
   static void freeResponseBuffer(RemoteInterface::Response *response) {
      RemoteInterface::responseQueue.free(response);
   }

   /**
    * Allocate response (send) buffer
    *
    * @return Pointer to allocated buffer
    * @return nullptr Failed allocation
    */
   static Response *allocResponseBuffer() {
      return responseQueue.alloc();
   }

   /**
    * Set response over CDC
    *
    * @param[in] response Response text to send
    *
    * @return true Success
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
    * @param[in] size Amount of data
    * @param[in] buff Buffer containing data
    *
    * @note the Data is volatile and is processed or saved immediately.
    */
   static void putData(int size, volatile const uint8_t *buff);
};

#endif /* SOURCES_REMOTEINTERFACE_H_ */
