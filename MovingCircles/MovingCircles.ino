#pragma GCC optimize ("O3")

#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE)
  #include <M5Stack.h>
  #include <M5StackUpdater.h>     // https://github.com/tobozo/M5Stack-SD-Updater/
  TFT_eSPI &Tft = M5.Lcd;
#elif defined(ARDUINO_M5Stick_C)
  #include <M5StickC.h>
  TFT_eSPI &Tft = M5.Lcd;
#else
  #include <TFT_eSPI.h>  // https://github.com/Bodmer/TFT_eSPI
  TFT_eSPI Tft;
#endif

#include "Main.h"

MainClass Main(&Tft);
uint32_t sec, psec;
uint16_t ball_count = 100;
uint16_t fps = 0, frame_count = 0;
uint8_t mode = 2;

void setMode(uint8_t m)
{
  mode = m;
  switch (mode) {
  case 0:                     Serial.print("standard drawCircle\r\n"); break;
  case 1: Main.setupSprite(); Serial.print("Use TFT_eSprite\r\n");    break;
  case 2: Main.setupDMA();    Serial.print("Use DMA\r\n");            break;
  }
}

void setup(void)
{
#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE)
  M5.begin();
  #ifdef __M5STACKUPDATER_H
    if(digitalRead(BUTTON_A_PIN) == 0) {
       Serial.println("Will Load menu binary");
       updateFromFS(SD);
       ESP.restart();
    }
  #endif
#elif defined(ARDUINO_M5Stick_C)
  M5.begin();
  M5.Lcd.setRotation(3);
#else
  Tft.begin();
#endif

  Main.setup();
  setMode(mode);
}

void loop(void)
{
#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5Stick_C)
  M5.update();
  if (M5.BtnA.wasPressed()) { 
    Main.close();
    setMode((mode < 2) ? mode + 1 : 0);
  } else
  if (M5.BtnB.isPressed()) {
    ball_count = max(10, ball_count - 10);
  } else
  if (M5.BtnC.isPressed()) {
    ball_count = min(1000, ball_count + 10);
  }
#endif

  switch (mode) {
  default:  Main.loop(ball_count, fps);       break;
  case 1:   Main.loopSprite(ball_count, fps); break;
  case 2:   Main.loopDMA(ball_count, fps);    break;
  }

  frame_count++;
  sec = millis() / 1000;
  if (psec != sec) {
    psec = sec;
    fps = frame_count;
    Serial.printf("ball:%d fps:%d\r\n", ball_count, frame_count);
    frame_count = 0;
  }
}

