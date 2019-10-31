#ifndef _MAIN_H_
#define _MAIN_H_

#pragma GCC optimize ("O3")

#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE)
  #include <M5Stack.h>
#elif defined(ARDUINO_M5Stick_C)
  #include <M5StickC.h>
#else
  #include <TFT_eSPI.h>  // https://github.com/Bodmer/TFT_eSPI
#endif

#include "DMADrawer.h"

const uint16_t SHIFTSIZE = 8;
const uint16_t BALL_MAX = 1000;
static uint16_t tft_width = 320;
static uint16_t tft_height = 240;


struct MainClass
{
  struct ball_info_t {
    int32_t x;
    int32_t y;
    int32_t dx;
    int32_t dy;
    int32_t r;
    int32_t m;
    uint16_t color;
  };


  TFT_eSPI* _tft;
  ball_info_t _balls[2][BALL_MAX];
  DMA_eSprite _sprites[2];
  DMADrawer _dma;
  uint16_t _ball_count = 0, _fps = 0;
  int imux = 0, imuy = 0;
  volatile bool _is_running;
  volatile uint16_t _draw_count;
  volatile uint16_t _loop_count;

public:
  MainClass(TFT_eSPI* tft)
  : _tft (tft)
  {}

  void setup()
  {
    setup_t s;
    _tft->getSetup(s);
    _dma.init(s);
    tft_width = _tft->width();
    tft_height = _tft->height();
    ball_info_t *a;
    for (uint16_t i = 0; i < BALL_MAX; i++) {
      a = &_balls[0][i];
      a->color = (uint16_t)rand() | 0x8210;
      a->x = random(0, tft_width << SHIFTSIZE);
      a->y = random(0, tft_height << SHIFTSIZE);
      a->dx = random(1 << SHIFTSIZE, 3 << SHIFTSIZE) * (i & 1 ? 1 : -1);
      a->dy = random(1 << SHIFTSIZE, 3 << SHIFTSIZE) * (i & 2 ? 1 : -1);
      a->r = (4 + (i & 0x07)) << SHIFTSIZE;
      a->m = 4 + (i & 0x07);
    }

    uint16_t h = max(80, (tft_height + 2) / 3);
    _sprites[0].createDMA(tft_width, h);
    _sprites[1].createDMA(tft_width, h);

#if defined(M5STACK_MPU6886) || defined(M5STACK_MPU9250) || defined(M5STACK_MPU6050) || defined(M5STACK_200Q)
    M5.Power.begin();
    M5.IMU.Init();
#endif

    _is_running = true;
    _draw_count = 0;
    _loop_count = 0;
    xTaskCreatePinnedToCore(taskDraw, "taskDraw", 4096, this, 1, NULL, 0);
  }


  void close()
  {
    _dma.wait();
    _sprites[0].deleteSprite();
    _sprites[1].deleteSprite();
    _tft->fillScreen(0);
  }


