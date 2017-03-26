/**
 * @file    SCPIInterface.cpp
 * @brief   SCPI (Very incomplete)
 *
 *  Created on: 26Feb.,2017
 *      Author: podonoghue
 */

#include "cmsis.h"
#include "SCPIInterface.h"
#include "configure.h"

SCPI_Interface::Command   *SCPI_Interface::command;
SCPI_Interface::Response  *SCPI_Interface::response;

CMSIS::Thread SCPI_Interface::handlerThread(SCPI_Interface::commandThread);

CMSIS::MailQueue<SCPI_Interface::Command, 4>  SCPI_Interface::commandQueue;
CMSIS::MailQueue<SCPI_Interface::Response, 4> SCPI_Interface::responseQueue;

const char *SCPI_Interface::IDN = "SMT-Oven 1.0.0.0\n\r";

/**
 * Set response over CDC
 *
 * @param text Response text to send
 */
bool SCPI_Interface::send(Response *response) {
   responseQueue.put(response);
   notifyUsbIn();
   return true;
}

/**
 * Writes thermocouple status to log
 *
 * @param time  Time of log entry to send
 * @param lastEntry Indicates this is the last entry so append "\n\r"
 */
void SCPI_Interface::logThermocoupleStatus(int time, bool lastEntry) {

   // Allocate buffer for response
   Response *response = allocResponseBuffer();
   if (response == nullptr) {
      // Failed allocation - discard
      return;
   }
   // Data point to log
   const DataPoint &point = Draw::getDataPoint(time);

   // Format response
   snprintf(reinterpret_cast<char*>(response->data), sizeof(response->data), "%s,%d,%0.1f,%0.1f,%d,%d,",
         Reporter::getStateName(point.getState()),
         time,
         point.getTargetTemperature(),
         point.getAverageTemperature(),
         point.getHeater(),
         point.getFan());
   for (unsigned t=0; t<DataPoint::NUM_THERMOCOUPLES; t++) {
      char buff2[10];
      float temperature;
      point.getTemperature(time, temperature);
      snprintf(buff2, sizeof(buff2), "%0.1f", temperature);
      if (t != 3) {
         strcat(buff2,",");
      }
      strcat(reinterpret_cast<char*>(response->data), buff2);
   }
   strcat(reinterpret_cast<char*>(response->data), ";");
   if (lastEntry) {
      strcat(reinterpret_cast<char*>(response->data),"\n\r");
   }
   response->size = strlen(reinterpret_cast<char*>(response->data));
   SCPI_Interface::send(response);
}

/**
 *  Parse profile information into selected profile
 *
 *  @param cmd Profile described by a string e.g.\n
 *  4,My Profile,FF,1.0,140,183,90,1.4,210,15,-3.0;
 */
bool parseProfile(char *cmd) {
   int profileNum;
   SolderProfile profile;
   char *tok;

   tok = strtok(cmd, ",");
   profileNum             = strtod(tok, &cmd);

   if ((profiles[profileNum].flags & P_UNLOCKED) == 0) {
      // Profile is locked
      return false;
   }
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   strncpy(profile.description, tok, sizeof(profile.description));
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.flags          = strtol(tok, nullptr, 16);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.ramp1Slope     = strtof(tok, nullptr);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.soakTemp1      = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.soakTemp2      = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.soakTime       = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.ramp2Slope     = strtof(tok, nullptr);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.peakTemp       = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.peakDwell      = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ";");
   if (tok == nullptr) {
      return false;
   }
   profile.rampDownSlope  = strtof(tok, nullptr);

   profiles[profileNum] = profile;
   return true;
}

/**
 *  Parse thermocouple information into selected profile
 *
 *  @param cmd Describes the enable and offset value for each thermocouple e.g.\n
 *  1,-5,0,0,1,0,1,0;
 */
bool parseTherocouples(char *cmd) {
   char *tok;
   bool enable;
   int  offset;

   tok    = strtok(cmd, ",");

   for (int t=0; t<4; t++) {
      if (tok == nullptr) {
         return false;
      }
      enable = (strtol(tok, nullptr, 10)!=0);
      tok    = strtok(nullptr, ",;");
      if (tok == nullptr) {
         return false;
      }
      offset = strtol(tok, nullptr, 10);
      tok    = strtok(nullptr, ",");
      if ((offset<-10) || (offset>10)) {
         return false;
      }
      temperatureSensors.getThermocouple(t).enable(enable);
      temperatureSensors.getThermocouple(t).setOffset(offset);
   }
   return true;
}

/**
 * Try to lock the Interactive mutex so that the remote session has ownership
 *
 * @param response Buffer to use for response if needed.
 *
 * @return true  => success
 * @return false => failed (A fail response has been sent to the remote and response has been consumed)
 */
bool SCPI_Interface::getInteractiveMutex(SCPI_Interface::Response *response) {
   // Lock interface
   osStatus status = interactiveMutex->wait(0);

   // Obtained lock
   if (status == osOK) {
      return true;
   }
   strcpy(reinterpret_cast<char*>(response->data), "Failed - Busy\n\r");
   response->size = strlen(reinterpret_cast<char*>(response->data));
   SCPI_Interface::send(response);
   return false;
}

/**
 * Execute remote command
 *
 * @param cmd Command string from remote
 *
 * @return true  => success
 * @return false => failed (A fail response has been sent to the remote)
 */
