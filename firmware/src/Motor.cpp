#include "Motor.h"
#include "Pins.h"

// Arduino-ESP32 v2.x LEDC API (channel-based).

namespace Motor {

static void writeChannel(uint8_t ch1, uint8_t ch2, int speed) {
  // slow-decay PWM:
  //   forward -> ch1 = 255 (HIGH), ch2 = 255 - duty
  //   reverse -> ch1 = 255 - duty, ch2 = 255 (HIGH)
  //   stop    -> both 0 (coast)
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

void begin() {
  pinMode(PIN_DRV_SLEEP, OUTPUT);
  digitalWrite(PIN_DRV_SLEEP, HIGH);    // wake driver

  ledcSetup(PWM_CH_AIN1, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_AIN2, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_BIN1, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_BIN2, PWM_FREQ, PWM_RES);

  ledcAttachPin(PIN_AIN1, PWM_CH_AIN1);
  ledcAttachPin(PIN_AIN2, PWM_CH_AIN2);
  ledcAttachPin(PIN_BIN1, PWM_CH_BIN1);
  ledcAttachPin(PIN_BIN2, PWM_CH_BIN2);

  stop();
}

void enable(bool on) { digitalWrite(PIN_DRV_SLEEP, on ? HIGH : LOW); }

void setA(int speed) { writeChannel(PWM_CH_AIN1, PWM_CH_AIN2, speed); }
void setB(int speed) { writeChannel(PWM_CH_BIN1, PWM_CH_BIN2, speed); }

void drive(int left, int right) { setA(left); setB(right); }

void stop() {
  ledcWrite(PWM_CH_AIN1, 0); ledcWrite(PWM_CH_AIN2, 0);
  ledcWrite(PWM_CH_BIN1, 0); ledcWrite(PWM_CH_BIN2, 0);
}

void brake() {
  ledcWrite(PWM_CH_AIN1, 255); ledcWrite(PWM_CH_AIN2, 255);
  ledcWrite(PWM_CH_BIN1, 255); ledcWrite(PWM_CH_BIN2, 255);
}

} // namespace
