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
 *
 * Remote Commands
 *
 * Identify oven
 *  -> "IDN?"
 *  <- "SMT-Oven 1.0.0.0"
 *
 * Set thermocouple enable and offset values
 *  -> "THERM T1Enable,T1Offset,T2Enable,T2Offset,T3Enable,T3Offset,T4Enable,T4Offset"
 *  <- "OK"
 *
 * Get thermocouple enable and offset values
 *  -> "THERM?"
 *  <- "T1Enable,T1Offset,T2Enable,T2Offset,T3Enable,T3Offset,T4Enable,T5Offset;"
 *
 * Set PID parameters
 *  -> "PID Proportional,Integral,Differential"
 *  <- "OK"
 *
 * Get PID parameters
 *  -> "PID?"
 *  <- "Proportional,Integral,Differential;"
 *
 * Set profile parameters
 *  -> "PROF profile-number,description,flags,liquidus,preheatTime,soakTemp1,soakTemp2,soakTime,rampUpSlope,peakTemp,peakDwell,rampDownSlope;
 *  <- "OK"
 *
 * Get current profile parameters
 *  -> "PROF?"
 *  -> "profile-number,description,flags,liquidus,preheatTime,soakTemp1,soakTemp2,soakTime,rampUpSlope,peakTemp,peakDwell,rampDownSlope;
 *
 *  Get plot value
 *  <- "PLOT?"
 *  -> 75;preheat,0,27.8,27.5,0,100,0.0,0.0,27.5,0.0;preheat,1,27.8,27.5,0,100,0.0,0.0,27.5,0.0;...//...;fail,74,0.0,27.8,100,30,0.0,0.0,27.8,0.0;
 *  Format: number_of_points;[state,time,target temperature, average temperature, heater percentage, fan percentage, T1, T2, T3, T4]*number_of_points
 *
 * Start running current profile
 *  <- "RUN"
 *  -> "OK"
 *
 * Abort running profile
 *  <- "ABORT"
 *  -> "OK"
 *
 * Get state of running profile
 *  <- "RUN?"
 *  -> "OK|Failed|Running"
 *
 * Unknown command
 *  <- "?????"
 *  -> "Failed - unrecognized command"
 */

#include "configure.h"
#include "cmsis.h"
#include "RemoteInterface.h"
#include "stringFormatter.h"

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
 * @param time       Time of log entry to send
 * @param lastEntry  Indicates this is the last entry so append "\n\r"
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
   USBDM::StringFormatter sf(reinterpret_cast<char*>(response->data), sizeof(response->data));
   sf.setFloatFormat(1);
   sf.write(Reporter::getStateName(point.getState())).write(',')
     .write(time).write(',')
     .write(point.getTargetTemperature()).write(',')
     .write(point.getAverageTemperature()).write(',')
     .write(point.getHeater()).write(',')
     .write(point.getFan()).write(',');

   for (unsigned t=0; t<DataPoint::NUM_THERMOCOUPLES; t++) {
      float temperature;
      point.getTemperature(t, temperature);
      sf.write(temperature);
      if (t != 3) {
         sf.write(',');
      }
   }
   sf.write(';');
   if (lastEntry) {
      // Terminate the whole transfer sequence
      sf.write("\n\r");
   }
   response->size = sf.length();
   send(response);
}

