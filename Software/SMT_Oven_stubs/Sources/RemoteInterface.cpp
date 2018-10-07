/**
 * @file    RemoteInterface.cpp
 * @brief   Oven Remote control
 *
 *  This file contains the handler for the remote USB CDC command handler.\n
 *  It runs as a separate thread communicating with the USB interrupt handler
 *  through MailQueue queues.
 *
 *  Created on: 26Feb.,2017
 *      Author: podonoghue
 */
#include <RemoteInterface.h>
#include "cmsis.h"
#include "configure.h"

/** Current command */
RemoteInterface::Command   *RemoteInterface::command;

/** Current response */
RemoteInterface::Response  *RemoteInterface::response;

/** The remote handler thread */
CMSIS::Thread RemoteInterface::handlerThread(RemoteInterface::commandThread);

/** Mail queue USB -> handler thread */
CMSIS::MailQueue<RemoteInterface::Command,  4> RemoteInterface::commandQueue;

/** Mail queue USB <- handler thread */
CMSIS::MailQueue<RemoteInterface::Response, 4> RemoteInterface::responseQueue;

/** ID string for Oven */
const char *RemoteInterface::IDN = "SMT-Oven 1.0.0.0\n\r";

/**
 * Set response over CDC
 *
 * @param response Response text to send
 *
 * @return true Success
 */
bool RemoteInterface::send(Response *response) {
   responseQueue.put(response);
//   PUTS("send()");
   notifyUsbIn();
   return true;
}

/**
 * Writes thermocouple status to remote
 *
 * @param time  Time of log entry to send
 * @param lastEntry Indicates this is the last entry so append "\n\r"
 *
 * @return Number of characters written to buffer
 */
void RemoteInterface::logThermocoupleStatus(int time, bool lastEntry) {

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
      point.getTemperature(t, temperature);
      snprintf(buff2, sizeof(buff2), "%0.1f", temperature);
      if (t != 3) {
         strcat(buff2,",");
      }
      strcat(reinterpret_cast<char*>(response->data), buff2);
   }
   strcat(reinterpret_cast<char*>(response->data), ";");
   if (lastEntry) {
      // Terminate the whole transfer sequence
      strcat(reinterpret_cast<char*>(response->data),"\n\r");
   }
   response->size = strlen(reinterpret_cast<char*>(response->data));
   RemoteInterface::send(response);
}

/**
 *  Parse profile information into selected profile
 *
 *  @param cmd Profile described by a string e.g.\n
 *  4,My Profile,FF,1.0,140,183,90,1.4,210,15,-3.0;
 *  profile-number,description,flags,liquidus,preheatTime,soakTemp1,soakTemp2,soakTime,rampUpSlope,peakTemp,peakDwell,rampDownSlope
 *
 *  @return true  Successfully parsed
 *  @return false Failed parse
 */
bool parseProfile(char *cmd) {
   unsigned profileNum;
   SolderProfile profile;

   char *tok = strtok(cmd, ",");

   profileNum = strtol(tok, &cmd, 10);

   if (profileNum>=MAX_PROFILES) {
      return false;
   }

   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      // Assume setting current profile without changes
      currentProfileIndex = profileNum;
      return true;
   }

   if ((profiles[profileNum].flags & P_UNLOCKED) == 0) {
      // Profile is locked
      return false;
   }

   strncpy(profile.description, tok, sizeof(profile.description));
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.flags = strtol(tok, nullptr, 16);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.liquidus = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.preheatTime = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.soakTemp1 = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.soakTemp2 = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.soakTime = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.rampUpSlope = strtof(tok, nullptr);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.peakTemp = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   profile.peakDwell = strtol(tok, nullptr, 10);
   tok = strtok(nullptr, ";\n\r");
   if (tok == nullptr) {
      return false;
   }
   profile.rampDownSlope = strtof(tok, nullptr);

   currentProfileIndex = profileNum;
   profiles[profileNum] = profile;

   return true;
}

/**
 *  Parse thermocouple information into selected profile
 *
 *  @param cmd Describes the enable and offset value for each thermocouple e.g.\n
 *  1,-5,0,0,1,0,1,0;
 *
 *  @return true  Successfully parsed
 *  @return false Failed parse
 */
