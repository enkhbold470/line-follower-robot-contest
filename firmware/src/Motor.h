// ============================================================
//  Motor.h — DRV8833 motor driver wrapper
//
//  DRV8833 modes per H-bridge:
//    AIN1 AIN2   Output
//     0    0     coast (high-Z)
//     0    1     reverse
//     1    0     forward
//     1    1     brake
//
//  We use slow-decay PWM: hold one pin HIGH, PWM the other low
//  (or vice-versa).  Cleaner current waveform than fast-decay.
// ============================================================
#pragma once
#include <Arduino.h>

namespace Motor {
  void begin();
  void enable(bool on);                 // controls DRV_SLEEP
  void setA(int speed);                 // -255 .. +255
  void setB(int speed);                 // -255 .. +255
  void drive(int left, int right);      // convenience: A=left, B=right
  void stop();                          // coast both
  void brake();                         // brake both
}