/**
 *  Parse profile information into selected profile
 *
 *  @param cmd    Profile described by a string e.g.\n
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

   strncpy(profile.description, tok, sizeof(profile.description)-1);
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
   using namespace USBDM;

   // Allocate response buffer
   Response *response = allocResponseBuffer();
   if (response == nullptr) {
      // Discard command if we can't respond
      // This should be impossible
      return false;
   }
   // Format response
   StringFormatter sf(reinterpret_cast<char*>(response->data), sizeof(response->data));
   sf.setFloatFormat(1);

   if (strcasecmp((const char *)(cmd->data), "IDN?\n") == 0) {
      /*
       *  Identify oven
       *  -> "IDN?"
       *  <- "SMT-Oven 1.0.0.0"
       */
      sf.write(IDN);
      response->size = sf.length();
      send(response);
   }
   else if (strncasecmp((const char *)(cmd->data), "THERM ", 6) == 0) {
      /*
       * Sets the enable and offset value for each thermocouple
       * -> "THERM T1Enable,T1Offset,T2Enable,T2Offset,T3Enable,T3Offset,T4Enable,T4Offset"
       * <- "OK"
       */
      if (!getInteractiveMutex(response)) {
         return false;
      }
      if (parseThermocouples(reinterpret_cast<char*>(&cmd->data[6]))) {
         sf.write("OK\n\r");
      }
      else {
         sf.write("Failed - Data error\n\r");
      }
      interactiveMutex.release();
      response->size = sf.length();
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
         sf.write((int)temperatureSensors.getThermocouple(t).isEnabled()).write(',')
           .write(temperatureSensors.getThermocouple(t).getOffset());
         if (t != 3) {
            sf.write(',');
         }
         else {
            sf.write(";\n\r");
         }
      }
      response->size = sf.length();
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
         sf.write("OK\n\r");
      }
      else {
         sf.write("Failed - Data error\n\r");
      }
      interactiveMutex.release();
      response->size = sf.length();
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PID?\n") == 0) {
      /*
       *  Get PID parameters
       *  -> "PID?"
       *  <- "Proportional,Integral,Differential;"
       */
      response->data[0] = (uint8_t)'\0';
      sf.write((float)pidKp).write(',');
      sf.write((float)pidKi).write(',');
      sf.write((float)pidKd).write("\n\r");
      response->size = sf.length();
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
         sf.write("OK\n\r");
      }
      else {
         sf.write("Failed - Data error\n\r");
      }
      interactiveMutex.release();
      response->size = sf.length();
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PROF?\n") == 0) {
      /*
       *  Get current profile parameters
       *  -> "PROF?"
       *  -> "profile-number,description,flags,liquidus,preheatTime,soakTemp1,soakTemp2,soakTime,rampUpSlope,peakTemp,peakDwell,rampDownSlope;
       */
      const NvSolderProfile &profile = profiles[currentProfileIndex];
      sf.write((int)          currentProfileIndex).write(',');     /* index         */
      sf.write((const char *) profile.description).write(',');     /* description   */
      sf.write((int)          profile.flags).write(',');           /* flags         */
      sf.write((int)          profile.liquidus).write(',');        /* liquidus      */
      sf.write((int)          profile.preheatTime).write(',');     /* preheatTime   */
      sf.write((int)          profile.soakTemp1).write(',');       /* soakTemp1     */
      sf.write((int)          profile.soakTemp2).write(',');       /* soakTemp2     */
      sf.write((int)          profile.soakTime).write(',');        /* soakTime      */
      sf.write((float)        profile.rampUpSlope).write(',');     /* ramp2Slope    */
      sf.write((int)          profile.peakTemp).write(',');        /* peakTemp      */
      sf.write((int)          profile.peakDwell).write(',');       /* peakDwell     */
      sf.write((float)        profile.rampDownSlope).write(',');   /* rampDownSlope */
      response->size = sf.length();
      send(response);
   }
   else if (strcasecmp((const char *)(cmd->data), "PLOT?\n") == 0) {
      /*
       *  Get plot value
       *  <- "PLOT?"
       *  -> 75;preheat,0,27.8,27.5,0,100,0.0,0.0,27.5,0.0;preheat,1,27.8,27.5,0,100,0.0,0.0,27.5,0.0;...//...;fail,74,0.0,27.8,100,30,0.0,0.0,27.8,0.0;
       *  number_of_points;[state,time,target temperature, average temperature, heater percentage, fan percentage, T1, T2, T3, T4]*
       */
      int lastValid = Draw::getData().getLastValid();
      sf.write(lastValid+1).write(';');
      if (lastValid < 0) {
         // Terminate the response early
         sf.write("\n\r");
      }
      response->size = sf.length();
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
      sf.write("OK\n\r");
      response->size = sf.length();
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
      // Unlock interface
      osStatus mutexStatus;
      do {
         mutexStatus = interactiveMutex.release();
      } while (mutexStatus == osOK);
      sf.write("OK\n\r");
      response->size = sf.length();
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
         sf.write("OK\n\r");
      }
      else if (state == s_fail) {
         // Unlock interface
         interactiveMutex.release();
         sf.write("Failed\n\r");
      }
      else {
         sf.write("Running\n\r");
      }
      // Unlock interface
      interactiveMutex.release();
      response->size = sf.length();
      send(response);
   }
   else {
      /*
       * Unknown command
       * <- "?????"
       * -> "Failed - unrecognized command"
       */
      sf.write("Failed - unrecognized command\n\r");
      response->size = sf.length();
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
void RemoteInterface::putData(int size, volatile const uint8_t *buff) {
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
      usbdm_assert(command->size<((sizeof(command->data)/sizeof(command->data[0]))-2), "Command size too large");

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
