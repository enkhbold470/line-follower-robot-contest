// ============================================================
//  Display.cpp - SSD1306 128x64 OLED via U8g2 (full buffer)
//
//  Dual-color 0.96" panels (common modules) have a yellow strip
//  in rows 0..15 and blue rows 16..63. Any single text element
//  must stay entirely inside ONE band - never start at y < 16 and
//  extend past it (the glyph would be sliced yellow-on-blue).
// ============================================================
#include "Display.h"
#include "Pins.h"
#include <Wire.h>
#include <U8g2lib.h>

#define W           128
#define H            64
#define OLED_ADDR  0x3C
#define YELLOW_H    16

#define FONT_S  u8g2_font_6x10_tr        // ~10px tall - fits in yellow strip
#define FONT_M  u8g2_font_ncenB14_tr     // ~14px - blue only
#define FONT_L  u8g2_font_logisoso24_tr  // ~24px - blue only
#define FONT_XL u8g2_font_logisoso30_tr  // ~30px - blue only (clock)

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// ---- helpers ----
static inline int strW(const char* s) { return oled.getStrWidth(s); }
static inline int fontH()             { return oled.getMaxCharHeight(); }

static void centeredStr(int boxW, int boxH, int boxX, int boxY,
                        const char* s) {
  int tw = strW(s);
  int th = fontH();
  oled.setCursor(boxX + (boxW - tw) / 2, boxY + (boxH - th) / 2);
  oled.print(s);
}

static void yellowHeader(const char* title, const char* right = nullptr) {
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(3, 3);
  oled.print(title);
  if (right) {
    int tw = strW(right);
    oled.setCursor(W - tw - 3, 3);
    oled.print(right);
  }
  oled.setDrawColor(1);
}

namespace Display {

void begin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  oled.setI2CAddress(OLED_ADDR << 1);
  if (!oled.begin()) Serial.println(F("[oled] init fail"));
  oled.setFontPosTop();
  oled.setDrawColor(1);
  oled.setFont(FONT_S);
  oled.clearBuffer();
  oled.sendBuffer();
}

void splash(const char* line, int size) {
  oled.clearBuffer();
  oled.setDrawColor(1);
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

void drawMenu(const char* const* items, int count, int sel,
              const bool* locked, bool unlocked) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  yellowHeader("MENU", unlocked ? "PRO" : "BAS");

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
  yellowHeader("BUSINESS CARD");

  oled.drawFrame(0, YELLOW_H, W, H - YELLOW_H);

  oled.setFont(FONT_M);
  {
    int tw = strW(name);
    int x  = (W - tw) / 2;
    if (x < 4) x = 4;
    oled.setCursor(x, 20);
    oled.print(name);
  }

  oled.setFont(FONT_S);
  {
    int tw = strW(title);
    int x  = (W - tw) / 2;
    if (x < 4) x = 4;
    oled.setCursor(x, 38);
    oled.print(title);
  }

  oled.drawHLine(8, 50, W - 16);
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
  yellowHeader("EVENT BADGE");

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

void drawBLE(bool connected) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  yellowHeader("BLE STATUS");

  oled.setFont(FONT_S);
  oled.setCursor(0, 22); oled.print(connected ? F("CONNECTED") : F("advertising"));
  oled.setCursor(0, 36); oled.print(F("name: ROBOT"));
  oled.setCursor(0, 50); oled.print(F("open web app"));

  oled.drawCircle(110, 40, 6);
  if (connected) oled.drawDisc(110, 40, 3);
  oled.sendBuffer();
}

void drawCustom(const char* payload) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  yellowHeader("CUSTOM");

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

// =====================================================================
// games
// =====================================================================

void drawSnake(const uint8_t* bx, const uint8_t* by, int len,
               int fx, int fy, int cell, int cols, int rows,
               int score, bool alive) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  // yellow header: title + score
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(3, 3); oled.print(F("SNAKE"));
  oled.setCursor(W - 36, 3); oled.print(F("scr:")); oled.print(score);
  oled.setDrawColor(1);

  // play area starts at y=YELLOW_H
  // border
  oled.drawFrame(0, YELLOW_H, cols * cell + 2, rows * cell + 2);

  // food (dot in cell)
  oled.drawBox(1 + fx * cell + 1, YELLOW_H + 1 + fy * cell + 1, cell - 2, cell - 2);

  // snake
  for (int i = 0; i < len; i++) {
    oled.drawBox(1 + bx[i] * cell, YELLOW_H + 1 + by[i] * cell, cell, cell);
  }

  if (!alive) {
    oled.setDrawColor(0);
    oled.drawBox(W/2 - 30, 30, 60, 16);
    oled.setDrawColor(1);
    oled.drawFrame(W/2 - 30, 30, 60, 16);
    oled.setFont(FONT_S);
    centeredStr(60, 16, W/2 - 30, 30, "GAME OVER");
  }
  oled.sendBuffer();
}

