/*
 * lcdTest.cpp
 *
 *  Created on: 25 Sep 2016
 *      Author: podonoghue
 */

static const uint8_t image1[(128*64)/8] = {

};

void testGraphicsTextScreen() {
   lcd->clearFrameBuffer();
   for (int line=0; line<20; line++) {
      if ((line&1)==0) {
         lcd->putString("abcdefghijklmnopqrstu\n");
      }
      else {
         lcd->putString("ABCDEFGHIJKLMNOPQRSTU\n");
      }
   }
   lcd->refreshImage();
   lcd->setGraphicMode();
   USBDM::waitMS(1000);
}

void testSmallDigits() {
   lcd->clearFrameBuffer();
   for (int line=0; line<20; line++) {
      for (int digit=0; digit<=9; digit++) {
         lcd->putSmallDigit(digit);
      }
      lcd->putChar('\n');
   }
   lcd->refreshImage();
   lcd->setGraphicMode();
   USBDM::waitMS(2000);
}

void testLines() {
   lcd->setInversion(false);
   lcd->clearFrameBuffer();
   for (int y=7; y<lcd->LCD_HEIGHT; y+=8) {
      lcd->drawHorizontalLine(y);
   }
   for (int x=0; x<lcd->LCD_WIDTH; x+=10) {
      lcd->drawVerticalLine(x);
   }
   lcd->refreshImage();
   lcd->setGraphicMode();
   USBDM::waitMS(2000);

   lcd->setInversion(true);
   lcd->clearFrameBuffer();
   for (int y=7; y<lcd->LCD_HEIGHT; y+=8) {
      lcd->drawHorizontalLine(y);
   }
   for (int x=0; x<lcd->LCD_WIDTH; x+=10) {
      lcd->drawVerticalLine(x);
   }
   lcd->refreshImage();
   lcd->setGraphicMode();
   USBDM::waitMS(2000);
}

void testDots() {
   lcd->setInversion(false);
   lcd->clearFrameBuffer();
   for (int y=7; y<lcd->LCD_HEIGHT; y+=8) {
      for (int x=0; x<lcd->LCD_WIDTH; x+=10) {
         lcd->drawPixel(x,y);
      }
   }
   lcd->refreshImage();
   lcd->setGraphicMode();
   USBDM::waitMS(2000);
   lcd->setInversion(true);
   lcd->clearFrameBuffer();
   for (int y=7; y<lcd->LCD_HEIGHT; y+=8) {
      for (int x=0; x<lcd->LCD_WIDTH; x+=10) {
         lcd->drawPixel(x,y);
      }
   }
   lcd->refreshImage();
   lcd->setGraphicMode();
   USBDM::waitMS(2000);
}



