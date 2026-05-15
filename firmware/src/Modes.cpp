#include "Modes.h"
#include "Motor.h"
#include "Sensors.h"
#include "Display.h"
#include "BLEService.h"
#include "Pins.h"

namespace Modes {

static PID pid;
static uint32_t modeEnterMs = 0;
static uint32_t lastUiMs    = 0;
static uint32_t lastStatusMs= 0;
static int      motorTestStep = 0;

void begin() {
  pid.kp = 0.40f;
  pid.ki = 0.0f;
  pid.kd = 2.5f;
}

bool longPressExits() {
  // calibrate uses long-press to confirm, motor test consumes long-press too — others bail.
  return true;
}

// ---- helpers ----
static void parseSimpleJsonString(const String& json, const char* key, String& out) {
  // tiny extractor: finds "key":"..." → captures inside the quotes.
  String pat = String("\"") + key + "\":\"";
  int i = json.indexOf(pat);
  if (i < 0) return;
  i += pat.length();
  int j = json.indexOf('"', i);
  if (j < 0) return;
  out = json.substring(i, j);
}

static void pushStatus(const String& mode, int pos, int l, int r) {
  uint32_t now = millis();
  if (now - lastStatusMs < 100) return;
  lastStatusMs = now;
  String s = "{\"mode\":\""+mode+"\",\"pos\":"+pos+
             ",\"l\":"+l+",\"r\":"+r+
             ",\"t\":"+String(now)+"}";
  Ble::pushStatus(s);
}

// =====================================================================
void enter(ModeId m) {
  modeEnterMs = millis();
  pid.reset();
  motorTestStep = 0;

  switch (m) {
    case MODE_LINE_BoW:
      Ble::pidValues(pid.kp, pid.ki, pid.kd);
      Motor::enable(true);
      break;
    case MODE_CALIBRATE:
      Sensors::calibrateBegin();
      Motor::stop();
      break;
    case MODE_CARD:    Motor::stop(); break;
    case MODE_BADGE:   Motor::stop(); break;
    case MODE_BLE:     Motor::stop(); Display::drawBLE(Ble::isConnected()); break;
    case MODE_IR_TEST: Motor::stop(); break;
    case MODE_MOTOR_TEST: Motor::enable(true); break;
    case MODE_PID_TUNE: Motor::stop(); break;
    case MODE_CUSTOM: Motor::stop(); break;
    default: break;
  }
}

void exit(ModeId m) {
  Motor::stop();
  if (m == MODE_CALIBRATE) Sensors::calibrateEnd();
}

// =====================================================================
void tick(ModeId m, int btn) {
  uint32_t now = millis();

  switch (m) {

    // --- line follow (Black on White) — only line-follow variant
    case MODE_LINE_BoW: {
      int pos = Sensors::position(false);
      float u = pid.step((float)pos);

      const int base = 110;                  // base speed (0..255)
      int l = base + (int)u;
      int r = base - (int)u;
      l = constrain(l, -255, 255);
      r = constrain(r, -255, 255);

      // line lost → spin slowly toward last-known direction
      if (Sensors::lineLost()) {
        int dir = (pid.prevErr >= 0) ? 1 : -1;
        l =  90 * dir; r = -90 * dir;
      }

      Motor::drive(l, r);
      if (now - lastUiMs > 100) {
        lastUiMs = now;
        Display::drawLineFollow(pos, l, r, false);
      }
      pushStatus("line_bow", pos, l, r);
      break;
    }

    // --- calibration sweep
    case MODE_CALIBRATE: {
      Sensors::calibrateStep();
      if (now - lastUiMs > 60) {
        lastUiMs = now;
        Display::drawCalibrate(now - modeEnterMs);
      }
      // long-press inside this mode = done → handled by main exiting
      break;
    }

    // --- business card (static; live updates from BLE app)
    case MODE_CARD: {
      if (now - lastUiMs > 200) {
        lastUiMs = now;
        String j = Ble::cardJson();
        String name="?", title="?", contact="";
        parseSimpleJsonString(j, "name",    name);
        parseSimpleJsonString(j, "title",   title);
        parseSimpleJsonString(j, "contact", contact);
        Display::drawCard(name.c_str(), title.c_str(), contact.c_str());
      }
      break;
    }

    // --- event badge (animated marquee)
    case MODE_BADGE: {
      if (now - lastUiMs > 40) {
        lastUiMs = now;
        String j = Ble::badgeJson();
        String l1="INKY G.", l2="say hi @ inkyg.com";
        parseSimpleJsonString(j, "l1", l1);
        parseSimpleJsonString(j, "l2", l2);
        Display::drawBadge(l1.c_str(), l2.c_str(), now - modeEnterMs);
      }
      break;
    }

    // --- BLE pairing screen
    case MODE_BLE: {
      if (now - lastUiMs > 250) {
        lastUiMs = now;
        Display::drawBLE(Ble::isConnected());
      }
      pushStatus("ble", 0, 0, 0);
      break;
    }

    // --- IR test
    case MODE_IR_TEST: {
      int s[4];
      Sensors::readNormalized(s, false);
      int pos = Sensors::position(false);
      if (now - lastUiMs > 80) {
        lastUiMs = now;
        Display::drawIRTest(s, pos);
      }
      pushStatus("ir_test", pos, 0, 0);
      break;
    }

    // --- Motor test (cycles A+, A-, B+, B-, both fwd, both rev)
    case MODE_MOTOR_TEST: {
      if (btn == 1 /*SHORT*/) motorTestStep = (motorTestStep + 1) % 7;
      int a = 0, b = 0;
      switch (motorTestStep) {
        case 0: a =    0; b =    0; break;    // STOP
        case 1: a =  160; b =    0; break;
        case 2: a = -160; b =    0; break;
        case 3: a =    0; b =  160; break;
        case 4: a =    0; b = -160; break;
        case 5: a =  160; b =  160; break;    // forward
        case 6: a = -160; b = -160; break;    // reverse
      }
      Motor::drive(a, b);
      if (now - lastUiMs > 100) {
        lastUiMs = now;
        Display::drawMotorTest(a, b);
      }
      pushStatus("motor_test", motorTestStep, a, b);
      break;
    }

    // --- PID tune (BLE-unlock only — values edited via app)
    case MODE_PID_TUNE: {
      if (now - lastUiMs > 200) {
        lastUiMs = now;
        Ble::pidValues(pid.kp, pid.ki, pid.kd);
        Display::drawPIDTune(pid.kp, pid.ki, pid.kd);
      }
      break;
    }

    // --- Custom mode (BLE-unlock only — payload is whatever app pushed)
    case MODE_CUSTOM: {
      if (now - lastUiMs > 200) {
        lastUiMs = now;
        String t = Ble::customText();
        Display::drawCustom(t.c_str());
      }
      break;
    }

    default: break;
  }

  // Handle commands pushed from the app (e.g. "motor:80,-80", "mode:line_bw")
  String cmd;
  if (Ble::popCommand(cmd)) {
    if (cmd.startsWith("motor:")) {
      int comma = cmd.indexOf(',');
      int a = cmd.substring(6, comma).toInt();
      int b = cmd.substring(comma+1).toInt();
      Motor::drive(a, b);
    } else if (cmd == "stop") {
      Motor::stop();
    }
    // additional commands can be added here
  }
}

} // namespace
