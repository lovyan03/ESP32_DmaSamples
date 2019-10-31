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

const uint16_t BALL_MAX = 1000;
static uint16_t tft_width = 320;
static uint16_t tft_height = 240;


struct MainClass
{
  struct ball_info_t {
    int16_t x;
    int16_t y;
    int16_t r;
    uint16_t color;
    int16_t dx;
    int16_t dy;

    void move()
    {
      x += dx;
      y += dy;
      if (x < 0) {
        x = 0;
        if (dx < 0) dx = - dx;
      } else if (x >= tft_width) {
        x = tft_width -1;
        if (dx > 0) dx = - dx;
      }
      if (y < 0) {
        y = 0;
        if (dy < 0) dy = - dy;
      } else if (y >= tft_height) {
        y = tft_height -1;
        if (dy > 0) dy = - dy;
      }
    }
  };


  TFT_eSPI* _tft;
  ball_info_t _balls[BALL_MAX];
  TFT_eSprite _sprite;
  DMA_eSprite _sprites[2];
  DMADrawer _dma;
public:
  MainClass(TFT_eSPI* tft)
  : _tft (tft)
  , _sprite(tft)
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
      a = &_balls[i];
      a->color = (uint16_t)rand() | 0x8210;
      a->x = random(0, tft_width);
      a->y = random(0, tft_height);
      a->dx = random(1, 4) * (i & 1 ? 1 : -1);
      a->dy = random(1, 4) * (i & 2 ? 1 : -1);
      a->r = 8 + (i & 0x07);
    }
  }

  void close()
  {
    _dma.wait();
    _sprites[0].deleteSprite();
    _sprites[1].deleteSprite();
    _sprite.deleteSprite();
    _tft->fillScreen(0);
  }

  void setupSprite()
  {
    _sprite.createSprite(tft_width, max(120, (tft_height+1) / 2));
  }

  void setupDMA()
  {
    uint16_t h = max(80, (tft_height + 2) / 3);
    _sprites[0].createDMA(tft_width, h);
    _sprites[1].createDMA(tft_width, h);
  }

  void loop(uint16_t ball_count, uint16_t fps)
  {
    ball_info_t *a;
    for (int i = 0; i != ball_count; i++) {
      a = &_balls[i];
      _tft->drawCircle(a->x, a->y, a->r, TFT_BLACK);
      a->move();
      _tft->drawCircle(a->x, a->y, a->r, a->color);
    }
  }

  void loopSprite(uint16_t ball_count, uint16_t fps)
  {
    ball_info_t *a;
    for (int i = 0; i != ball_count; i++) {
      _balls[i].move();
    }
    int16_t sprite_height = _sprite.height();
    for (int y = 0; y < tft_height; y += sprite_height) {
      _sprite.fillSprite(TFT_BLACK);
      for (int i = 0; i != ball_count; i++) {
        a = &_balls[i];
        if (( a->y + a->r >= y ) && ( a->y - a->r <= y + sprite_height ))
          _sprite.drawCircle(a->x, a->y - y, a->r, a->color);
      }

      if (y == 0) {
        _sprite.setCursor(0,0);
        _sprite.setTextFont(2);
        _sprite.setTextColor(TFT_WHITE);
        _sprite.printf("TFT_eSprite  ball:%d  fps:%d", ball_count, fps);
      }

      _sprite.pushSprite(0, y);
    }
  }

  void loopDMA(uint16_t ball_count, uint16_t fps)
  {
    static uint8_t flip = 0;

    ball_info_t *a;
    for (int i = 0; i != ball_count; i++) {
      _balls[i].move();
    }
    int16_t sprite_height = _sprites[0].height();
    for (int y = 0; y < tft_height; y += sprite_height) {
      flip = flip ? 0 : 1;
      _sprites[flip].fillSprite(TFT_BLACK);
      for (int i = 0; i != ball_count; i++) {
        a = &_balls[i];
        if (( a->y + a->r >= y ) && ( a->y - a->r <= y + sprite_height ))
          _sprites[flip].drawCircle(a->x, a->y - y, a->r, a->color);
      }

      if (y == 0) {
        _sprites[flip].setCursor(0,0);
        _sprites[flip].setTextFont(2);
        _sprites[flip].setTextColor(TFT_WHITE);
        _sprites[flip].printf("DMA  ball:%d  fps:%d", ball_count, fps);
      }

      _dma.draw(0, y, &_sprites[flip]);
    }
  }
};

#endif
