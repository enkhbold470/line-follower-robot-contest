// ============================================================
//  Modes.h — one function per menu mode (enter/tick/exit)
//  Pure software modes: badge / card / games / utils. No motor.
// ============================================================
#pragma once
#include <Arduino.h>

enum ModeId {
  MODE_NONE,
  MODE_CARD,        // business card
  MODE_BADGE,       // event badge marquee
  MODE_BLE,         // BLE pairing screen
  MODE_SNAKE,       // 1-button snake
  MODE_DINO,        // dino-runner jump
  MODE_REACTION,    // reaction-time test
  MODE_DICE,        // dice roller
  MODE_MAGIC8,      // magic 8-ball
  MODE_CLOCK,       // clock (BLE-set or uptime)
  MODE_STOPWATCH,
  MODE_COUNTDOWN,
  MODE_CUSTOM       // arbitrary text from BLE
};

namespace Modes {
  void begin();
  void enter(ModeId m);
  void tick (ModeId m, int buttonEvent);   // 0=none 1=short 2=long 3=double
  void exit (ModeId m);

  // Whether long-press exits the mode (true) or is consumed internally (false).
  bool longPressExits(ModeId m);
}