void drawDino(float y, int obsX, int obsW, int obsH, int score, bool alive) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  // header
  oled.drawBox(0, 0, W, YELLOW_H);
  oled.setDrawColor(0);
  oled.setFont(FONT_S);
  oled.setCursor(3, 3); oled.print(F("DINO"));
  oled.setCursor(W - 40, 3); oled.print(F("scr:")); oled.print(score);
  oled.setDrawColor(1);

  // ground
  oled.drawHLine(0, 52, W);

  // dino: 12x12 box, x=10, y top = 52-12+y (y<=0)
  int dy = 52 - 12 + (int)y;
  oled.drawBox(10, dy, 12, 12);
  // eye
  oled.setDrawColor(0);
  oled.drawPixel(18, dy + 3);
  oled.setDrawColor(1);

  // obstacle
  oled.drawBox(obsX, 52 - obsH, obsW, obsH);

  if (!alive) {
    oled.setFont(FONT_S);
    centeredStr(W, 12, 0, 56, "press to retry");
  }
  oled.sendBuffer();
}

void drawReaction(const char* state, uint32_t reactMs, int best) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  yellowHeader("REACTION");

  oled.setFont(FONT_M);
  const char* msg = "";
  if (!strcmp(state, "ready")) msg = "TAP TO START";
  else if (!strcmp(state, "wait")) msg = "WAIT...";
  else if (!strcmp(state, "go"))   msg = "TAP NOW!";
  else                             msg = (reactMs == 0) ? "TOO SOON" : "RESULT";
  centeredStr(W, 18, 0, 20, msg);

  oled.setFont(FONT_S);
  if (!strcmp(state, "result")) {
    char buf[24];
    if (reactMs == 0) snprintf(buf, sizeof(buf), "false start");
    else              snprintf(buf, sizeof(buf), "%lu ms", (unsigned long)reactMs);
    centeredStr(W, 12, 0, 42, buf);
    if (best > 0) {
      char b[24];
      snprintf(b, sizeof(b), "best: %d ms", best);
      centeredStr(W, 12, 0, 54, b);
    }
  } else if (!strcmp(state, "go")) {
    // flash big bar
    oled.drawBox(0, 50, W, 14);
  }
  oled.sendBuffer();
}

static void drawDieFace(int x, int y, int side, int v) {
  oled.drawFrame(x, y, side, side);
  // dot positions on a 3x3 grid
  int p[9][2] = {
    {x + side/4,     y + side/4},     // TL
    {x + side/2,     y + side/4},     // TM
    {x + 3*side/4,   y + side/4},     // TR
    {x + side/4,     y + side/2},     // ML
    {x + side/2,     y + side/2},     // MM
    {x + 3*side/4,   y + side/2},     // MR
    {x + side/4,     y + 3*side/4},   // BL
    {x + side/2,     y + 3*side/4},   // BM
    {x + 3*side/4,   y + 3*side/4},   // BR
  };
  const int pip[7][9] = {
    {0,0,0,0,0,0,0,0,0},                 // 0 (unused)
    {0,0,0,0,1,0,0,0,0},                 // 1
    {1,0,0,0,0,0,0,0,1},                 // 2
    {1,0,0,0,1,0,0,0,1},                 // 3
    {1,0,1,0,0,0,1,0,1},                 // 4
    {1,0,1,0,1,0,1,0,1},                 // 5
    {1,0,1,1,0,1,1,0,1}                  // 6
  };
  if (v < 1) v = 1; if (v > 6) v = 6;
  for (int i = 0; i < 9; i++)
    if (pip[v][i]) oled.drawDisc(p[i][0], p[i][1], 2);
}

void drawDice(int d1, int d2, bool rolling) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  char hdr[16];
  snprintf(hdr, sizeof(hdr), "sum: %d", d1 + d2);
  yellowHeader("DICE", rolling ? "..." : hdr);

  int side = 36;
  drawDieFace( 14, 22, side, d1);
  drawDieFace( 78, 22, side, d2);

  oled.setFont(FONT_S);
  centeredStr(W, 8, 0, H - 8, rolling ? "rolling" : "tap to roll");
  oled.sendBuffer();
}

