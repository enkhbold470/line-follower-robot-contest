// ============================================================
//  Display.cpp - SSD1306 128x64 OLED via U8g2 (full buffer)
//
//  Bus: HW I2C on Wire (SDA=PIN_SDA, SCL=PIN_SCL @ 400 kHz)
//  Addr: 0x3C (7-bit) / 0x78 (8-bit write)
//
//  Font convention:
//    FONT_S = ~6x10  (replaces Adafruit setTextSize(1))
//    FONT_M = ~14px  (replaces setTextSize(2))
//    FONT_L = ~24px  (replaces setTextSize(3))
//
//  setFontPosTop() is enabled globally so setCursor(x,y) treats y
//  as the top of the glyph (matches the previous Adafruit code's
//  coordinate system).
// ============================================================
#include "Display.h"
#include "Pins.h"
#include <Wire.h>
#include <U8g2lib.h>

#define W           128
#define H            64
#define OLED_ADDR  0x3C

// Dual-color 0.96" panels (common Chinese modules) have a yellow
// strip in the top 16 rows and blue rows 16..63. Any single text
// element must stay entirely inside ONE band - never start at
// y < YELLOW_H and extend past it (the glyph would be sliced
// yellow-on-top, blue-on-bottom).
#define YELLOW_H    16

#define FONT_S  u8g2_font_6x10_tr     // ~10px tall - fits in yellow strip
#define FONT_M  u8g2_font_ncenB14_tr  // ~14px - blue only
#define FONT_L  u8g2_font_logisoso24_tr // ~24px - blue only

// U8G2_R2 = 180-degree rotation (flips X and Y).
// Use R0/R1/R2/R3 for 0/90/180/270 deg, or U8G2_MIRROR for horiz flip only.
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// ---- helpers ----
static inline int strW(const char* s) { return oled.getStrWidth(s); }
static inline int fontH() { return oled.getMaxCharHeight(); }

static void centeredStr(int boxW, int boxH, int boxX, int boxY,
                        const char* s) {
  int tw = strW(s);
  int th = fontH();
  oled.setCursor(boxX + (boxW - tw) / 2, boxY + (boxH - th) / 2);
  oled.print(s);
}

namespace Display {

void begin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  oled.setI2CAddress(OLED_ADDR << 1);   // U8g2 wants 8-bit form (0x78)
  if (!oled.begin()) {
    Serial.println(F("[oled] init fail"));
  }
  oled.setFontPosTop();                 // y = top of glyph
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.clearBuffer();
  oled.sendBuffer();
}

void splash(const char* line, int size) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  // pick font by size; FONT_L spans blue zone anyway (cy=20).
  // FONT_M / FONT_S also center safely in blue zone.
  const uint8_t* font = (size <= 0) ? FONT_S
                      : (size == 1) ? FONT_M
                                    : FONT_L;
  oled.setFont(font);
  centeredStr(W, H, 0, 0, line);
  oled.sendBuffer();
}

void toast(const char* msg) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  oled.drawFrame(0, 0, W, H);
  oled.setFont(FONT_S);
  centeredStr(W, H, 0, 0, msg);
  oled.sendBuffer();
}

// menu: yellow header bar (full 16px strip) + 4 blue rows
void drawMenu(const char* const* items, int count, int sel,
              const bool* locked, bool unlocked) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  // header fills entire yellow band (0..15) so it reads as solid yellow
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(3, 3);
  oled.print(F("MENU"));
  oled.setCursor(W - 22, 3);
  oled.print(unlocked ? F("PRO") : F("BAS"));
  oled.setDrawColor(1);

  // 4 rows entirely in blue band: y = 18, 30, 42, 54
  // Each row 12px tall (FONT_S ~10px glyph + 2px padding).
  int visible = 4;
  int top = sel - 1;
  if (top < 0) top = 0;
  if (top > count - visible) top = max(0, count - visible);

  for (int row = 0; row < visible && (top + row) < count; row++) {
    int idx  = top + row;
    int y    = 18 + row * 12;
    bool isSel = (idx == sel);
    if (isSel) {
      oled.drawBox(0, y - 2, W, 11);
      oled.setDrawColor(0);
    } else {
      oled.setDrawColor(1);
    }
    oled.setCursor(4, y);
    oled.print(items[idx]);
    if (locked[idx] && !unlocked) {
      oled.setCursor(W - 8, y);
      oled.print('*');
    }
    oled.setDrawColor(1);
  }
  oled.sendBuffer();
}

void drawCard(const char* name, const char* title, const char* contact) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  // yellow strip header
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(4, 3);
  oled.print(F("BUSINESS CARD"));
  oled.setDrawColor(1);

  // blue zone frame
  oled.drawFrame(0, YELLOW_H, W, H - YELLOW_H);

  // name (big) centered horizontally
  oled.setFont(FONT_M);
  {
    int tw = strW(name);
    int x  = (W - tw) / 2;
    if (x < 4) x = 4;
    oled.setCursor(x, 20);
    oled.print(name);
  }

  // title centered, FONT_S
  oled.setFont(FONT_S);
  {
    int tw = strW(title);
    int x  = (W - tw) / 2;
    if (x < 4) x = 4;
    oled.setCursor(x, 38);
    oled.print(title);
  }

  // divider
  oled.drawHLine(8, 50, W - 16);

  // contact (email OR website, whichever caller passes) - centered
  {
    int tw = strW(contact);
    int x  = (W - tw) / 2;
    if (x < 4) x = 4;
    oled.setCursor(x, 53);
    oled.print(contact);
  }

  oled.sendBuffer();
}