bool SCPI_Interface::doCommand(Command *cmd) {

   // Allocate response buffer
   Response *response = allocResponseBuffer();
   if (response == nullptr) {
      // Discard command if we can't respond
      return false;
   }

   if (strcasecmp((const char *)(cmd->data), "IDN?\n") == 0) {
      strcpy(reinterpret_cast<char*>(response->data), IDN);
      response->size = strlen(IDN);
      send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "THERM ", 6) == 0) {
      // Lock interface
      if (!getInteractiveMutex(response)) {
         return false;
      }
      if (parseTherocouples(reinterpret_cast<char*>(&cmd->data[6]))) {
         strcpy(reinterpret_cast<char*>(response->data), "OK\n\r");
      }
      else {
         strcpy(reinterpret_cast<char*>(response->data), "Failed - Data error\n\r");
      }
      interactiveMutex->release();
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "THERM?\n") == 0) {
      response->data[0] = (uint8_t)'\0';
      for (int t=0; t<4; t++) {
         char buff[10];
         snprintf(buff, sizeof(buff),"%d,%d",
               temperatureSensors.getThermocouple(t).isEnabled(),
               temperatureSensors.getThermocouple(t).getOffset());
         if (t != 3) {
            strcat(buff, ",");
         }
         else {
            strcat(buff, ";\n\r");
         }
         strcat(reinterpret_cast<char*>(response->data), buff);
      }
      response->size = strlen(reinterpret_cast<char*>(response->data));
      SCPI_Interface::send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "PROF ", 5) == 0) {
      // Lock interface
      if (!getInteractiveMutex(response)) {
         return false;
      }
      if (parseProfile(reinterpret_cast<char*>(&cmd->data[5]))) {
         strcpy(reinterpret_cast<char*>(response->data), "OK\n\r");
      }
      else {
         strcpy(reinterpret_cast<char*>(response->data), "Failed - data error\n\r");
      }
      interactiveMutex->release();
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PROF?\n") == 0) {
      const NvSolderProfile &profile = profiles[profileIndex];
      snprintf(reinterpret_cast<char*>(response->data), sizeof(response->data),
            /* index         */ "%d,"
            /* name          */ "%s,"
            /* flags         */ "%2.2X,"
            /* ramp1Slope    */ "%.1f,"
            /* soakTemp1     */ "%d,"
            /* soakTemp2     */ "%d,"
            /* soakTime      */ "%d,"
            /* ramp2Slope    */ "%.1f,"
            /* peakTemp      */ "%d,"
            /* peakDwell     */ "%d,"
            /* rampDownSlope */ "%.1f;\n\r",
            (int)profileIndex,
            (const char *)profile.description,
            (uint8_t)~(uint8_t)profile.flags,
            (float)profile.ramp1Slope,
            (int)  profile.soakTemp1,
            (int)  profile.soakTemp2,
            (int)  profile.soakTime,
            (float)profile.ramp2Slope,
            (int)  profile.peakTemp,
            (int)  profile.peakDwell,
            (float)profile.rampDownSlope);
      response->size = strlen(reinterpret_cast<char*>(response->data));
      SCPI_Interface::send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PLOT?\n") == 0) {
      int lastValid = Draw::getData().getLastValid();
      snprintf(reinterpret_cast<char*>(response->data), sizeof(response->data), "%d;", lastValid+1);
      if (lastValid < 0) {
         // Terminate the response early
         strcat(reinterpret_cast<char*>(response->data), "\n\r");
      }
      response->size = strlen(reinterpret_cast<char*>(response->data));
      SCPI_Interface::send(response);
      for (int index=0; index<=lastValid; index++) {
         logThermocoupleStatus(index, index == lastValid);
      }
   }
   else {
      strcpy(reinterpret_cast<char*>(response->data), "Failed - unrecognized command\n\r");
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   return true;
}

/**
 * Thread handling CDC traffic
 */
void SCPI_Interface::commandThread(const void *) {
   for(;;) {
      osEvent event = commandQueue.get();
      if (event.status == osEventMail) {
         // Get command
         Command *cmd = (Command *)event.value.p;
         // Process command
         doCommand(cmd);
         // Release command storage
         commandQueue.free(cmd);
      }
   }
}

/**
 * Initialise
 *
 * Starts the thread that handles the communications.
 */
void SCPI_Interface::initialise() {
   command  = nullptr;
   response = nullptr;

   commandQueue.create();
   responseQueue.create();

   handlerThread.run();
}

/**
 * Process data received from host\n
 * The data is collected into a command and then added to command queue
 *
 * @param size Amount of data
 * @param buff Buffer for data
 *
 * @note the Data is volatile and is processed or saved immediately.
 */
void SCPI_Interface::putData(int size, const uint8_t *buff) {
   for (int i=0; i<size; i++) {
      if (command == nullptr) {
         // Allocate new command buffer
         command = commandQueue.allocISR();
         if (command == nullptr) {
            // Can't allocate buffer - discard data & return
            return;
         }
         command->size = 0;
      }
      // Check for command too large
      assert(command->size<((sizeof(command->data)/sizeof(command->data[0]))-2));

      // Check for command termination
      if ((buff[i] == '\r') || (buff[i] == '\n')) {
         // Discard empty commands (discards '\r', '\n')
         if (command->size>0) {
            // Terminate command
            command->data[command->size++] = '\n';
            command->data[command->size++] = '\0';

            // Add this command to queue
            commandQueue.put(command);

            // We no longer have an active buffer
            command = nullptr;
         }
         continue;
      }
      // Save data to buffer
      command->data[command->size++] = buff[i];
   }
}
