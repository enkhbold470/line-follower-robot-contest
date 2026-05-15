// ============================================================
//  BLEService.h — GATT server for the Web Bluetooth control app
//
//  Single service exposes these characteristics:
//
//   CMD       (write, no response)     "mode:snake" | "menu" | "btn"
//                                      "time:<epoch>" | "cd:<seconds>"
//   CARD      (read + write)           JSON: {"name":"...", ...}
//   BADGE     (read + write)           JSON: {"l1":"hi", "l2":"world"}
//   CUSTOM    (read + write)           UTF-8 payload for Custom Mode
//   STATUS    (read + notify)          JSON: live telemetry
//   UNLOCK    (write)                  write "1" -> unlock locked modes
// ============================================================
#pragma once
#include <Arduino.h>

namespace Ble {
  void begin();
  void tick();

  bool isConnected();
  bool isUnlocked();

  // payloads (latest values written by app)
  String  cardJson();
  String  badgeJson();
  String  customText();

  // utils set by CMD strings
  uint32_t clockEpochSeconds();              // current seconds-of-day if set
  bool     clockIsSet();
  uint32_t countdownSeconds();               // user-configured countdown duration

  // command queue (consumed once per pop)
  bool    peekModeCommand(String& out);

  // status push
  void    pushStatus(const String& json);
}