void drawMagic8(const char* answer, bool shaking, uint32_t tickMs) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  yellowHeader("MAGIC 8");

  // ball: filled circle, with "8" in center
  int cx = 32, cy = 40, r = 18;
  if (shaking) {
    cx += (int)(((tickMs / 50) % 5) - 2);
    cy += (int)(((tickMs / 60) % 5) - 2);
  }
  oled.drawDisc(cx, cy, r);
  oled.setDrawColor(0);
  oled.drawDisc(cx, cy, 7);
  oled.setFont(FONT_S);
  oled.setDrawColor(1);
  oled.setCursor(cx - 3, cy - 5);
  oled.print('8');

  // answer text on right
  oled.setFont(FONT_S);
  int rx = 60;
  if (shaking) {
    oled.setCursor(rx, 22); oled.print(F("shaking..."));
  } else {
    // wrap simple at ~12 chars
    char line[32];
    int  li = 0, y = 22;
    for (const char* p = answer; *p && y < 60; p++) {
      if ((li >= 10 && *p == ' ') || li >= 11) {
        line[li] = 0;
        oled.setCursor(rx, y); oled.print(line);
        li = 0; y += 10;
        if (*p == ' ') continue;
      }
      line[li++] = *p;
    }
    if (li) { line[li] = 0; oled.setCursor(rx, y); oled.print(line); }
  }
  oled.sendBuffer();
}

// =====================================================================
// utils
// =====================================================================

void drawClock(uint8_t h, uint8_t m, uint8_t s, bool isSet) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  yellowHeader("CLOCK", isSet ? "SET" : "UPT");

  char buf[16];
  snprintf(buf, sizeof(buf), "%02u:%02u", h, m);
  oled.setFont(FONT_XL);
  {
    int tw = strW(buf);
    oled.setCursor((W - tw) / 2, 20);
    oled.print(buf);
  }
  // seconds small
  char sb[8];
  snprintf(sb, sizeof(sb), ":%02u", s);
  oled.setFont(FONT_S);
  {
    int tw = strW(sb);
    oled.setCursor((W - tw) / 2, 55);
    oled.print(sb);
  }
  oled.sendBuffer();
}

void drawStopwatch(uint32_t elapsedMs, bool running) {
  oled.clearBuffer();
  oled.setDrawColor(1);
  yellowHeader("STOPWATCH", running ? "RUN" : "STOP");

  uint32_t totalCs = elapsedMs / 10;          // hundredths
  uint32_t cs = totalCs % 100;
  uint32_t s  = (totalCs / 100) % 60;
  uint32_t mi = (totalCs / 6000) % 100;

  char big[16];
  snprintf(big, sizeof(big), "%02lu:%02lu", (unsigned long)mi, (unsigned long)s);
  oled.setFont(FONT_L);
  {
    int tw = strW(big);
    oled.setCursor((W - tw) / 2 - 10, 18);
    oled.print(big);
  }
  char ss[8];
  snprintf(ss, sizeof(ss), ".%02lu", (unsigned long)cs);
  oled.setFont(FONT_M);
  {
    oled.setCursor(W - 32, 24);
    oled.print(ss);
  }
  oled.setFont(FONT_S);
  centeredStr(W, 10, 0, 54, "short=start/stop  long=reset");
  oled.sendBuffer();
}

void drawCountdown(uint32_t remainMs, bool running, uint32_t targetMs) {
  oled.clearBuffer();
  oled.setDrawColor(1);

  const char* hdr = running ? "RUN"
                            : (remainMs == 0 ? "DONE" : "PAUSE");
  yellowHeader("COUNTDOWN", hdr);

  uint32_t s  = (remainMs + 999) / 1000;
  uint32_t mi = s / 60;
  uint32_t se = s % 60;

  char big[16];
  snprintf(big, sizeof(big), "%02lu:%02lu", (unsigned long)mi, (unsigned long)se);
  oled.setFont(FONT_L);
  {
    int tw = strW(big);
    oled.setCursor((W - tw) / 2, 20);
    oled.print(big);
  }

  // progress bar
  oled.drawFrame(8, 52, W - 16, 8);
  if (targetMs > 0) {
    uint32_t done = targetMs - (remainMs > targetMs ? targetMs : remainMs);
    int barW = (int)((uint64_t)done * (W - 18) / targetMs);
    oled.drawBox(9, 53, barW, 6);
  }
  oled.sendBuffer();
}

} // namespace
