// ============================================================
//  test_motor.cpp - DRV8833 motor bring-up test
//
//  Hardware: ESP32-C3 (lolin_c3_mini) + DRV8833 + 2 DC motors
//    AIN1 = GPIO5     (motor A)
//    AIN2 = GPIO6
//    BIN1 = GPIO21    (motor B)
//    BIN2 = GPIO20
//    nSLEEP = GPIO7   (must be HIGH to wake driver)
//    VM  = 3.7V LiPo  (DRV8833 range 2.7..10.8 V)
//
//  Sequence (3 s per step, repeats):
//    1. STOP                  - both motors off (coast)
//    2. A forward slow        - motor A only, duty 100
//    3. A forward fast        - motor A only, duty 220
//    4. A reverse             - motor A only, duty 180 reverse
//    5. B forward slow        - motor B only, duty 100
//    6. B forward fast        - motor B only, duty 220
//    7. B reverse             - motor B only, duty 180 reverse
//    8. BOTH forward          - straight line
//    9. BOTH reverse          - back up
//   10. SPIN LEFT             - A reverse, B forward
//   11. SPIN RIGHT            - A forward, B reverse
//   12. BRAKE                 - both motors held (active braking)
//
//  What to watch:
//   - serial prints current step
//   - listen for chip reset (boot banner re-prints) -> missing VM
//     bulk cap or VINT cap, or battery sag
//   - feel chip temperature -> if hot quickly, check wiring polarity
//
//  Flash:    pio run -e esp32c3_motortest -t upload
//  Monitor:  pio device monitor -e esp32c3_motortest
// ============================================================

#include <Arduino.h>

// ---- pin map (mirrors src/Pins.h; standalone so test doesn't pull src/) ----
#define PIN_AIN1         5
#define PIN_AIN2         6
#define PIN_BIN1        21
#define PIN_BIN2        20
#define PIN_DRV_SLEEP    7

#define CH_AIN1   0
#define CH_AIN2   1
#define CH_BIN1   2
#define CH_BIN2   3
#define PWM_FREQ   20000   // 20 kHz - quiet
#define PWM_RES        8   // 8-bit duty (0..255)

#define STEP_MS       3000

// ---- DRV8833 slow-decay PWM ----
//   forward: in1 = 255 (HIGH),  in2 = 255 - duty
//   reverse: in1 = 255 - duty,  in2 = 255 (HIGH)
//   coast  : both = 0
//   brake  : both = 255
static void writePair(uint8_t ch1, uint8_t ch2, int speed) {
  if (speed > 0) {
    int duty = constrain(speed, 0, 255);
    ledcWrite(ch1, 255);
    ledcWrite(ch2, 255 - duty);
  } else if (speed < 0) {
    int duty = constrain(-speed, 0, 255);
    ledcWrite(ch1, 255 - duty);
    ledcWrite(ch2, 255);
  } else {
    ledcWrite(ch1, 0);
    ledcWrite(ch2, 0);
  }
}

static void driveA(int s) { writePair(CH_AIN1, CH_AIN2, s); }
static void driveB(int s) { writePair(CH_BIN1, CH_BIN2, s); }
static void stopAll() { driveA(0); driveB(0); }
static void brakeAll() {
  ledcWrite(CH_AIN1, 255); ledcWrite(CH_AIN2, 255);
  ledcWrite(CH_BIN1, 255); ledcWrite(CH_BIN2, 255);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("=========================================="));
  Serial.println(F(" test_motor  -  DRV8833 bring-up"));
  Serial.println(F("=========================================="));
  Serial.printf("[cfg] AIN=%d,%d  BIN=%d,%d  nSLEEP=%d  PWM=%d Hz\n",
                PIN_AIN1, PIN_AIN2, PIN_BIN1, PIN_BIN2,
                PIN_DRV_SLEEP, PWM_FREQ);

  pinMode(PIN_DRV_SLEEP, OUTPUT);
  digitalWrite(PIN_DRV_SLEEP, HIGH);   // wake driver
  Serial.println(F("[drv] nSLEEP HIGH"));

  ledcSetup(CH_AIN1, PWM_FREQ, PWM_RES);
  ledcSetup(CH_AIN2, PWM_FREQ, PWM_RES);
  ledcSetup(CH_BIN1, PWM_FREQ, PWM_RES);
  ledcSetup(CH_BIN2, PWM_FREQ, PWM_RES);

  ledcAttachPin(PIN_AIN1, CH_AIN1);
  ledcAttachPin(PIN_AIN2, CH_AIN2);
  ledcAttachPin(PIN_BIN1, CH_BIN1);
  ledcAttachPin(PIN_BIN2, CH_BIN2);

  stopAll();
  Serial.println(F("[drv] PWM channels attached, motors stopped"));
  delay(500);
}

struct Step {
  const char* label;
  int a;
  int b;
  bool brake;
};

static const Step STEPS[] = {
  { "01 STOP            ",    0,    0, false },
  { "02 A fwd slow      ",  100,    0, false },
  { "03 A fwd fast      ",  220,    0, false },
  { "04 A reverse       ", -180,    0, false },
  { "05 B fwd slow      ",    0,  100, false },
  { "06 B fwd fast      ",    0,  220, false },
  { "07 B reverse       ",    0, -180, false },
  { "08 BOTH forward    ",  180,  180, false },
  { "09 BOTH reverse    ", -180, -180, false },
  { "10 SPIN LEFT       ", -180,  180, false },
  { "11 SPIN RIGHT      ",  180, -180, false },
  { "12 BRAKE           ",    0,    0, true  },
};
static const int N_STEPS = sizeof(STEPS) / sizeof(STEPS[0]);

void loop() {
  static int      idx     = 0;
  static uint32_t stepAt  = 0;
  uint32_t now = millis();

  if (stepAt == 0 || now - stepAt >= STEP_MS) {
    stepAt = now;
    const Step& s = STEPS[idx];
    Serial.printf("[%s]  A=%4d  B=%4d %s\n",
                  s.label, s.a, s.b, s.brake ? "(brake)" : "");
    if (s.brake) {
      brakeAll();
    } else {
      driveA(s.a);
      driveB(s.b);
    }
    idx = (idx + 1) % N_STEPS;
  }
}
