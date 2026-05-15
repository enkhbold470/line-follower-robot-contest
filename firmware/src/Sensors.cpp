#include "Sensors.h"
#include "Pins.h"

namespace Sensors {

static const int PINS[N] = { PIN_IR1, PIN_IR2, PIN_IR3, PIN_IR4 };

static int minV[N] = { 4095, 4095, 4095, 4095 };
static int maxV[N] = {    0,    0,    0,    0 };

static int  lastPos = 0;
static bool lost    = false;

void begin() {
  for (int i = 0; i < N; i++) pinMode(PINS[i], INPUT);
  analogReadResolution(12);
#if defined(ESP32)
  analogSetPinAttenuation(PIN_IR1, ADC_11db);
  analogSetPinAttenuation(PIN_IR2, ADC_11db);
  analogSetPinAttenuation(PIN_IR3, ADC_11db);
  analogSetPinAttenuation(PIN_IR4, ADC_11db);
#endif
}

void readRaw(int out[N]) {
  for (int i = 0; i < N; i++) out[i] = analogRead(PINS[i]);
}

void readNormalized(int out[N], bool invert) {
  int raw[N]; readRaw(raw);
  for (int i = 0; i < N; i++) {
    int range = maxV[i] - minV[i];
    if (range < 50) range = 50;           // not calibrated yet → safe default
    int v = ((raw[i] - minV[i]) * 1000L) / range;
    if (v < 0)    v = 0;
    if (v > 1000) v = 1000;
    // Default: line = high (black on white → black reads HIGH raw → high normalized)
    // If invert (white on black): flip
    if (invert) v = 1000 - v;
    out[i] = v;
  }
}

// ---- calibration ----
void calibrateBegin() {
  for (int i = 0; i < N; i++) { minV[i] = 4095; maxV[i] = 0; }
}
void calibrateStep() {
  int raw[N]; readRaw(raw);
  for (int i = 0; i < N; i++) {
    if (raw[i] < minV[i]) minV[i] = raw[i];
    if (raw[i] > maxV[i]) maxV[i] = raw[i];
  }
}
void calibrateEnd() {
  // shrink to leave a small dead-zone at each end
  for (int i = 0; i < N; i++) {
    int span = maxV[i] - minV[i];
    if (span < 100) {                     // sensor never saw contrast
      minV[i] = 0; maxV[i] = 4095;
    }
  }
}

// weighted position
int position(bool invert) {
  static const int W[N] = { -1500, -500, +500, +1500 };
  int s[N]; readNormalized(s, invert);

  long num = 0, den = 0;
  for (int i = 0; i < N; i++) {
    num += (long)s[i] * W[i];
    den += s[i];
  }

  if (den < 200) {                        // no sensor sees the line
    lost = true;
    return lastPos;                       // hold last known
  }
  lost = false;
  lastPos = (int)(num / den);
  return lastPos;
}

bool lineLost() { return lost; }

} // namespace
