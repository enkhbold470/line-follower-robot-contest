// ============================================================
//  Display.h — SSD1306 0.96" 128×64 OLED on I2C
//  Uses Adafruit_SSD1306 + Adafruit_GFX.
// ============================================================
#pragma once
#include <Arduino.h>

namespace Display {
  void begin();

  // size: 0 = small (~10px), 1 = medium (~14px), 2 = large (~24px, default)
  void splash(const char* line, int size = 2);
  void toast(const char* msg);                            // 1-line popup
  void drawMenu(const char* const* items, int count,
                int sel, const bool* locked, bool unlocked);

  void drawCard(const char* name, const char* title, const char* contact);
  void drawBadge(const char* line1, const char* line2,
                 uint32_t tickMs);                        // animated
  void drawIRTest(const int normalized[4], int pos);
  void drawMotorTest(int a, int b);
  void drawBLE(bool connected);
  void drawLineFollow(int pos, int leftSpd, int rightSpd, bool invert);
  void drawCalibrate(uint32_t elapsedMs);
  void drawPIDTune(float kp, float ki, float kd);
  void drawCustom(const char* payload);                   // text from BLE
  void drawRaw(const String& s);
}
