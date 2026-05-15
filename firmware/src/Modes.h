// ============================================================
//  Modes.h — one function per menu mode (enter/tick/exit)
// ============================================================
#pragma once
#include <Arduino.h>

enum ModeId {
  MODE_NONE,
  MODE_LINE_BoW,      // Black line on White surface (only line-follow variant)
  MODE_CALIBRATE,
  MODE_CARD,
  MODE_BADGE,
  MODE_BLE,
  MODE_IR_TEST,
  MODE_MOTOR_TEST,
  MODE_PID_TUNE,
  MODE_CUSTOM
};

namespace Modes {
  void begin();
  void enter(ModeId m);
  void tick (ModeId m, int buttonEvent);   // 0=none 1=short 2=long 3=double
  void exit (ModeId m);

  // If true, a long-press inside the active mode exits.
  // For modes that need long-press internally (e.g. calibrate "done"),
  // they handle it themselves and this returns false.
  bool longPressExits();
}
