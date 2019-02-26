/**
 * @file CopyProfile.cpp
 *
 *  Created on: 17Mar.,2017
 *      Author: podonoghue
 */

#include <copyProfile.h>
#include "configure.h"
#include "lcd_st7920.h"
#include "messageBox.h"
#include "stringFormatter.h"

namespace CopyProfile {

static unsigned sourceProfileIndex;
static unsigned destinationProfileIndex;

/**
 * Draw copy profile selection screen
 */
void draw() {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   lcd.gotoXY(10,0);
   lcd.setInversion(true); lcd.write(" Copy Profile "); lcd.setInversion(false);
   lcd.gotoXY(0,1*lcd.FONT_HEIGHT+5);
   lcd.setInversion(false);lcd.write("Copy:");          lcd.setInversion(false);
   lcd.gotoXY(0,2*lcd.FONT_HEIGHT+5);
   lcd.write(sourceProfileIndex).write(":").write((const char *)profiles[sourceProfileIndex].description);
   lcd.gotoXY(0,4*lcd.FONT_HEIGHT);
   lcd.setInversion(false);lcd.write("To:");            lcd.setInversion(false);
   lcd.gotoXY(0,5*lcd.FONT_HEIGHT);
   lcd.write(destinationProfileIndex).write(":").write((const char *)profiles[destinationProfileIndex].description);

   lcd.gotoXY(8,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putUpArrow();        lcd.putSpace(4); lcd.setInversion(false); lcd.putSpace(6);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putDownArrow();      lcd.putSpace(4); lcd.setInversion(false); lcd.putSpace(6);
   lcd.gotoXY(lcd.LCD_WIDTH-6*lcd.FONT_WIDTH-22,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   if ((destinationProfileIndex != sourceProfileIndex) && (profiles[destinationProfileIndex].flags&P_UNLOCKED)) {
      lcd.setInversion(true); lcd.putSpace(4); lcd.write("OK");     lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(3);
   }
   lcd.gotoXY(lcd.LCD_WIDTH-4*lcd.FONT_WIDTH-11,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(4); lcd.write("EXIT"); lcd.putSpace(3); lcd.setInversion(false);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

/**
 * Copy a profile with confirmation dialogue
 *
 * @param[in] sourceIndex      Index of source profile
 * @param[in] destinationIndex Index of destination profile
 *
 * @return true  => Profile copied
 * @return false => Profile not copied (illegal/cancelled)
 */
bool copyProfile(unsigned sourceIndex, unsigned destinationIndex) {
   using namespace USBDM;

   MessageBoxResult rc;
   if ((destinationProfileIndex == sourceProfileIndex) || !(profiles[destinationProfileIndex].flags&P_UNLOCKED)) {
      // Illegal copy - quietly ignore
      return false;
   }
   USBDM::StringFormatter_T<100> sf;
   sf.write("Overwrite:\n").write(destinationIndex).write(":").write((const char *)profiles[destinationIndex].description);
   rc = messageBox("Overwrite Profile", sf.toString(), MSG_YES_NO);
   if (rc == MSG_IS_YES) {
      // Update profile in NV ram
      profiles[destinationIndex] = profiles[sourceIndex];
      profiles[destinationIndex].flags = profiles[destinationIndex].flags | P_UNLOCKED;
      return true;
   }
   return false;
}

/**
 * Run Copy Profile Dialogue
 *
 * @param[in] index Index of source profile to copy
 */
void run(int index) {
   sourceProfileIndex      = index;
   destinationProfileIndex = 4; // 1st writable profile

   bool needsUpdate = true;

   do {
      if (needsUpdate) {
         draw();
         needsUpdate = false;
      }
      switch(buttons.getButton()) {
         case SwitchValue::SW_F1:
            if (destinationProfileIndex>0) {
               destinationProfileIndex--;
               needsUpdate = true;
            }
            break;
         case SwitchValue::SW_F2:
            if ((destinationProfileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
               destinationProfileIndex++;
               needsUpdate = true;
            }
            break;
         case SwitchValue::SW_F4:
            if (copyProfile(sourceProfileIndex, destinationProfileIndex)) {
               return;
            }
            needsUpdate = true;
            break;
         case SwitchValue::SW_S:
            return;
         default:
            break;
      }
   } while(true);
}
}

