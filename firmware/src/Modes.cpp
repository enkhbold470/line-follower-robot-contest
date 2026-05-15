// ============================================================
//  Modes.cpp — badge / card / games / utils. No motor, no IR.
//
//  Each mode is a small state machine driven by:
//    enter() once on selection, tick() at loop rate, exit() once.
//  Button events: 0=NONE 1=SHORT 2=LONG 3=DOUBLE  (DOUBLE always
//  exits to menu — handled in main.cpp).
// ============================================================
#include "Modes.h"
#include "Display.h"
#include "BLEService.h"
#include "Pins.h"

namespace Modes {

// ---------- shared state ----------
static uint32_t modeEnterMs = 0;
static uint32_t lastUiMs    = 0;
static uint32_t lastStatusMs= 0;

// ---------- snake state ----------
namespace SnakeGame {
  static constexpr int CELL  = 6;            // px per cell
  static constexpr int COLS  = 21;           // 128/6 = 21
  static constexpr int ROWS  = 8;            // 48/6 = 8 (y 16..63)
  static constexpr int MAXLEN = COLS * ROWS;

  static uint8_t bx[MAXLEN], by[MAXLEN];     // body cells (head at 0)
  static int     len;
  static int     dir;                        // 0=R 1=D 2=L 3=U
  static int     fx, fy;
  static uint32_t lastStep;
  static int     stepMs;
  static bool    alive;
  static int     score;

  static void placeFood() {
    while (true) {
      fx = random(0, COLS);
      fy = random(0, ROWS);
      bool hit = false;
      for (int i = 0; i < len; i++) if (bx[i]==fx && by[i]==fy) { hit=true; break; }
      if (!hit) return;
    }
  }

  void reset() {
    len = 3;
    bx[0]=COLS/2;   by[0]=ROWS/2;
    bx[1]=COLS/2-1; by[1]=ROWS/2;
    bx[2]=COLS/2-2; by[2]=ROWS/2;
    dir = 0;
    stepMs = 220;
    alive = true;
    score = 0;
    lastStep = millis();
    placeFood();
  }

  void turn() {
    // single button = turn clockwise
    dir = (dir + 1) & 3;
  }

  void step() {
    int nx = bx[0], ny = by[0];
    switch (dir) {
      case 0: nx++; break;
      case 1: ny++; break;
      case 2: nx--; break;
      case 3: ny--; break;
    }
    if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) { alive = false; return; }
    for (int i = 0; i < len; i++) if (bx[i]==nx && by[i]==ny) { alive=false; return; }

    bool grow = (nx == fx && ny == fy);
    int newLen = grow ? min(len+1, MAXLEN) : len;
    for (int i = newLen - 1; i > 0; i--) { bx[i] = bx[i-1]; by[i] = by[i-1]; }
    bx[0] = nx; by[0] = ny;
    len = newLen;
    if (grow) {
      score++;
      placeFood();
      if (stepMs > 80) stepMs -= 6;          // speed up
    }
  }
}

// ---------- dino state ----------
namespace DinoGame {
  static float    y, vy;                     // dino y, vertical velocity
  static bool     onGround;
  static int      obsX;                      // obstacle x
  static int      obsW, obsH;
  static float    speed;                     // px/frame
  static uint32_t lastFrame;
  static int      score;
  static bool     alive;

  void reset() {
    y = 0; vy = 0; onGround = true;
    obsX = 128; obsW = 6; obsH = 10;
    speed = 2.0f;
    score = 0;
    alive = true;
    lastFrame = millis();
  }

  void jump() {
    if (onGround) { vy = -3.2f; onGround = false; }
  }

  void step() {
    uint32_t now = millis();
    if (now - lastFrame < 33) return;        // ~30 fps
    lastFrame = now;

    // physics
    vy += 0.22f;
    y  += vy;
    if (y >= 0) { y = 0; vy = 0; onGround = true; }

    // obstacle
    obsX -= (int)speed;
    if (obsX < -obsW) {
      obsX = 128 + random(20, 80);
      obsH = random(8, 16);
      obsW = random(4, 10);
      score++;
      if (speed < 5.0f) speed += 0.15f;
    }

    // collision: dino rect 12x12 at x=10, top = ground - 12 + y
    int dinoLeft = 10, dinoRight = 22;
    int dinoTop  = 52 - 12 + (int)y;
    int dinoBot  = 52 + (int)y;
    int oLeft = obsX, oRight = obsX + obsW;
    int oTop  = 52 - obsH, oBot = 52;
    if (dinoRight > oLeft && dinoLeft < oRight &&
        dinoBot   > oTop  && dinoTop  < oBot) {
      alive = false;
    }
  }
}