void drawBadge(const char* line1, const char* line2, uint32_t tickMs) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  // yellow header
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(4, 3);
  oled.print(F("EVENT BADGE"));
  oled.setDrawColor(1);

  // blue zone: marquee FONT_M sits at y=20..34, entirely below yellow
  oled.setFont(FONT_M);
  int tw    = strW(line1);
  int total = tw + W;
  int off   = (tickMs / 40) % total;
  oled.setCursor(W - off, 20);
  oled.print(line1);

  oled.setFont(FONT_S);
  centeredStr(W, 12, 0, 42, line2);

  int dots = (tickMs / 250) % 4;
  for (int i = 0; i < dots; i++)
    oled.drawDisc(W / 2 - 12 + i * 8, 58, 2);

  oled.sendBuffer();
}

void drawIRTest(const int s[4], int pos) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.setCursor(0, 0); oled.print(F("IR TEST"));

  for (int i = 0; i < 4; i++) {
    int barH = map(s[i], 0, 1000, 0, 40);
    int x = 8 + i * 28;
    oled.drawFrame(x, 14, 18, 42);
    oled.drawBox(x + 1, 14 + (42 - barH), 16, barH);
    oled.setCursor(x + 6, 54);
    oled.print(i + 1);
  }
  oled.setCursor(80, 0);
  oled.print(F("pos:"));
  oled.print(pos);
  oled.sendBuffer();
}

void drawMotorTest(int a, int b) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.setCursor(0, 0);  oled.print(F("MOTOR TEST"));
  oled.setCursor(0, 16); oled.print(F("A: ")); oled.print(a);
  oled.setCursor(0, 28); oled.print(F("B: ")); oled.print(b);

  int aw = map(abs(a), 0, 255, 0, 100);
  int bw = map(abs(b), 0, 255, 0, 100);
  oled.drawFrame(20, 18, 102, 6);
  oled.drawBox(21, 19, aw, 4);
  oled.drawFrame(20, 30, 102, 6);
  oled.drawBox(21, 31, bw, 4);
  oled.setCursor(0, 52); oled.print(F("short=cycle dbl=exit"));
  oled.sendBuffer();
}

void drawBLE(bool connected) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  // yellow strip: "BLE" title in FONT_S
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(4, 3); oled.print(F("BLE STATUS"));
  oled.setDrawColor(1);

  // blue zone
  oled.setFont(FONT_S);
  oled.setCursor(0, 22); oled.print(connected ? F("CONNECTED") : F("advertising"));
  oled.setCursor(0, 36); oled.print(F("name: ROBOT"));
  oled.setCursor(0, 50); oled.print(F("open web app"));

  oled.drawCircle(110, 40, 6);
  if (connected) oled.drawDisc(110, 40, 3);
  oled.sendBuffer();
}

void drawLineFollow(int pos, int l, int r, bool invert) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.setCursor(0, 0);
  oled.print(invert ? F("LINE  W/B") : F("LINE  B/W"));

  oled.drawFrame(4, 16, 120, 10);
  int x = map(constrain(pos, -1500, 1500), -1500, 1500, 5, 122);
  oled.drawBox(x - 2, 17, 4, 8);
  oled.drawVLine(64, 14, 14);

  oled.setCursor(0, 34);  oled.print(F("L:")); oled.print(l);
  oled.setCursor(64, 34); oled.print(F("R:")); oled.print(r);
  oled.setCursor(0, 50);  oled.print(F("pos:")); oled.print(pos);
  oled.sendBuffer();
}

void drawCalibrate(uint32_t ms) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.setCursor(0, 0);  oled.print(F("CALIBRATING"));
  oled.setCursor(0, 16); oled.print(F("sweep over line"));
  oled.setCursor(0, 28); oled.print(F("& off line..."));
  int p = (ms / 30) % 120;
  oled.drawFrame(4, 44, 120, 10);
  oled.drawBox(6, 46, p, 6);
  oled.setCursor(0, 56); oled.print(F("long-press=done"));
  oled.sendBuffer();
}

void drawPIDTune(float kp, float ki, float kd) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.setCursor(0, 0);  oled.print(F("PID TUNE (BLE)"));
  oled.setCursor(0, 18); oled.print(F("Kp: ")); oled.print(kp, 3);
  oled.setCursor(0, 30); oled.print(F("Ki: ")); oled.print(ki, 3);
  oled.setCursor(0, 42); oled.print(F("Kd: ")); oled.print(kd, 3);
  oled.setCursor(0, 54); oled.print(F("edit via web app"));
  oled.sendBuffer();
}

void drawCustom(const char* payload) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  // yellow header
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(4, 3); oled.print(F("CUSTOM"));
  oled.setDrawColor(1);

  // blue zone: text starts at y=20, line height 10 -> rows 20,30,40,50
  oled.drawFrame(0, YELLOW_H, W, H - YELLOW_H);
  oled.setFont(FONT_S);
  int y = 20, x = 4;
  for (const char* p = payload; *p; p++) {
    if (*p == '\n' || x > W - 10) {
      y += 10; x = 4;
      if (y > H - 10) break;
      if (*p == '\n') continue;
    }
    oled.setCursor(x, y);
    oled.print(*p);
    x += 6;
  }
  oled.sendBuffer();
}

void drawRaw(const String& s) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.setCursor(0, 0);
  oled.print(s);
  oled.sendBuffer();
}

} // namespace
