// ============================================================
//  Sensors.h — 4× QRE1113GR + PID line-position estimator
//
//  QRE1113GR analog output:
//    bright surface (white) → LOW  (~0.1V)
//    dark surface  (black)  → HIGH (~3V)
//
//  Position is a weighted average with sensor weights
//    [-1500, -500, +500, +1500]
//  giving range −1500 (full left) … +1500 (full right), 0 = center.
//
//  Invert mode lets the same code follow either:
//    - BLACK line on WHITE surface (default)
//    - WHITE line on BLACK surface (inverted)
// ============================================================
#pragma once
#include <Arduino.h>

namespace Sensors {

  constexpr int N = 4;

  void begin();
  void readRaw(int out[N]);                      // raw 0..4095
  void readNormalized(int out[N], bool invert);  // 0..1000 (line=high)

  // calibration sweep — call from menu while user drags the bot
  void calibrateBegin();
  void calibrateStep();                          // call ~100 Hz
  void calibrateEnd();

  // weighted position; returns LAST position if no sensor sees line
  // pos range: -1500 (left) .. +1500 (right); 0 = centered
  int  position(bool invert);
  bool lineLost();
}

// -------------------- PID --------------------
struct PID {
  float kp = 0, ki = 0, kd = 0;
  float integ = 0, prevErr = 0;
  uint32_t lastUs = 0;
  bool  first  = true;
  float iLimit = 5000.0f;

  void reset() { integ = 0; prevErr = 0; lastUs = micros(); first = true; }

  float step(float error) {
    uint32_t now = micros();
    float dt = (lastUs == 0) ? 0.01f : (now - lastUs) * 1e-6f;
    if (dt <= 0 || dt > 0.2f) dt = 0.01f;
    lastUs = now;

    if (first) { prevErr = error; first = false; }    // avoid derivative kick

    integ += error * dt;
    if (integ >  iLimit) integ =  iLimit;
    if (integ < -iLimit) integ = -iLimit;

    float deriv = (error - prevErr) / dt;
    prevErr = error;

    return kp * error + ki * integ + kd * deriv;
  }
};