// ---------- reaction state ----------
namespace Reaction {
  enum St { READY, WAIT, GO, RESULT };
  static St      st;
  static uint32_t tStart;
  static uint32_t tGo;
  static uint32_t reactMs;
  static uint32_t waitTarget;
  static int      best;

  void reset() { st = READY; reactMs = 0; }
}

// ---------- dice state ----------
namespace DiceRoll {
  static int      d1, d2;
  static int      rolling;                   // frames remaining
  static uint32_t lastFrame;
}

// ---------- magic 8 ball state ----------
namespace Magic8 {
  static const char* ANS[] = {
    "yes",
    "no",
    "maybe",
    "ask later",
    "no doubt",
    "unlikely",
    "absolutely",
    "very doubtful",
    "signs point yes",
    "concentrate &",
    "try again",
    "cannot predict",
    "outlook good",
    "outlook poor",
    "without a doubt"
  };
  static const int N = sizeof(ANS) / sizeof(ANS[0]);
  static int      cur;
  static uint32_t shakeUntil;
}

// ---------- stopwatch state ----------
namespace Stopwatch {
  static bool     running;
  static uint32_t accumMs;
  static uint32_t startMs;
  static uint32_t elapsed() {
    return accumMs + (running ? (millis() - startMs) : 0);
  }
}

// ---------- countdown state ----------
namespace Countdown {
  static uint32_t targetMs;
  static uint32_t remainMs;
  static bool     running;
  static uint32_t lastTickMs;
}

// ---------- helpers ----------
static void parseSimpleJsonString(const String& json, const char* key, String& out) {
  String pat = String("\"") + key + "\":\"";
  int i = json.indexOf(pat);
  if (i < 0) return;
  i += pat.length();
  int j = json.indexOf('"', i);
  if (j < 0) return;
  out = json.substring(i, j);
}

static void pushStatus(const char* mode, int score, const char* state) {
  uint32_t now = millis();
  if (now - lastStatusMs < 200) return;
  lastStatusMs = now;
  String s = String("{\"mode\":\"") + mode +
             "\",\"score\":" + score +
             ",\"state\":\"" + state + "\"," +
             "\"t\":" + String(now) + "}";
  Ble::pushStatus(s);
}

// =====================================================================
void begin() {
  randomSeed(esp_random());
  Stopwatch::running = false;
  Stopwatch::accumMs = 0;
  Countdown::running = false;
  Countdown::targetMs = 30 * 1000;
  Countdown::remainMs = Countdown::targetMs;
}

bool longPressExits(ModeId m) {
  // stopwatch / countdown / dino consume long-press internally (reset).
  switch (m) {
    case MODE_STOPWATCH:
    case MODE_COUNTDOWN:
    case MODE_DINO:
      return false;
    default:
      return true;
  }
}

// =====================================================================
void enter(ModeId m) {
  modeEnterMs = millis();
  lastUiMs    = 0;

  switch (m) {
    case MODE_SNAKE:    SnakeGame::reset(); break;
    case MODE_DINO:     DinoGame::reset();  break;
    case MODE_REACTION: Reaction::reset();  break;
    case MODE_DICE:     DiceRoll::d1 = 1; DiceRoll::d2 = 1; DiceRoll::rolling = 0; break;
    case MODE_MAGIC8:   Magic8::cur = -1; Magic8::shakeUntil = 0; break;
    case MODE_CLOCK:    break;
    case MODE_STOPWATCH: /* keep prior state */ break;
    case MODE_COUNTDOWN:
      Countdown::targetMs = Ble::countdownSeconds() * 1000UL;
      if (Countdown::targetMs == 0) Countdown::targetMs = 30 * 1000;
      Countdown::remainMs = Countdown::targetMs;
      Countdown::running  = false;
      Countdown::lastTickMs = millis();
      break;
    default: break;
  }
}

void exit(ModeId m) {
  (void)m;
}

