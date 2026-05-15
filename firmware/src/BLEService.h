// ============================================================
//  BLEService.h — GATT server for the Web Bluetooth control app
//
//  Single service (UUID below) exposes these characteristics:
//
//   CMD       (write, no response)     "mode:line_bw" | "motor:80,-80" | etc
//   CARD      (read + write)            JSON: {"name":"...", ...}
//   BADGE     (read + write)            JSON: {"l1":"hi", "l2":"world"}
//   PID       (read + write)            JSON: {"kp":0.4,"ki":0,"kd":2}
//   CUSTOM    (read + write)            UTF-8 payload for Custom Mode
//   STATUS    (read + notify)           JSON: live telemetry
//   UNLOCK    (write)                   write 1 → unlock locked menu items
//
//  UUIDs are deterministic (made-up vendor namespace).
// ============================================================
#pragma once
#include <Arduino.h>

// Namespace name `Ble` avoids collision with Arduino-ESP32's `BLEService` class.
namespace Ble {
  void begin();
  void tick();                                  // call from loop()

  bool isConnected();
  bool isUnlocked();

  // payloads (latest values written by app)
  String  cardJson();
  String  badgeJson();
  String  customText();
  void    pidValues(float& kp, float& ki, float& kd);

  // commands queue (cmd handed off to Modes.cpp by main loop, simple poll)
  bool    popCommand(String& out);

  // status push (called by Modes.cpp ~10 Hz)
  void    pushStatus(const String& json);
}
