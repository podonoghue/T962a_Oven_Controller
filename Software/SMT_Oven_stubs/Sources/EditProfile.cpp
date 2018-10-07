/**
 * @file    editProfile.cpp
 * @brief   Profile editor
 *
 *  Created on: 8Oct.,2016
 *      Author: podonoghue
 */

#include <EditProfile.h>
#include "configure.h"
#include "messageBox.h"

template<typename T> char ProfileSetting_T<T>::buff[];

static char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ- abcdefghijklmnopqrstuvwxyz";
static constexpr int LETTER_GRID_WIDTH = 14;

void ProfileNameSetting::draw() {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();

   lcd.gotoXY(10, 0);
   lcd.setInversion(true); lcd.putString(" Edit Name "); lcd.setInversion(false);

   lcd.gotoXY(0, 1*lcd.FONT_HEIGHT+3);
   lcd.putString(nameBuffer);

   // Highlight selected edit letter
   lcd.gotoXY(editPosition*lcd.FONT_WIDTH, 1*lcd.FONT_HEIGHT+3);
   lcd.setInversion(true);
   lcd.putChar(nameBuffer[editPosition]);
   lcd.setInversion(false);

   // Draw letter selection list
   for (unsigned index=0; index<sizeof(letters); index++) {
      lcd.gotoXY((index%LETTER_GRID_WIDTH)*lcd.FONT_WIDTH,
            (2+(index/LETTER_GRID_WIDTH))*lcd.FONT_HEIGHT+6);
      lcd.putChar(letters[index]);
   }

   // Highlight selected entry letter
   lcd.gotoXY((letterPosition%LETTER_GRID_WIDTH)*lcd.FONT_WIDTH,
         (2+(letterPosition/LETTER_GRID_WIDTH))*lcd.FONT_HEIGHT+6);
   lcd.setInversion(true);
   lcd.putChar(letters[letterPosition]);
   lcd.setInversion(false);

   // Draw menu
   lcd.gotoXY(0,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putLeftArrow();    lcd.putSpace(4); lcd.setInversion(false); lcd.putSpace(6);
   lcd.setInversion(true); lcd.putSpace(3); lcd.putRightArrow();   lcd.putSpace(4); lcd.setInversion(false); lcd.putSpace(6);
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

   do {
      if (needsUpdate) {
         draw();
         needsUpdate = false;
      }
      switch(buttons.getButton()) {
      case SwitchValue::SW_F1: // Left
         if (letterPosition>0) {
            letterPosition--;
            needsUpdate = true;
         }
         break;
      case SwitchValue::SW_F2: // Right
         if ((letterPosition+1)<sizeof(letters)) {
            letterPosition++;
            needsUpdate = true;
         }
         break;
      case SwitchValue::SW_F3: // Sel
         needsUpdate = true;
         nameBuffer[editPosition] = letters[letterPosition];
         changed = true;
         if (editPosition<STRING_LENGTH) {
            editPosition++;
         }
         break;
      case SwitchValue::SW_F4: // Del
         if (editPosition>0) {
            editPosition--;
         }
         nameBuffer[editPosition] = ' ';
         changed = true;
         needsUpdate = true;
         break;
      case SwitchValue::SW_S: // Exit
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
      case SwitchValue::SW_F1:
         if (selection>0) {
            selection--;
            changed = true;
         }
         break;
      case SwitchValue::SW_F2:
         if (selection<(NUM_ITEMS-1)) {
            selection++;
            changed = true;
         }
         break;
      case SwitchValue::SW_F3F4:
         needsUpdate = items[selection]->reset() || needsUpdate;
         changed = true;
         break;
      case SwitchValue::SW_F3:
         needsUpdate = items[selection]->increment() || needsUpdate;
         changed = true;
         break;
      case SwitchValue::SW_F4:
         needsUpdate = items[selection]->decrement() || needsUpdate;
         changed = true;
         break;
      case SwitchValue::SW_S:
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
 * @param[in,out] nvProfile The profile to edit
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
