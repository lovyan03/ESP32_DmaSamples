#pragma GCC optimize ("O3")

//  #define M5STACK_MPU6886
//  #define M5STACK_MPU9250
//  #define M5STACK_MPU6050
//  #define M5STACK_200Q

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
uint16_t ball_count = 100;
uint32_t sec, psec;
uint16_t fps = 0, frame_count = 0;


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
}

void loop(void)
{
#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE) || defined(ARDUINO_M5Stick_C)
  M5.update();
  if (M5.BtnB.isPressed()) {
    ball_count = max(10, ball_count - 10);
  } else
  if (M5.BtnC.isPressed()) {
    ball_count = min(1000, ball_count + 10);
  }
#endif

  Main.loop(ball_count, fps);

  frame_count++;
  sec = millis() / 1000;
  if (psec != sec) {
    psec = sec;
    fps = frame_count;
    Serial.printf("ball:%d fps:%d\r\n", ball_count, frame_count);
    frame_count = 0;
  }
}

