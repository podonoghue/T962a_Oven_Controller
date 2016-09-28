/*
 * runProfile.h
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_RUNPROFILE_H_
#define SOURCES_RUNPROFILE_H_

#include "configure.h"
#include "functional"

using namespace USBDM;

template<int timerChannel>
class RunProfile {

private:
   class FunctionWrapper {
      static RunProfile *This;

   public:
      FunctionWrapper(RunProfile *This) {
         this->This = This;
      };
      static void f(void) {
         This->handler();
      }
   };

   volatile unsigned  time;
   volatile bool abort;

   void handler() {
      time++;
      pid.setSetpoint(profiles[profileIndex].profile[time]);
   };

public:
   void thermocoupleStatus() {
      lcd.clearFrameBuffer();
      lcd.setInversion(false);
      lcd.putSpace(14); lcd.putString("Status Oven  ColdJn\n");
      lcd.drawHorizontalLine(9);
      lcd.gotoXY(0, 12);
      for (int t=0; t<=3; t++) {
         float temperature, coldReference;
         int status = temperatureSensors[t].getReading(temperature, coldReference);

         lcd.printf("T%d:", t+1); lcd.putSpace(2);
         if (status == 0) {
            lcd.printf("OK   %5.1f\x7F %5.1f\x7F\n", temperature, coldReference);
         }
         else if (status != 7) {
            lcd.printf("%-4s  ----  %5.1f\x7F\n", Max31855::getStatusName(status), coldReference);
         }
         else {
            lcd.printf("%-4s\n", Max31855::getStatusName(status));
         }
      }
      if (pid.isEnabled()) {
         lcd.printf("%3ds S=%d T=%0.1f\x7F \n", pid.getTicks(), (int)round(pid.getSetpoint()), pid.getInput());
      }
   }

   void run() {
      time  = 0;
      abort = false;

      pid.setSetpoint(profiles[profileIndex].profile[0]);
      pid.enable();

      // Using PIT
      USBDM::Pit::enable();

      FunctionWrapper fw(this);

      USBDM::Pit::setCallback(timerChannel, fw.f);

      USBDM::Pit::setPeriod(timerChannel, 10000*USBDM::ms);
      USBDM::Pit::enableChannel(timerChannel);
      USBDM::Pit::enableInterrupts(timerChannel);

      ovenControl.setFanDutycycle(minimumFanSpeed);
      do {
         thermocoupleStatus();
         lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
         lcd.setInversion(false); lcd.putSpace(80);
         lcd.setInversion(true); lcd.putString(" Abort "); lcd.setInversion(false); lcd.putSpace(3);
         lcd.refreshImage();
         lcd.setGraphicMode();
         __WFI();

         if (time >= 10) { //(sizeof(profiles[0].profile)/sizeof(profiles[0].profile[0]))) {
            break;
         }
      } while (buttons.getButton() != SW_S);

      ovenControl.setHeaterDutycycle(0);
      if (abort) {
         ovenControl.setFanDutycycle(100);
      }
      else {
         ovenControl.setFanDutycycle(0);
      }
      pid.setSetpoint(0);
      pid.enable(false);
      USBDM::Pit::enableChannel(timerChannel, false);
      USBDM::Pit::enableInterrupts(timerChannel, false);
      USBDM::Pit::setCallback(timerChannel, nullptr);
   }
};

template<int timerChannel> RunProfile<timerChannel>* RunProfile<timerChannel>::FunctionWrapper::This = nullptr;

#endif /* SOURCES_RUNPROFILE_H_ */
