/*
 * editProfile.cpp
 *
 *  Created on: 8Oct.,2016
 *      Author: podonoghue
 */

#include "configure.h"
#include "editProfile.h"
#include "messageBox.h"

template<typename T> char ProfileSetting_T<T>::buff[];

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
