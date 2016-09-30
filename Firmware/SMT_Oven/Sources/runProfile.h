/*
 * runProfile.h
 *
 *  Created on: 28 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_RUNPROFILE_H_
#define SOURCES_RUNPROFILE_H_

using namespace USBDM;

template<int timerChannel>
class RunProfile {

private:
   /** Profile being run */
   const NvSolderProfile * volatile profile;

   /** Step in the profile sequence */
   volatile unsigned  step;

   /** Profile complete flag */
   volatile bool complete;

   class FunctionWrapper {
      static RunProfile      *This;

   public:
      FunctionWrapper(RunProfile *This) {
         this->This = This;
      };
      static void f(void) {
         This->handler();
      }
   };

   /*
    * Handles the call-back from the PIT to step through the sequence
    */
   void handler() {
      if (complete) {
         return;
      }
      if (step>=(sizeof(profile->profile)/sizeof(profile->profile[0]))) {
         complete = true;
         return;
      }
      step++;
      int setpoint = profile->profile[step];
      pid.setSetpoint(setpoint);
      if (setpoint == 0) {
         complete = true;
         return;
      }
   };

public:
   void thermocoupleStatus() {
      lcd.setInversion(false);
      lcd.clearFrameBuffer();
      lcd.putSpace(14); lcd.putString("Status Oven  ColdJn\n");
      lcd.drawHorizontalLine(9);
      lcd.gotoXY(0, 12+4*lcd.FONT_HEIGHT);
      if (pid.isEnabled()) {
         lcd.printf("%4ds S=%3d T=%0.1f\x7F", pid.getTicks(), (int)round(pid.getSetpoint()), pid.getInput());
         printf("%4d, %4d, S=%4d, T=%5.1f,", step, pid.getTicks(), (int)round(pid.getSetpoint()), pid.getInput());
      }
      lcd.gotoXY(0, 12);
      for (int t=0; t<=3; t++) {
         float temperature, coldReference;
         int status = temperatureSensors[t].getReading(temperature, coldReference);

         lcd.printf("T%d:", t+1); lcd.putSpace(2);
         if (status == 0) {
            lcd.printf("OK   %5.1f\x7F %5.1f\x7F\n", temperature, coldReference);
            printf("%5.1f, %5.1f,", temperature, coldReference);
         }
         else if (status != 7) {
            lcd.printf("%-4s  ----  %5.1f\x7F\n", Max31855::getStatusName(status), coldReference);
            printf("%5.1f, %5.1f,", 0.0, coldReference);
         }
         else {
            lcd.printf("%-4s\n", Max31855::getStatusName(status));
            printf("%5.1f, %5.1f,", 0.0, 0.0);
         }
      }
      puts("");
   }

   void run() {
      step     = 0;
      complete = false;
      profile  = &profiles[profileIndex];

      printf("Running: %s\n", (const char *)profile->description);

      pid.setSetpoint(profile->profile[0]);
      pid.enable();

      // Using PIT
      USBDM::Pit::enable();

      FunctionWrapper fw(this);
      USBDM::Pit::setCallback(timerChannel, fw.f);

      USBDM::Pit::setPeriod(timerChannel, (float)profile->SECONDS_PER_STEP);
      USBDM::Pit::enableChannel(timerChannel);
      USBDM::Pit::enableInterrupts(timerChannel);

      ovenControl.setFanDutycycle(minimumFanSpeed);
      float lastElapsedTime = -2;
      do {
         float elapsedTime = pid.getElapsedTime();
         if ((elapsedTime - lastElapsedTime) >= 1.0)  {
            thermocoupleStatus();
            lcd.gotoXY(82, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
            lcd.setInversion(true); lcd.putString(" Abort "); lcd.setInversion(false);
            lcd.refreshImage();
            lcd.setGraphicMode();
            lastElapsedTime = elapsedTime;
         }
         __WFI();
         if (complete) {
            break;
         }
      } while (buttons.getButton() != SW_S);

      pid.setSetpoint(0);
      pid.enable(false);
      ovenControl.setHeaterDutycycle(0);
      USBDM::Pit::enableInterrupts(timerChannel, false);
      USBDM::Pit::enableChannel(timerChannel, false);
      USBDM::Pit::setCallback(timerChannel, nullptr);

      lcd.gotoXY(22, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(false); lcd.putString(complete?"Complete - ":" Aborted - ");
      lcd.setInversion(true); lcd.putString(" Exit "); lcd.setInversion(false);
      lcd.refreshImage();
      lcd.setGraphicMode();

      if (complete) {
         ovenControl.setFanDutycycle(0);
      }
      else {
         // Aborted
         complete = true;
         ovenControl.setFanDutycycle(100);
      }

      // Sound buzzer
      Buzzer::play();
      while (buttons.getButton() != SW_S) {
         __WFI();
      }
   }
};

template<int timerChannel> RunProfile<timerChannel>* RunProfile<timerChannel>::FunctionWrapper::This = nullptr;

#endif /* SOURCES_RUNPROFILE_H_ */
