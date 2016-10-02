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

   /** Profile complete flag */
   volatile bool complete;

   /** Used to record ambient temperature at start (Celsius) */
   float ambient;

   static RunProfile *This;

   static void callbackWrapper() {
      if (This != nullptr) {
         This->handler();
      }
   }

   static void setFan(uint speed) {
      switch (speed) {
      case 0: ovenControl.setFanDutycycle(0); break;
      case 1:
      case 2: ovenControl.setFanDutycycle(minimumFanSpeed); break;
      case 3: ovenControl.setFanDutycycle(100); break;
      }
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

      // Next point = destination temperature
      SolderProfile::Point nextPoint = profile->profile[step+1];
      if (time>nextPoint.time) {
         // Advance step in sequence
         step++;
         nextPoint = profile->profile[step+1];
      }
      SolderProfile::Point lastPoint = profile->profile[step];
      //  Last point = start temperature
      if (lastPoint.stop) {
         pid.setSetpoint(0);
         complete = true;
         return;
      }
      // Interpolate temperature
      float setpoint = lastPoint.temperature +
            (double)(time-lastPoint.time)*(nextPoint.temperature-lastPoint.temperature)
            /(nextPoint.time-lastPoint.time);

      // Set controller
      pid.setSetpoint(setpoint);
      setFan(lastPoint.fanSpeed);
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
         printf("%6.1f, %4d, %5.1f, %5.1f, %5.1f, %4d,", pid.getElapsedTime(), step, pid.getSetpoint(), pid.getInput(), pid.getOutput(), ovenControl.getFanDutycycle());
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
      printf("  Time, Step, Target, Actual, Heater, Fan, T1-probe, T1-cold, T2-probe, T2-cold, T3-probe, T3-cold, T4-probe, T4-cold\n");

      setFan(profile->profile[0].fanSpeed);
      pid.setSetpoint(profile->profile[0].temperature);
      pid.enable();

      // Using PIT
      USBDM::Pit::enable();

      This = this;
      USBDM::Pit::setCallback(timerChannel, callbackWrapper);
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

template<int timerChannel> RunProfile<timerChannel>* RunProfile<timerChannel>::This = nullptr;

#endif /* SOURCES_RUNPROFILE_H_ */