  void loop(uint16_t ball_count, uint16_t fps)
  {
    float e = 1;

#if defined(M5STACK_MPU6886) || defined(M5STACK_MPU9250) || defined(M5STACK_MPU6050) || defined(M5STACK_200Q)
    e = 0.99;

    float accX = 0.0F;
    float accY = 0.0F;
    float accZ = 0.0F;

    float gyroX = 0.0F;
    float gyroY = 0.0F;
    float gyroZ = 0.0F;

    float pitch = 0.0F;
    float roll  = 0.0F;
    float yaw   = 0.0F;
    M5.IMU.getGyroData(&gyroX,&gyroY,&gyroZ);
    M5.IMU.getAccelData(&accX,&accY,&accZ);
    M5.IMU.getAhrsData(&pitch,&roll,&yaw);
    imux = pitch;
    imuy = roll;
#endif
    while (_loop_count != _draw_count) taskYIELD();
    _loop_count++;

    ball_info_t *a, *b, *balls;
    int32_t rr, len, vx2vy2;
    float vx, vy, distance, t;
    float adx, ady, amx, amy, arx, ary, bdx, bdy, bmx, bmy, brx, bry;

    size_t f = _loop_count & 1;
    balls = a = &_balls[f][0];
    b = &_balls[1 - f][0];
    for (int i = 0; i != ball_count; i++) {
      a[i] = b[i];
    }

    for (int i = 0; i != ball_count; i++) {
      a = &balls[i];
      a->dx += imux;
      a->dy += imuy;
      a->x += a->dx;
      a->y += a->dy;
      if (a->x < a->r) {
        a->x = a->r;
        if (a->dx < 0) a->dx = - a->dx;
      } else if (a->x >= (tft_width << SHIFTSIZE) - a->r) {
        a->x = (tft_width << SHIFTSIZE) - a->r -1;
        if (a->dx > 0) a->dx = - a->dx;
      }
      if (a->y < a->r) {
        a->y = a->r;
        if (a->dy < 0) a->dy = - a->dy;
      } else if (a->y >= (tft_height << SHIFTSIZE) - a->r) {
        a->y = (tft_height << SHIFTSIZE) - a->r -1;
        if (a->dy > 0) a->dy = - a->dy;
      }
      for (int j = i + 1; j != ball_count; j++) {
        b = &balls[j];

        rr = a->r + b->r;
        vx = a->x - b->x;
        if (abs(vx) > rr) continue;
        vy = a->y - b->y;
        if (abs(vy) > rr) continue;

        len = sqrt(vx * vx + vy * vy);
        if (len == 0.0) continue;
        if (len >= rr) continue;
        distance = (rr - len) >> 1;
        vx *= distance / len;
        vy *= distance / len;

        a->x += vx;
        a->y += vy;
        b->x -= vx;
        b->y -= vy;

        vx = b->x - a->x;
        vy = b->y - a->y;
        vx2vy2 = vx * vx + vy * vy;

        t = -(vx * a->dx + vy * a->dy) / vx2vy2;
        arx = a->dx + vx * t;
        ary = a->dy + vy * t;

        t = -(-vy * a->dx + vx * a->dy) / vx2vy2;
        amx = a->dx - vy * t;
        amy = a->dy + vx * t;

        t = -(vx * b->dx + vy * b->dy) / vx2vy2;
        brx = b->dx + vx * t;
        bry = b->dy + vy * t;

        t = -(-vy * b->dx + vx * b->dy) / vx2vy2;
        bmx = b->dx - vy * t;
        bmy = b->dy + vx * t;

        adx = (a->m * amx + b->m * bmx + bmx * e * b->m - amx * e * b->m) / (a->m + b->m);
        bdx = - e * (bmx - amx) + adx;
        ady = (a->m * amy + b->m * bmy + bmy * e * b->m - amy * e * b->m) / (a->m + b->m);
        bdy = - e * (bmy - amy) + ady;

        a->dx = adx + arx;
        a->dy = ady + ary;
        b->dx = bdx + brx;
        b->dy = bdy + bry;
      }
    }
    _fps = fps;
    _ball_count = ball_count;
  }


  static void taskDraw(void* arg)
  {
    MainClass* me = (MainClass*) arg;
    volatile uint16_t &loop_count(me->_loop_count);
    volatile uint16_t &draw_count(me->_draw_count);

    static int flip = 0;
    ball_info_t *balls;
    ball_info_t *a;
    DMA_eSprite *sprite;

    int16_t sprite_height = me->_sprites[0].height();
    int16_t ay, ar;
    while ( me->_is_running ) {
      if (loop_count == draw_count) {
        taskYIELD();
        continue;
      }
      balls = &me->_balls[draw_count & 1][0];
      for (int y = 0; y < tft_height; y += sprite_height) {
        flip = flip ? 0 : 1;
        sprite = &(me->_sprites[flip]);
        sprite->fillSprite(TFT_BLACK);
        for (int i = 0; i < me->_ball_count; i++) {
          a = &balls[i];
          ay = (a->y >> SHIFTSIZE);
          ar = (a->r >> SHIFTSIZE);
          if (( ay + ar >= y ) && ( ay - ar <= y + sprite_height ))
            sprite->drawCircle((a->x >> SHIFTSIZE), ay - y, ar, a->color);
        }

        if (y == 0) {
          sprite->setCursor(0,0);
          sprite->setTextFont(2);
          sprite->setTextColor(TFT_WHITE);
          sprite->printf("ball:%d  fps:%d", me->_ball_count, me->_fps);
        }

        me->_dma.draw(0, y, sprite);
      }
      draw_count++;
    }
    vTaskDelete(NULL);
  }
};

#endif
