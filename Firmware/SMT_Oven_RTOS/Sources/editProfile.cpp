/**
 * @file    editProfile.cpp
 * @brief   Profile editor
 *
 *  Created on: 8Oct.,2016
 *      Author: podonoghue
 */

#include "configure.h"
#include "editProfile.h"
#include "messageBox.h"

template<typename T> char ProfileSetting_T<T>::buff[];

//static const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_@#-";

void ProfileNameSetting::draw() {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   lcd.gotoXY(10,0);
   lcd.setInversion(true); lcd.putString(" Edit Name "); lcd.setInversion(false);

   lcd.gotoXY(0,1*lcd.FONT_HEIGHT+3);
   unsigned offset = 0;
   if (editPosition>=(lcd.LCD_WIDTH/lcd.FONT_WIDTH)) {
      offset = 1 + editPosition - (lcd.LCD_WIDTH/lcd.FONT_WIDTH);
   }
   lcd.putString(nameBuffer+offset);

   // Draw letter selection list
   lcd.gotoXY((editPosition-offset)*lcd.FONT_WIDTH, 1*lcd.FONT_HEIGHT+3);
   lcd.setInversion(true); lcd.putChar(nameBuffer[editPosition]); lcd.setInversion(false);
   lcd.gotoXY(0,2*lcd.FONT_HEIGHT+6);
   lcd.setInversion(false);
   for (uint8_t ch='A'; ch<='Z'; ch++) {
      lcd.putChar(ch);
      if (((ch-'A')%16) == 15) {
         lcd.putChar('\n');
      }
   }
   // Highlight selected entry letter
   lcd.gotoXY((letterPosition%16)*lcd.FONT_WIDTH,(2+(letterPosition/16))*lcd.FONT_HEIGHT+6);
   lcd.setInversion(true);
   lcd.putChar('A'+letterPosition);
   lcd.setInversion(false);

   // Draw menu
   lcd.gotoXY(2,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(2); lcd.putLeftArrow();    lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(6);
   lcd.setInversion(true); lcd.putSpace(2); lcd.putRightArrow();   lcd.putSpace(3); lcd.setInversion(false); lcd.putSpace(6);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Sel");  lcd.putSpace(2); lcd.setInversion(false); lcd.putSpace(6);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putString("Del");  lcd.putSpace(2); lcd.setInversion(false); lcd.putSpace(6);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putString("EXIT"); lcd.putSpace(2); lcd.setInversion(false);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

bool ProfileNameSetting::edit() {
   bool needsUpdate = true;
   bool changed     = false;

   letterPosition = 0;
   editPosition   = 0;

   do {
      if (needsUpdate) {
         draw();
         needsUpdate = false;
      }
      switch(buttons.getButton()) {
      case SW_F1: // Left
         if (letterPosition>0) {
            letterPosition--;
         }
         needsUpdate = true;
         break;
      case SW_F2: // Right
         if (letterPosition<26) {
            letterPosition++;
         }
         needsUpdate = true;
         break;
      case SW_F3: // Sel
         needsUpdate = true;
         break;
      case SW_F4: // Del
         needsUpdate = true;
         break;
      case SW_S: // Exit
         return changed;
      default:
         break;
      }

   } while(true);
   return true;
}

/** Current menu selection */
int EditProfile::selection = 0;

/** Offset scrolled for menu items */
int EditProfile::offset = 0;

/**
 * Draw screen
 */
void EditProfile::drawScreen() {
   static constexpr int MAX_VISIBLE_ITEMS = (lcd.LCD_HEIGHT/8)-2;

   // Adjust menu so selected item is visible
   if ((selection-offset) >= MAX_VISIBLE_ITEMS) {
      offset++;
   }
   if ((selection < offset)) {
      offset--;
   }
   lcd.setInversion(false); lcd.clearFrameBuffer();
   lcd.setInversion(true);  lcd.putString("  Edit Profile\n");  lcd.setInversion(false);

   for (int item=0; item<NUM_ITEMS; item++) {
      // Check visible
      if (item<offset) {
         continue;
      }
      if (item>(offset+MAX_VISIBLE_ITEMS)) {
         continue;
      }
      lcd.setInversion(item == selection);
      lcd.gotoXY(0, (item+1-offset)*lcd.FONT_HEIGHT);
      lcd.printf(items[item]->getDescription());
   }
   lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(false); lcd.putSpace(4);
   lcd.setInversion(true);  lcd.putString(" ");      lcd.putUpArrow();   lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" ");      lcd.putDownArrow(); lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" + ");    lcd.setInversion(false);            lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" - ");    lcd.setInversion(false);            lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" Exit "); lcd.setInversion(false);            lcd.putSpace(3);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

/**
 * Run Edit profile menu
 *
 * @return true => Profile edited and needs update in NV memory
 */
bool EditProfile::doit() {
   bool changed     = true;
   bool needsUpdate = false;
   for(;;) {
      if (changed) {
         drawScreen();
         changed = false;
      }
      switch(buttons.getButton()) {
      case SW_F1:
         if (selection>0) {
            selection--;
            changed = true;
         }
         break;
      case SW_F2:
         if (selection<(NUM_ITEMS-1)) {
            selection++;
            changed = true;
         }
         break;
      case SW_F3F4:
         needsUpdate = items[selection]->reset() || needsUpdate;
         changed = true;
         break;
      case SW_F3:
         needsUpdate = items[selection]->increment() || needsUpdate;
         changed = true;
         break;
      case SW_F4:
         needsUpdate = items[selection]->decrement() || needsUpdate;
         changed = true;
         break;
      case SW_S:
         return needsUpdate;
      default:
         break;
      }
      __WFI();
   }
   return needsUpdate;
}

/**
 * Allows editing of a Solder profile.\n
 * After editing the user is prompted to save the profile.
 *
 * @param nvProfile The profile to edit
 */
void EditProfile::run(NvSolderProfile &nvProfile) {

   if ((nvProfile.flags&P_UNLOCKED) == 0) {
      messageBox("Profile is locked", " Profile cannot be\n changed", MSG_OK);
      return;
   }
   // Make copy of profile to edit
   SolderProfile tempProfile;
   tempProfile = nvProfile;

   // Do the editing
   EditProfile editProfile(tempProfile);

   // Prompt user to save any changes
   MessageBoxResult rc;
   bool changed = false;
   do {
      if (editProfile.doit() || changed) {
         changed = true;
         char buff[100];
         if (!tempProfile.isValid()) {
            snprintf(buff, sizeof(buff), "%s\n\nProfile is invalid\nPlease check", tempProfile.description);
            messageBox("Profile changed", buff, MSG_OK);
            rc = MSG_IS_CANCEL;
         }
         else {
            snprintf(buff, sizeof(buff), "%s\n\nSave Profile changes?", tempProfile.description);
            rc = messageBox("Profile changed", buff, MSG_YES_NO_CANCEL);
            if (rc == MSG_IS_YES) {
               // Update profile in NV ram
               nvProfile = tempProfile;
            }
         }
      }
      else {
         rc = MSG_IS_NO;
      }
   } while (rc == MSG_IS_CANCEL);
}