bool parseThermocouples(char *cmd) {
   char *tok;
   bool enable;
   int  offset;

   tok = strtok(cmd, ",");

   for (int t=0; t<4; t++) {
      if (tok == nullptr) {
         return false;
      }
      enable = (strtol(tok, nullptr, 10)!=0);
      tok    = strtok(nullptr, ",;\n\r");
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
 *  Parse PID information into PID parameters
 *
 *  @param cmd Describes the PID parameters e.g.\n
 *  .1,.024,23.4;
 *
 *  @return true  Successfully parsed
 *  @return false Failed parse
 */
bool parsePidParameters(char *cmd) {
   char *tok;

   tok = strtok(cmd, ",");
   if (tok == nullptr) {
      return false;
   }
   float kp = strtof(tok, nullptr);

   tok = strtok(nullptr, ",");
   if (tok == nullptr) {
      return false;
   }
   float ki = strtof(tok, nullptr);

   tok = strtok(nullptr, ";\n\r");
   if (tok == nullptr) {
      return false;
   }
   float kd = strtof(tok, nullptr);

   pidKp = kp;
   pidKi = ki;
   pidKd = kd;
   return true;
}

/**
 * Try to lock the Interactive MUTEX so that the remote session has ownership
 *
 * @param response Buffer to use for response if getting MUTEX fails.
 *
 * @return true  => success
 * @return false => failed (A fail response has been sent to the remote and response has been consumed)
 */
bool RemoteInterface::getInteractiveMutex(RemoteInterface::Response *response) {
   // Lock interface
   osStatus status = interactiveMutex.wait(0);

   // Obtained lock
   if (status == osOK) {
      return true;
   }
   strcpy(reinterpret_cast<char*>(response->data), "Failed - Busy\n\r");
   response->size = strlen(reinterpret_cast<char*>(response->data));
   RemoteInterface::send(response);
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
bool RemoteInterface::doCommand(Command *cmd) {

   // Allocate response buffer
   Response *response = allocResponseBuffer();
   if (response == nullptr) {
      // Discard command if we can't respond
      // This should be impossible
      return false;
   }

   if (strcasecmp((const char *)(cmd->data), "IDN?\n") == 0) {
      /*
       *  Identify oven
       *  -> "IDN?"
       *  <- "SMT-Oven 1.0.0.0"
       */
      strcpy(reinterpret_cast<char*>(response->data), IDN);
      response->size = strlen(IDN);
      send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "THERM ", 6) == 0) {
      /*
       * Sets the enable and offset value for each thermocouple
       * -> "THERM T1Enable,T1Offset,T2Enable,T2Offset,T3Enable,T3Offset,T4Enable,T5Offset"
       * <- "OK"
       */
      if (!getInteractiveMutex(response)) {
         return false;
      }
      if (parseThermocouples(reinterpret_cast<char*>(&cmd->data[6]))) {
         strcpy(reinterpret_cast<char*>(response->data), "OK\n\r");
      }
      else {
         strcpy(reinterpret_cast<char*>(response->data), "Failed - Data error\n\r");
      }
      interactiveMutex.release();
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "THERM?\n") == 0) {
      /*
       *  Get thermocouple status
       *  -> "THERM?"
       *  <- "T1Enable,T1Offset,T2Enable,T2Offset,T3Enable,T3Offset,T4Enable,T5Offset;"
       */
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
      send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "PID ", 4) == 0) {
      /*
       *  Set PID parameters
       *  -> "PID Proportional,Integral,Differential"
       *  <- "OK"
       */
      // Lock interface
      if (!getInteractiveMutex(response)) {
         return false;
      }
      if (parsePidParameters(reinterpret_cast<char*>(&cmd->data[4]))) {
         strcpy(reinterpret_cast<char*>(response->data), "OK\n\r");
      }
      else {
         strcpy(reinterpret_cast<char*>(response->data), "Failed - Data error\n\r");
      }
      interactiveMutex.release();
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PID?\n") == 0) {
      /*
       *  Set PID parameters
       *  -> "PID?"
       *  <- "Proportional,Integral,Differential;"
       */
      response->data[0] = (uint8_t)'\0';
      snprintf(reinterpret_cast<char*>(response->data), sizeof(response->data), "%f,%f,%f\n\r",
            (float)pidKp, (float)pidKi, (float)pidKd);
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "PROF ", 5) == 0) {
      /*
       *  Set profile parameters
       *  -> "PROF profile-number,description,flags,liquidus,preheatTime,soakTemp1,soakTemp2,soakTime,rampUpSlope,peakTemp,peakDwell,rampDownSlope;
       *  <- "OK"
       */
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
      interactiveMutex.release();
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PROF?\n") == 0) {
      /*
       *  Get current profile parameters
       *  -> "PROF?"
       *  -> "profile-number,description,flags,liquidus,preheatTime,soakTemp1,soakTemp2,soakTime,rampUpSlope,peakTemp,peakDwell,rampDownSlope;
       */
      const NvSolderProfile &profile = profiles[currentProfileIndex];
      snprintf(reinterpret_cast<char*>(response->data), sizeof(response->data),
            /* index         */ "%d,"
            /* description   */ "%s,"
            /* flags         */ "%2.2X,"
            /* liquidus      */ "%d,"
            /* preheatTime   */ "%d,"
            /* soakTemp1     */ "%d,"
            /* soakTemp2     */ "%d,"
            /* soakTime      */ "%d,"
            /* ramp2Slope    */ "%.1f,"
            /* peakTemp      */ "%d,"
            /* peakDwell     */ "%d,"
            /* rampDownSlope */ "%.1f;\n\r",
            (int)currentProfileIndex,
            (const char *)profile.description,
            (int)  profile.flags,
            (int)  profile.liquidus,
            (int)  profile.preheatTime,
            (int)  profile.soakTemp1,
            (int)  profile.soakTemp2,
            (int)  profile.soakTime,
            (float)profile.rampUpSlope,
            (int)  profile.peakTemp,
            (int)  profile.peakDwell,
            (float)profile.rampDownSlope);
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PLOT?\n") == 0) {
      /*
       *  Get plot value
       *  <- "PLOT?"
       *  ->
       */
      int lastValid = Draw::getData().getLastValid();
      snprintf(reinterpret_cast<char*>(response->data), sizeof(response->data), "%d;", lastValid+1);
      if (lastValid < 0) {
         // Terminate the response early
         strcat(reinterpret_cast<char*>(response->data), "\n\r");
      }
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
      for (int index=0; index<=lastValid; index++) {
         logThermocoupleStatus(index, index == lastValid);
      }
   }
   else if (strncasecmp((const char *)(cmd->data), "RUN\n\r", 4) == 0) {
      /*
       *   Start running current profile
       *   <- "RUN"
       *   -> "OK"
       */
      // Lock interface
      if (!getInteractiveMutex(response)) {
         return false;
      }
      RunProfile::remoteStartRunProfile();
      strcpy(reinterpret_cast<char*>(response->data), "OK\n\r");
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "ABORT\n\r", 4) == 0) {
      /*
       *   Abort running profile
       *   <- "ABORT"
       *   -> "OK"
       */
      // Lock interface
      if (!getInteractiveMutex(response)) {
         return false;
      }
      RunProfile::abortRunProfile();
      // Unlock previous lock
      interactiveMutex.release();
      strcpy(reinterpret_cast<char*>(response->data), "OK\n\r");
      // Unlock interface
      interactiveMutex.release();
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "RUN?\n\r", 4) == 0) {
      /*
       * Get state of running profile
       * <- "RUN?"
       * -> "OK|Failed|Running"
       */
      // Lock interface
      if (!getInteractiveMutex(response)) {
         return false;
      }
      State state = RunProfile::remoteCheckRunProfile();
      if (state == s_complete) {
         // Unlock previous lock
         interactiveMutex.release();
         strcpy(reinterpret_cast<char*>(response->data), "OK\n\r");
      }
      else if (state == s_fail) {
         // Unlock interface
         interactiveMutex.release();
         strcpy(reinterpret_cast<char*>(response->data), "Failed\n\r");
      }
      else {
         strcpy(reinterpret_cast<char*>(response->data), "Running\n\r");
      }
      // Unlock interface
      interactiveMutex.release();
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   else {
      /*
       * Unknown command
       * <- "?????"
       * -> "Failed - ..."
       */
      strcpy(reinterpret_cast<char*>(response->data), "Failed - unrecognized command\n\r");
      response->size = strlen(reinterpret_cast<char*>(response->data));
      send(response);
   }
   return true;
}

/**
 * Thread handling CDC traffic
 */
void RemoteInterface::commandThread(const void *) {
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
 * Starts the thread that handles the CDC communications.
 */
void RemoteInterface::initialise() {
   command  = nullptr;
   response = nullptr;

   commandQueue.create();
   responseQueue.create();

   handlerThread.run();
}

/**
 * Process data received from host\n
 * The data is collected into a command and then added to command queue\n
 * This function is actually called from the USB interrupt thread and passes
 * the assembled command to the handler using a MailQueue queue.
 *
 * @param size Amount of data
 * @param buff Buffer for data
 *
 * @note the Data is volatile and is processed or saved immediately.
 */
void RemoteInterface::putData(int size, const uint8_t *buff) {
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
