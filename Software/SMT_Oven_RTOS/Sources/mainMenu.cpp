/**
 * @file mainMenu.cpp
 * @brief   Main menu for Oven
 *
 *  Created on: 9Oct.,2016
 *      Author: podonoghue
 */

#include "manageProfiles.h"
#include "SolderProfile.h"
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
   int rc = messageBox("Factory Defaults",
         "Reset ALL settings\n"
         "including profiles\n"
         "to Factory defaults?", MSG_YES_NO);
   if (rc == MSG_IS_YES) {
      // Reset all to factory defaults
      Settings::initialiseSettings();
   }
}

static const MenuItem menu[] {
      {"Manual Mode",          RunProfile::manualMode,        },
      {"Run Profile",          RunProfile::runProfile,        },
      {"Manage Profiles",      ManageProfiles::profileMenu,   },
      {"Thermocouples",        Monitor::monitor,              },
      {"Settings",             [](){settings.runMenu();},     },
      {"Factory defaults",     factoryDefaults,               },
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
   lcd.setInversion(true);   lcd.write("  Main Menu\n"); lcd.setInversion(false);
   for (int item=0; item<NUM_ITEMS; item++) {
      if (item<offset) {
         continue;
      }
      if (item>(offset+MAX_VISIBLE_ITEMS)) {
         continue;
      }
      lcd.setInversion(item == selection);
      lcd.gotoXY(0, (item-offset+1)*lcd.FONT_HEIGHT);
      lcd.write(menu[item].desciption);
   }
   lcd.setInversion(false);
   lcd.gotoXY(0, lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true);  lcd.putSpace(8);     lcd.putUpArrow();   lcd.putSpace(9); lcd.setInversion(false); lcd.putSpace(5);
   lcd.setInversion(true);  lcd.putSpace(8);     lcd.putDownArrow(); lcd.putSpace(9); lcd.setInversion(false); lcd.putSpace(5);
   lcd.setInversion(false); lcd.putSpace(42);
   lcd.setInversion(true);  lcd.write(" SEL "); lcd.setInversion(false);

   lcd.refreshImage();
   lcd.setGraphicMode();
}

void displayBusy() {
   lcd.setInversion(false);  lcd.clearFrameBuffer();

   lcd.gotoXY(0, 20);
   lcd.write("  Locked for \n");
   lcd.write("  Remote use");
   lcd.refreshImage();
   lcd.setGraphicMode();
}

void run() {
   bool changed = true;
   osStatus status;
   for(;;) {
      if (changed) {
         drawScreen();
         changed = false;
      }
      SwitchValue button = buttons.getButton(100);
      if (button != SwitchValue::SW_NONE) {
         // Try to get mutex - no wait so we can update display if busy
         status = interactiveMutex.wait(0);
         if (status != osOK) {
            displayBusy();
            // Wait again until we are successful
            changed = true;
            interactiveMutex.wait();
            // Release immediately as we will retry in loop
            interactiveMutex.release();
            // Discard key
            continue;
         }
         switch(button) {
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
         case SwitchValue::SW_S:
            menu[selection].action();
            changed = true;
            break;
         default:
            break;
         }
         interactiveMutex.release();
      }
   }
}
};
