// ============================================================
//  ROBOT FIRMWARE — line-following / business-card / badge bot
//  Hardware:  ESP32-C3 (lolin_c3_mini) + DRV8833 + 4xQRE1113GR
//             + SSD1306 OLED + 1 button
//
//  BUTTON UX
//    short press  (< 400 ms)  -> next menu item / next action
//    long  press  (>= 700 ms) -> select / enter
//    double press (< 300 ms)  -> back / exit
//
//  Menu items                          (ones marked * unlock via BLE app)
//    1. Line Follow (Black on White)
//    2. Calibrate IR
//    3. Business Card
//    4. Event Badge
//    5. BLE Connect
//    6. IR Test
//    7. Motor Test
//    8. PID Tune *
//    9. Custom Mode *
// ============================================================

#include <Arduino.h>
#include "Pins.h"
#include "Motor.h"
#include "Sensors.h"
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
enum AppState {
  STATE_MENU,
  STATE_RUNNING
};

static AppState  appState = STATE_MENU;
static int       menuIdx  = 0;
static ModeId    activeMode = MODE_NONE;

static const char* MENU_ITEMS[] = {
  "Line Follow",
  "Calibrate IR",
  "Business Card",
  "Event Badge",
  "BLE Connect",
  "IR Test",
  "Motor Test",
  "PID Tune",
  "Custom Mode"
};
static const ModeId MENU_MODES[] = {
  MODE_LINE_BoW, MODE_CALIBRATE,
  MODE_CARD,    MODE_BADGE,    MODE_BLE,
  MODE_IR_TEST, MODE_MOTOR_TEST,
  MODE_PID_TUNE, MODE_CUSTOM
};
static const bool MENU_LOCKED[] = {
  false,false,false,false,false,false,false,
  true,
  true
};
static const int MENU_LEN = sizeof(MENU_ITEMS)/sizeof(MENU_ITEMS[0]);

// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\n[boot] robot firmware (esp32-c3)"));

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Motor::begin();
  Sensors::begin();
  Display::begin();
  Ble::begin();
  Modes::begin();
  
  Display::splash("hello?",   2); delay(700);  // big
  Display::splash("support?", 1); delay(600);  // medium
  Display::splash("inkyg.com",1); delay(800);  // medium
  Display::drawMenu(MENU_ITEMS, MENU_LEN, menuIdx, MENU_LOCKED,
                    Ble::isUnlocked());
}

// ============================================================
void loop() {
  Ble::tick();

  Button::Event ev = Button::poll();

  if (appState == STATE_MENU) {
    if (ev == Button::SHORT) {
      menuIdx = (menuIdx + 1) % MENU_LEN;
      Display::drawMenu(MENU_ITEMS, MENU_LEN, menuIdx, MENU_LOCKED,
                        Ble::isUnlocked());
    }
    else if (ev == Button::LONG) {
      if (MENU_LOCKED[menuIdx] && !Ble::isUnlocked()) {
        Display::toast("LOCKED - pair BLE");
        delay(900);
        Display::drawMenu(MENU_ITEMS, MENU_LEN, menuIdx, MENU_LOCKED,
                          Ble::isUnlocked());
      } else {
        activeMode = MENU_MODES[menuIdx];
        appState   = STATE_RUNNING;
        Modes::enter(activeMode);
      }
    }
  }
  else {
    if (ev == Button::DOUBLE || (ev == Button::LONG && Modes::longPressExits())) {
      Modes::exit(activeMode);
      Motor::stop();
      activeMode = MODE_NONE;
      appState   = STATE_MENU;
      Display::drawMenu(MENU_ITEMS, MENU_LEN, menuIdx, MENU_LOCKED,
                        Ble::isUnlocked());
    } else {
      Modes::tick(activeMode, ev);
    }
  }
}
