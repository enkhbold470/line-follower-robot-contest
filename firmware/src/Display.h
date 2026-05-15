// ============================================================
//  Display.h — SSD1306 0.96" 128×64 OLED on I2C via U8g2.
// ============================================================
#pragma once
#include <Arduino.h>

namespace Display {
  void begin();

  // size: 0 = small, 1 = medium, 2 = large
  void splash(const char* line, int size = 2);
  void toast(const char* msg);
  void drawMenu(const char* const* items, int count,
                int sel, const bool* locked, bool unlocked);

  void drawCard(const char* name, const char* title, const char* contact);
  void drawBadge(const char* line1, const char* line2, uint32_t tickMs);
  void drawBLE(bool connected);
  void drawCustom(const char* payload);
  void drawRaw(const String& s);

  // ---- games ----
  void drawSnake(const uint8_t* bx, const uint8_t* by, int len,
                 int fx, int fy, int cell, int cols, int rows,
                 int score, bool alive);
  void drawDino(float y, int obsX, int obsW, int obsH,
                int score, bool alive);
  void drawReaction(const char* state, uint32_t reactMs, int best);
  void drawDice(int d1, int d2, bool rolling);
  void drawMagic8(const char* answer, bool shaking, uint32_t tickMs);

  // ---- utils ----
  void drawClock(uint8_t h, uint8_t m, uint8_t s, bool isSet);
  void drawStopwatch(uint32_t elapsedMs, bool running);
  void drawCountdown(uint32_t remainMs, bool running, uint32_t targetMs);
}
