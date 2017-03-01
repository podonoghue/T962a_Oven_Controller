/*
 * SCPIInterface.h
 *
 *  Created on: 26Feb.,2017
 *      Author: podonoghue
 */

#ifndef SOURCES_SCPIINTERFACE_H_
#define SOURCES_SCPIINTERFACE_H_

#include <algorithm>
#include "cmsis.h"
#include "CDCInterface.h"

class SCPI_Interface: public CDC_Interface {
public:
   using Command  = struct {uint8_t data[100];  unsigned size; };
   using Response = struct {uint8_t data[1000]; unsigned size; };

protected:
   SCPI_Interface() {}
   virtual ~SCPI_Interface() {};

public:
   static Command  *command;
   static Response *response;

   static CMSIS::Pool<Command, 4>     *commandPool;
   static CMSIS::Pool<Response, 4>    *responsePool;
   static CMSIS::Message<Command, 4>  *commandQueue;
   static CMSIS::Message<Response, 4> *responseQueue;

   static CMSIS::Thread *handlerThread;
   static const char     *IDN;

public:

   static bool send(const char *text) {
      Response *response = responsePool->alloc();
      if (response == nullptr) {
         return false;
      }
      strncpy((char *)(response->data), text, sizeof(Response::data));
      response->size = std::min(strlen(text), sizeof(Response::data));
      responseQueue->put(response);
      notifyUsbIn();
      return true;
   }

   static void handler(const void *) {
      for(;;) {
         osEvent event = commandQueue->get();
         if (event.status == osEventMessage) {
            Command *cmd = (Command *)event.value.p;
            if (strncasecmp((const char *)(cmd->data), "IDN?", cmd->size) == 0) {
               send(IDN);
            }
            commandPool->free(cmd);
         }
      }
   }

   static void initialise() {
     commandPool   = new CMSIS::Pool<Command, 4>();
     responsePool  = new CMSIS::Pool<Response, 4>();
     command = commandPool->alloc();
     command->size = 0;
     response = nullptr;
     commandQueue  = new CMSIS::Message<Command, 4>();
     responseQueue = new CMSIS::Message<Response, 4>();

     handlerThread = new CMSIS::Thread(handler);
   }

   static void processCommand() {
      commandQueue->putISR((uint32_t)command);
      command       = commandPool->alloc();
      command->size = 0;
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
      assert((command->size+size)<(sizeof(command->data)/sizeof(command->data[0])));
      for (int i=0; i<size; i++) {
         if ((buff[i] == '\r') || (buff[i] == '\n')) {
            if (command->size>0) {
               processCommand();
            }
            continue;
         }
         command->data[command->size++] = buff[i];
      }
   }
//   /**
//    * Get data to transmit to host
//    *
//    * @param bufSize Size of buffer
//    * @param buff    Buffer for data
//    *
//    * @return Amount of data placed in buffer
//    */
//   static int getData(unsigned bufSize, uint8_t *buff) {
//      static unsigned index = 0;
//      if (response == nullptr) {
//         osEvent status = responseQueue->getISR();
//         if (status.status != osEventMessage) {
//            // No messages waiting
//            return 0;
//         }
//         // Set up new message
//         response = (Response*)status.value.p;
//         index = 0;
//      }
//      unsigned size = response->size - index;
//      if (size>bufSize) {
//         size = bufSize;
//      }
//      memcpy(buff, response->data+index, size);
//      index += size;
//      if (index >= response->size) {
//         responsePool->free(response);
//         response = nullptr;
//      }
//      return size;
//   }

};

#endif /* SOURCES_SCPIINTERFACE_H_ */
