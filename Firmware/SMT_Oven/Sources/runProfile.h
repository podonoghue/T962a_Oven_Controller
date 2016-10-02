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
   const NvSolderProfile *profile;

   /** Time in the sequence (seconds) */
   volatile int  time;

   /** Step in the profile sequence */
   volatile unsigned  step;

   /** Step in the profile sequence */
   volatile int  microStep;

   /** Profile complete flag */
   volatile bool complete;

   /** Used to record ambient temperature at start (Celsius) */
   float ambient;

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

   /**
    * Maps special temperature values
    */
   int mapTemp(int temp) {
      if (temp < 0) {
         temp = ambient;
      }
      return temp;
   }

   /*
    * Handles the call-back from the PIT to step through the sequence
    */
   void handler() {
      if (complete) {
         // Already completed
         return;
      }
      if (step>=(sizeof(profile->profile)/sizeof(profile->profile[0]))) {
         // At end of array
         complete = true;
         return;
      }
      // Advance time
      time++;
      if (time>profile->profile[step+1].time) {
         step++;
      }
      //  Last point - start temperature
      int lastTemp = profile->profile[step].temperature;
      if (lastTemp == STOP_TEMP) {
         pid.setSetpoint(0);
         complete = true;
         return;
      }
      lastTemp = mapTemp(lastTemp);
      // Next point - destination temperature
      int nextTemp = mapTemp(profile->profile[step+1].temperature);

      // Interpolate temperature
      float setpoint = lastTemp;
      setpoint +=
            (double)(time-profile->profile[step].time)*(nextTemp-lastTemp)
            /(profile->profile[step+1].time-profile->profile[step].time);
      // Set controller
      pid.setSetpoint(setpoint);
      if (nextTemp>=lastTemp) {
         // Ramp up or dwell
         ovenControl.setFanDutycycle(minimumFanSpeed);
      }
      else if (nextTemp<(lastTemp-50)) {
         // Cool down
         ovenControl.setFanDutycycle(100);
      }
   };

public:
   /**
    * Reports status to LCD and log
    */
   void thermocoupleStatus() {
      lcd.setInversion(false);
      lcd.clearFrameBuffer();
      lcd.putSpace(14); lcd.putString("Status Oven  ColdJn\n");
      lcd.drawHorizontalLine(9);
      lcd.gotoXY(0, 12+4*lcd.FONT_HEIGHT);
      if (pid.isEnabled()) {
         lcd.printf("%4ds S=%3d T=%0.1f\x7F", pid.getTicks(), (int)round(pid.getSetpoint()), pid.getInput());
         printf("%6.1f, %4d, %5.1f, %5.1f, %4d,", pid.getElapsedTime(), step, pid.getSetpoint(), pid.getInput(), ovenControl.getFanDutycycle());
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

   /**
    * Run sequence
    */
   void run() {
      step      = 0;
      time      = 0;
      microStep = 0;
      complete  = false;
      profile   = &profiles[profileIndex];
      /*
       * Obtain ambient temperature as reference
       */
      ambient = getTemperature();
      if (ambient>35) {
         // Assume an ambient of 35 if oven is still hot
         ambient = 35;
      }
      printf("Profile, %s\n",   (const char *)profile->description);
      printf("Ambient, %5.1f\n\n", ambient);
      printf("Time, Step, Target, Actual, Fan, T1-probe, T1-cold, T2-probe, T2-cold, T3-probe, T3-cold, T4-probe, T4-cold\n");

      pid.setSetpoint(mapTemp(profile->profile[0].temperature));
      pid.enable();

      // Using PIT
      USBDM::Pit::enable();

      FunctionWrapper fw(this);
      USBDM::Pit::setCallback(timerChannel, fw.f);

      USBDM::Pit::setPeriod(timerChannel, 1.0f);
      USBDM::Pit::enableChannel(timerChannel);
      USBDM::Pit::enableInterrupts(timerChannel);

      ovenControl.setFanDutycycle(minimumFanSpeed);
      float lastElapsedTime = -2;
      // Wait for completion
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
      // Wait for exit button before dialogue exit
      while (buttons.getButton() != SW_S) {
         __WFI();
      }
   }
};

template<int timerChannel> RunProfile<timerChannel>* RunProfile<timerChannel>::FunctionWrapper::This = nullptr;

#endif /* SOURCES_RUNPROFILE_H_ */
