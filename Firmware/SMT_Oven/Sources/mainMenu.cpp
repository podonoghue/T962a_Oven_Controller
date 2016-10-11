/*
 * mainMenu.cpp
 *
 *  Created on: 9Oct.,2016
 *      Author: podonoghue
 */

#include <solderProfiles.h>
#include "configure.h"
#include "mainMenu.h"
#include "editProfile.h"
#include "settings.h"
#include "messageBox.h"

namespace MainMenu {

struct MenuItem {
   const char *desciption;
   void (*action)();
};

/**
 * Resets to factory defaults.\n
 * The user is prompted to confirm before doing so.
 */
static void factoryDefaults() {
   int rc = messageBox("Factory Defaults", "Reset ALL settings\nto Factory defaults?", MSG_YES_NO);
   if (rc == MSG_IS_YES) {
      // Reset all to factory defaults
      Settings::initialiseSettings();
   }
}

static MenuItem menu[] = {
      {"Manual Mode",          RunProfile::manualMode,                                 },
      {"Run Profile",          [](){RunProfile::runProfile(profiles[profileIndex]);},  },
      {"Manage Profiles",      RunProfile::profileMenu,                                },
      {"Thermocouples",        RunProfile::monitor,                                    },
      {"Settings",             [](){settings.runMenu();},                              },
      {"Factory defaults",     factoryDefaults,                           },
};

static constexpr int NUM_ITEMS = sizeof(menu)/sizeof(menu[0]);

static int selection = 0;
static int offset    = 0;

static constexpr int MAX_VISIBLE_ITEMS = (lcd.LCD_HEIGHT/8)-2;

static void drawScreen() {
   // Adjust so selected item is visible
   if ((selection-offset) >= MAX_VISIBLE_ITEMS) {
      offset++;
   }
   if ((selection < offset)) {
      offset--;
   }
   lcd.setInversion(false);  lcd.clearFrameBuffer();
   lcd.setInversion(true);   lcd.putString("  Main Menu\n"); lcd.setInversion(false);
   for (int item=0; item<NUM_ITEMS; item++) {
      if (item<offset) {
         continue;
      }
      if (item>(offset+MAX_VISIBLE_ITEMS)) {
         continue;
      }
      lcd.setInversion(item == selection);
      lcd.gotoXY(0, (item-offset+1)*lcd.FONT_HEIGHT);
      lcd.putString(menu[item].desciption);
   }
   lcd.setInversion(false);
   lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(false); lcd.putSpace(4);
   lcd.setInversion(true);  lcd.putString(" ");     lcd.putUpArrow();   lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
   lcd.setInversion(true);  lcd.putString(" ");     lcd.putDownArrow(); lcd.putString(" "); lcd.setInversion(false); lcd.putSpace(3);
   lcd.setInversion(false); lcd.putSpace(48);
   lcd.setInversion(true);  lcd.putString(" SEL "); lcd.setInversion(false);            lcd.putSpace(3);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

void run() {
   bool changed = true;
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
      case SW_S:
         menu[selection].action();
         changed = true;
         break;
      default:
         break;
      }
      __WFI();
   }
}
};