// =====================================================================
void tick(ModeId m, int btn) {
  uint32_t now = millis();

  switch (m) {

    // ---------------- business card ----------------
    case MODE_CARD: {
      if (now - lastUiMs > 250) {
        lastUiMs = now;
        String j = Ble::cardJson();
        String name="?", title="?", contact="";
        parseSimpleJsonString(j, "name",    name);
        parseSimpleJsonString(j, "title",   title);
        parseSimpleJsonString(j, "contact", contact);
        Display::drawCard(name.c_str(), title.c_str(), contact.c_str());
      }
      pushStatus("card", 0, "idle");
      break;
    }

    // ---------------- event badge ----------------
    case MODE_BADGE: {
      if (now - lastUiMs > 40) {
        lastUiMs = now;
        String j = Ble::badgeJson();
        String l1="INKY G.", l2="say hi @ inkyg.com";
        parseSimpleJsonString(j, "l1", l1);
        parseSimpleJsonString(j, "l2", l2);
        Display::drawBadge(l1.c_str(), l2.c_str(), now - modeEnterMs);
      }
      pushStatus("badge", 0, "idle");
      break;
    }

    // ---------------- BLE status ----------------
    case MODE_BLE: {
      if (now - lastUiMs > 250) {
        lastUiMs = now;
        Display::drawBLE(Ble::isConnected());
      }
      pushStatus("ble", 0, Ble::isConnected() ? "linked" : "advertising");
      break;
    }

    // ---------------- Snake ----------------
    case MODE_SNAKE: {
      if (SnakeGame::alive) {
        if (btn == 1) SnakeGame::turn();
        if (now - SnakeGame::lastStep >= (uint32_t)SnakeGame::stepMs) {
          SnakeGame::lastStep = now;
          SnakeGame::step();
        }
      } else {
        if (btn == 1) SnakeGame::reset();
      }
      if (now - lastUiMs > 40) {
        lastUiMs = now;
        Display::drawSnake(SnakeGame::bx, SnakeGame::by, SnakeGame::len,
                           SnakeGame::fx, SnakeGame::fy,
                           SnakeGame::CELL, SnakeGame::COLS, SnakeGame::ROWS,
                           SnakeGame::score, SnakeGame::alive);
      }
      pushStatus("snake", SnakeGame::score, SnakeGame::alive ? "play" : "dead");
      break;
    }

    // ---------------- Dino ----------------
    case MODE_DINO: {
      if (DinoGame::alive) {
        if (btn == 1) DinoGame::jump();
        DinoGame::step();
      } else {
        if (btn == 1 || btn == 2) DinoGame::reset();
      }
      if (now - lastUiMs > 33) {
        lastUiMs = now;
        Display::drawDino(DinoGame::y, DinoGame::obsX, DinoGame::obsW,
                          DinoGame::obsH, DinoGame::score, DinoGame::alive);
      }
      pushStatus("dino", DinoGame::score, DinoGame::alive ? "play" : "dead");
      break;
    }

    // ---------------- Reaction ----------------
    case MODE_REACTION: {
      using namespace Reaction;
      if (st == READY) {
        if (btn == 1) {
          st = WAIT;
          tStart = now;
          waitTarget = now + random(900, 3500);
        }
      } else if (st == WAIT) {
        if (btn == 1) {           // false start
          st = RESULT;
          reactMs = 0;             // 0 = false start
        } else if (now >= waitTarget) {
          st = GO;
          tGo = now;
        }
      } else if (st == GO) {
        if (btn == 1) {
          reactMs = now - tGo;
          if (best == 0 || reactMs < (uint32_t)best) best = reactMs;
          st = RESULT;
        }
      } else { // RESULT
        if (btn == 1) st = READY;
      }
      if (now - lastUiMs > 60) {
        lastUiMs = now;
        const char* s = (st==READY)?"ready":(st==WAIT)?"wait":(st==GO)?"go":"result";
        Display::drawReaction(s, reactMs, best);
      }
      pushStatus("reaction", reactMs, st==GO?"go":st==WAIT?"wait":st==READY?"ready":"result");
      break;
    }

    // ---------------- Dice ----------------
    case MODE_DICE: {
      if (btn == 1) {
        DiceRoll::rolling = 18;            // ~18 frames of shuffle
        DiceRoll::lastFrame = now;
      }
      if (DiceRoll::rolling > 0 && now - DiceRoll::lastFrame > 50) {
        DiceRoll::lastFrame = now;
        DiceRoll::d1 = random(1, 7);
        DiceRoll::d2 = random(1, 7);
        DiceRoll::rolling--;
      }
      if (now - lastUiMs > 50) {
        lastUiMs = now;
        Display::drawDice(DiceRoll::d1, DiceRoll::d2, DiceRoll::rolling > 0);
      }
      pushStatus("dice", DiceRoll::d1 + DiceRoll::d2, DiceRoll::rolling>0?"roll":"idle");
      break;
    }

    // ---------------- Magic 8-Ball ----------------
    case MODE_MAGIC8: {
      if (btn == 1) {
        Magic8::shakeUntil = now + 800;
        Magic8::cur = random(0, Magic8::N);
      }
      bool shaking = (now < Magic8::shakeUntil);
      if (now - lastUiMs > 60) {
        lastUiMs = now;
        const char* ans = (Magic8::cur < 0) ? "press to ask"
                                            : Magic8::ANS[Magic8::cur];
        Display::drawMagic8(ans, shaking, now);
      }
      pushStatus("magic8", Magic8::cur, shaking?"shake":"idle");
      break;
    }

    // ---------------- Clock ----------------
    case MODE_CLOCK: {
      if (now - lastUiMs > 250) {
        lastUiMs = now;
        uint32_t epochS = Ble::clockEpochSeconds();
        // if BLE set time, epochS = base + (now - baseMs)/1000
        uint8_t h = (epochS / 3600) % 24;
        uint8_t mi = (epochS / 60) % 60;
        uint8_t s  = epochS % 60;
        Display::drawClock(h, mi, s, Ble::clockIsSet());
      }
      pushStatus("clock", 0, "tick");
      break;
    }

    // ---------------- Stopwatch ----------------
    case MODE_STOPWATCH: {
      if (btn == 1) {                                   // short: start/stop
        if (Stopwatch::running) {
          Stopwatch::accumMs += millis() - Stopwatch::startMs;
          Stopwatch::running = false;
        } else {
          Stopwatch::startMs = millis();
          Stopwatch::running = true;
        }
      }
      if (btn == 2) {                                   // long: reset
        Stopwatch::running = false;
        Stopwatch::accumMs = 0;
      }
      if (now - lastUiMs > 60) {
        lastUiMs = now;
        Display::drawStopwatch(Stopwatch::elapsed(), Stopwatch::running);
      }
      pushStatus("stopwatch", Stopwatch::elapsed()/10, Stopwatch::running?"run":"stop");
      break;
    }

    // ---------------- Countdown ----------------
    case MODE_COUNTDOWN: {
      if (btn == 1) {                                   // short: start/pause
        Countdown::running = !Countdown::running;
        Countdown::lastTickMs = now;
      }
      if (btn == 2) {                                   // long: reset to target
        Countdown::running = false;
        Countdown::targetMs = Ble::countdownSeconds() * 1000UL;
        if (Countdown::targetMs == 0) Countdown::targetMs = 30 * 1000;
        Countdown::remainMs = Countdown::targetMs;
      }
      if (Countdown::running) {
        uint32_t dt = now - Countdown::lastTickMs;
        Countdown::lastTickMs = now;
        if (dt >= Countdown::remainMs) {
          Countdown::remainMs = 0;
          Countdown::running = false;
        } else {
          Countdown::remainMs -= dt;
        }
      } else {
        Countdown::lastTickMs = now;
      }
      if (now - lastUiMs > 100) {
        lastUiMs = now;
        Display::drawCountdown(Countdown::remainMs, Countdown::running,
                               Countdown::targetMs);
      }
      pushStatus("countdown", Countdown::remainMs/1000,
                 Countdown::running?"run":(Countdown::remainMs==0?"done":"pause"));
      break;
    }

    // ---------------- Custom ----------------
    case MODE_CUSTOM: {
      if (now - lastUiMs > 200) {
        lastUiMs = now;
        String t = Ble::customText();
        Display::drawCustom(t.c_str());
      }
      pushStatus("custom", 0, "show");
      break;
    }

    default: break;
  }
}

} // namespace
