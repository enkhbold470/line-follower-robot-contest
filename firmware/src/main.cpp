// ============================================================
//  BADGE FIRMWARE — ESP32-C3 + SSD1306 OLED + 1 button + BLE
//
//  Hardware: lolin_c3_mini, SSD1306 0.96" I2C OLED, 1 push button.
//  Motors / IR sensors are NOT used.
//
//  BUTTON UX
//    short  press (<400 ms)  -> next menu item / in-mode action
//    long   press (>=700 ms) -> select / exit mode (per-mode)
//    double press           -> back / exit to menu
//
//  Menu (ones marked * unlock via BLE app)
//    1. Business Card
//    2. Event Badge
//    3. BLE Connect
//    4. Snake
//    5. Dino Jump
//    6. Reaction
//    7. Dice Roll
//    8. Magic 8-Ball
//    9. Clock
//   10. Stopwatch
//   11. Countdown
//   12. Custom Screen *
// ============================================================

#include <Arduino.h>
#include "Pins.h"
#include "Display.h"
#include "BLEService.h"
#include "Modes.h"

// ---------- button handler ----------
namespace Button {
  enum Event { NONE, SHORT, LONG, DOUBLE };

  static uint32_t pressStart  = 0;
  static uint32_t lastRelease = 0;
  static bool     prevState   = HIGH;        // active-low
  static bool     waitingDbl  = false;
  static bool     longFired   = false;

  Event poll() {
    bool s = digitalRead(PIN_BUTTON);
    uint32_t now = millis();
    Event ev = NONE;

    if (s == LOW && prevState == HIGH) {
      pressStart = now;
      longFired  = false;
    }
    if (s == LOW && !longFired && (now - pressStart) >= 700) {
      ev = LONG;
      longFired = true;
    }
    if (s == HIGH && prevState == LOW) {
      uint32_t dur = now - pressStart;
      if (!longFired && dur >= 30) {
        if (waitingDbl && (now - lastRelease) < 300) {
          ev = DOUBLE;
          waitingDbl = false;
        } else {
          waitingDbl = true;
          lastRelease = now;
        }
      }
    }
    if (waitingDbl && (now - lastRelease) > 300) {
      ev = SHORT;
      waitingDbl = false;
    }
    prevState = s;
    return ev;
  }
}

// ---------- global state ----------
enum AppState { STATE_MENU, STATE_RUNNING };

static AppState  appState   = STATE_MENU;
static int       menuIdx    = 0;
static ModeId    activeMode = MODE_NONE;

static const char* MENU_ITEMS[] = {
  "Business Card",
  "Event Badge",
  "BLE Connect",
  "Snake",
  "Dino Jump",
  "Reaction",
  "Dice Roll",
  "Magic 8-Ball",
  "Clock",
  "Stopwatch",
  "Countdown",
  "Custom Screen"
};
static const ModeId MENU_MODES[] = {
  MODE_CARD, MODE_BADGE, MODE_BLE,
  MODE_SNAKE, MODE_DINO, MODE_REACTION,
  MODE_DICE, MODE_MAGIC8,
  MODE_CLOCK, MODE_STOPWATCH, MODE_COUNTDOWN,
  MODE_CUSTOM
};
static const bool MENU_LOCKED[] = {
  false,false,false,false,false,false,false,false,false,false,false,
  true                     // Custom Screen requires BLE unlock
};
static const int MENU_LEN = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

static void redrawMenu() {
  Display::drawMenu(MENU_ITEMS, MENU_LEN, menuIdx, MENU_LOCKED,
                    Ble::isUnlocked());
}

// Handle BLE "mode:xxx" command to jump straight into any mode.
static ModeId modeFromName(const String& n) {
  if (n == "card")      return MODE_CARD;
  if (n == "badge")     return MODE_BADGE;
  if (n == "ble")       return MODE_BLE;
  if (n == "snake")     return MODE_SNAKE;
  if (n == "dino")      return MODE_DINO;
  if (n == "reaction")  return MODE_REACTION;
  if (n == "dice")      return MODE_DICE;
  if (n == "magic8")    return MODE_MAGIC8;
  if (n == "clock")     return MODE_CLOCK;
  if (n == "stopwatch") return MODE_STOPWATCH;
  if (n == "countdown") return MODE_COUNTDOWN;
  if (n == "custom")    return MODE_CUSTOM;
  return MODE_NONE;
}

// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n[boot] badge firmware (esp32-c3)"));

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Display::begin();
  Ble::begin();
  Modes::begin();

  Display::splash("hello?",   2); delay(600);
  Display::splash("BADGE",    2); delay(600);
  Display::splash("inkyg.com",1); delay(700);
  redrawMenu();
}

// ============================================================
void loop() {
  Ble::tick();

  Button::Event ev = Button::poll();

  // -- handle "mode:xxx" / "menu" / "game:btn" from BLE --
  String cmd;
  if (Ble::peekModeCommand(cmd)) {
    if (cmd == "menu") {
      if (appState == STATE_RUNNING) {
        Modes::exit(activeMode);
        activeMode = MODE_NONE;
        appState   = STATE_MENU;
        redrawMenu();
      }
    } else if (cmd.startsWith("mode:")) {
      ModeId m = modeFromName(cmd.substring(5));
      if (m != MODE_NONE) {
        if (appState == STATE_RUNNING) Modes::exit(activeMode);
        activeMode = m;
        appState   = STATE_RUNNING;
        Modes::enter(activeMode);
      }
    } else if (cmd == "btn") {
      // remote button press = SHORT
      ev = Button::SHORT;
    }
  }

  if (appState == STATE_MENU) {
    if (ev == Button::SHORT) {
      menuIdx = (menuIdx + 1) % MENU_LEN;
      redrawMenu();
    }
    else if (ev == Button::LONG) {
      if (MENU_LOCKED[menuIdx] && !Ble::isUnlocked()) {
        Display::toast("LOCKED - pair BLE");
        delay(900);
        redrawMenu();
      } else {
        activeMode = MENU_MODES[menuIdx];
        appState   = STATE_RUNNING;
        Modes::enter(activeMode);
      }
    }
  }
  else {
    // DOUBLE always exits. LONG exits only if mode says so.
    if (ev == Button::DOUBLE ||
        (ev == Button::LONG && Modes::longPressExits(activeMode))) {
      Modes::exit(activeMode);
      activeMode = MODE_NONE;
      appState   = STATE_MENU;
      redrawMenu();
    } else {
      Modes::tick(activeMode, ev);
    }
  }
}
