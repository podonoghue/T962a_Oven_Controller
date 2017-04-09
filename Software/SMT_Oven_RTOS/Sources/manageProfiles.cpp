/**
 * @file ManageProfiles.cpp
 *
 *  Created on: 17Mar.,2017
 *      Author: podonoghue
 */

#include <copyProfile.h>
#include <manageProfiles.h>
#include <plotting.h>
#include "lcd_st7920.h"
#include "configure.h"
#include "EditProfile.h"

namespace ManageProfiles {

static void putProfileMenu(const NvSolderProfile &profile) {
   bool editable = (profile.flags&P_UNLOCKED) != 0;

   // Menu
   constexpr int xMenuOffset = lcd.LCD_WIDTH-21;
   constexpr int yMenuOffset = 8;
   lcd.gotoXY(xMenuOffset, yMenuOffset);
   lcd.setInversion(true);
   lcd.putSpace(1); lcd.putString("F1"); lcd.putLeftArrow(); lcd.putSpace(2);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*1);
   lcd.putSpace(1); lcd.putString("F2"); lcd.putRightArrow(); lcd.putSpace(2);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*2);
   lcd.putSpace(1); lcd.putString("F3"); lcd.putSpace(2); lcd.putString(editable?"E":" "); lcd.putSpace(1);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*3);
   lcd.putSpace(1); lcd.putString("F4"); lcd.putSpace(2); lcd.putString("C"); lcd.putSpace(1);
   lcd.gotoXY(xMenuOffset, yMenuOffset+lcd.FONT_HEIGHT*4);
   lcd.putSpace(1); lcd.putString("S "); lcd.putEnter(); lcd.putSpace(2);
   lcd.setInversion(false);
}

/**
 * Display profiles for selection or editing
 *
 * On exit the current profile may be changed
 */
void profileMenu() {
   unsigned profileIndex = ::currentProfileIndex;
   bool needUpdate = true;

   for(;;) {
      if (needUpdate) {
         lcd.clear();
         Draw::drawProfile(profileIndex);
         Draw::update();
         putProfileMenu(profiles[profileIndex]);
         lcd.refreshImage();
         lcd.setGraphicMode();
         needUpdate = false;
      }
      switch(buttons.getButton()) {
      case SwitchValue::SW_F1:
         if (profileIndex>0) {
            profileIndex--;
            needUpdate = true;
         }
         break;
      case SwitchValue::SW_F2:
         if ((profileIndex+1)<(sizeof(profiles)/sizeof(profiles[0]))) {
            profileIndex++;
            needUpdate = true;
         }
         break;
      case SwitchValue::SW_F3:
         EditProfile::run(profiles[profileIndex]);
         needUpdate = true;
         break;
      case SwitchValue::SW_F4:
         CopyProfile::run(profileIndex);
         needUpdate = true;
         break;
      case SwitchValue::SW_S:
         ::currentProfileIndex.operator =(profileIndex);
         return;
      default:
         break;
      }
      __WFI();
   };
}

};

