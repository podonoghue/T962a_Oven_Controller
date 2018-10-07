/**
 * @file messageBox.cpp
 * @brief   Message box for LCD
 *
 *  Created on: 8Oct.,2016
 *      Author: podonoghue
 */

#include "math.h"
#include "lcd_st7920.h"
#include "messageBox.h"
#include "configure.h"

static SwitchValue waitForPress(int acceptableKeys) {
   SwitchValue keyPress;
   do {
      keyPress = buttons.getButton();
   } while ((keyPress&acceptableKeys) == 0);
   return keyPress;
};

/**
 * Writes a full screen message to LCD
 *
 * @param title      Title for screen
 * @param message    Message to display
 * @param selection  Key selection to display at bottom of screen
 *
 * @note Waits for valid key press before returning.
 *
 * @return Value reflecting key pressed
 */
MessageBoxResult messageBox(const char *title, const char *message, MessageBoxSelection selection) {
   lcd.setInversion(false);
   lcd.clearFrameBuffer();
   lcd.gotoXY(0,0);
   lcd.putSpace(5); lcd.setInversion(true); lcd.putChar(' '); lcd.putString(title); lcd.putChar(' '); lcd.setInversion(false);

   lcd.gotoXY(0, 12+lcd.FONT_HEIGHT);
   lcd.putString(message);

   if (selection==MSG_OK) {
      lcd.gotoXY(lcd.LCD_WIDTH-(4*lcd.FONT_WIDTH+4)+4,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putString(" OK "); lcd.setInversion(false);
      lcd.refreshImage();
      lcd.setGraphicMode();
      waitForPress(SwitchValue::SW_S);
      return MSG_IS_OK;
   }
   else if (selection==MSG_OK_CANCEL) {
      lcd.gotoXY(lcd.LCD_WIDTH-(12*lcd.FONT_WIDTH+2*4)+4,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putString(" OK "); lcd.setInversion(false);
      lcd.putSpace(4);
      lcd.setInversion(true); lcd.putString(" CANCEL "); lcd.setInversion(false);
      lcd.refreshImage();
      lcd.setGraphicMode();
      SwitchValue sw = waitForPress(SwitchValue::SW_F4|SwitchValue::SW_S);
      return (sw==SwitchValue::SW_S)?MSG_IS_CANCEL:MSG_IS_OK;
   }
   else if (selection==MSG_YES_NO) {
      lcd.gotoXY(lcd.LCD_WIDTH-(9*lcd.FONT_WIDTH+2*4)+4,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
      lcd.setInversion(true); lcd.putString(" YES "); lcd.setInversion(false);
      lcd.putSpace(4);
      lcd.setInversion(true); lcd.putString(" NO "); lcd.setInversion(false);
      lcd.refreshImage();
      lcd.setGraphicMode();
      SwitchValue sw = waitForPress(SwitchValue::SW_F4|SwitchValue::SW_S);
      return (sw==SwitchValue::SW_S)?MSG_IS_NO:MSG_IS_YES;
   }
   else // MSG_YES_NO_CANCEL
      lcd.gotoXY(lcd.LCD_WIDTH-(11*lcd.FONT_WIDTH+9*4)+4,lcd.LCD_HEIGHT-lcd.FONT_HEIGHT);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putString("YES"); lcd.putSpace(4); lcd.setInversion(false);
   lcd.putSpace(4);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putString("NO"); lcd.putSpace(4); lcd.setInversion(false);
   lcd.putSpace(4);
   lcd.setInversion(true); lcd.putSpace(4); lcd.putString("CANCEL"); lcd.putSpace(4); lcd.setInversion(false);
   lcd.refreshImage();
   lcd.setGraphicMode();
   SwitchValue sw = waitForPress(SwitchValue::SW_F3|SwitchValue::SW_F4|SwitchValue::SW_S);
   return (sw==SwitchValue::SW_S)?MSG_IS_CANCEL:((sw==SwitchValue::SW_F4)?MSG_IS_NO:MSG_IS_YES);
}

//static void test1(MessageBoxResult selection) {
//   lcd.gotoXY(10,lcd.LCD_HEIGHT-2*lcd.FONT_HEIGHT);
//   switch(selection) {
//   case MessageBoxResult::MSG_IS_OK :     lcd.putString("MSG_IS_OK");      break;
//   case MessageBoxResult::MSG_IS_YES :    lcd.putString("MSG_IS_YES");     break;
//   case MessageBoxResult::MSG_IS_NO :     lcd.putString("MSG_IS_NO");      break;
//   case MessageBoxResult::MSG_IS_CANCEL : lcd.putString("MSG_IS_CANCEL");  break;
//   }
//   lcd.refreshImage();
//   lcd.setGraphicMode();
//   while (buttons.getButton() == SW_NONE) {
//      __asm__("nop");
//   }
//}
//
//static void test2() {
//   test1(messageBox("Error", "MSG_OK -\n test",            MSG_OK));
//   test1(messageBox("Error", "MSG_OK_CANCEL -\n test",     MSG_OK_CANCEL));
//   test1(messageBox("Error", "MSG_YES_NO -\n test",        MSG_YES_NO));
//   test1(messageBox("Error", "MSG_YES_NO_CANCEL -\n test", MSG_YES_NO_CANCEL));
//}

/**
 * Checks if thermocouples are present and enabled\n
 * Displays a message and waits for key press if not.
 *
 * @return true => At least one thermocouple is active
 */
bool checkThermocouples() {
   if (std::isnan(getTemperature())) {
      messageBox("Error", "No enabled\nthermocouples present");
      return false;
   }
   return true;
}




