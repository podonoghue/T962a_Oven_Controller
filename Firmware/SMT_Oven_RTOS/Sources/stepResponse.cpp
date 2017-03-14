/*
 * stepResponse.cpp
 *
 *  Created on: 8Oct.,2016
 *      Author: podonoghue
 */

#include <math.h>
#include <stdio.h>
#include "configure.h"

class StepResponse {
public:
   static int   time;
   static float lastTemperature;
   static float temperature;
   static int   fan;
   static int   heater;
   static bool  abort;

   static bool printflag;

   static bool abortCheck() {
      if (printflag) {
         time++;
         printflag         = false;
         lastTemperature   = temperature;
         temperature       = getTemperature();
         fan               = ovenControl.getFanDutycycle();
         heater            = ovenControl.getHeaterDutycycle();
         printf("%4d, %4d, %4d, %5.1f\n", time, fan, heater, temperature);
      }
      return (buttons.getButton() == SwitchValue::SW_S);
   }

   void report() {
      printflag = true;
      if (USBDM::wait(1.0f, abortCheck)) {
         abort = true;
      }
   }

   void waitForPlateau() {
      bool flag;
      do {
         report();
         if (abort) {
            return;
         }
         disableInterrupts();
         flag = (fabs(temperature-lastTemperature)<0.01);
         enableInterrupts();
      } while (!flag);
   }

   void run() {
      printf("Step response\n\n");
      printf("Time, Fan, Heater, Temperature\n");

      abort = 0;
      time = 0;
      ovenControl.setFanDutycycle(0);
      ovenControl.setHeaterDutycycle(30);

      // Wait until temp stops rising
      waitForPlateau();

      for(int i=0; i<50; i++) {
         if (abort) {
            break;
         }
         report();
      }
      ovenControl.setHeaterDutycycle(40);

      // Wait until temp stops rising
      waitForPlateau();

      for(int i=0; i<50; i++) {
         if (abort) {
            break;
         }
         report();
      }
      ovenControl.setHeaterDutycycle(0);
      ovenControl.setFanDutycycle(100);
      while (temperature > 50.0) {
         if (abort) {
            break;
         }
         report();
      }
      ovenControl.setFanDutycycle(0);
      printf("Step response - done\n\n");
   }
};

int   StepResponse::time;
float StepResponse::lastTemperature;
float StepResponse::temperature;
int   StepResponse::fan;
int   StepResponse::heater;
bool  StepResponse::printflag;
bool  StepResponse::abort;

StepResponse stepResponse;



