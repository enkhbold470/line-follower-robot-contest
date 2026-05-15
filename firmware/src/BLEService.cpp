// ============================================================
//  BLEService.cpp - NimBLE GATT server for the Web Bluetooth app
//
//  Switched from arduino-esp32 bluedroid (BLEDevice.h) to
//  NimBLE-Arduino because the bluedroid stack on ESP32-C3 throws
//  "GATT Error: Not supported" when Web Bluetooth tries to
//  startNotifications() (CCCD write fails). NimBLE auto-adds the
//  CCCD (0x2902) descriptor for any NOTIFY characteristic and
//  uses ~5x less flash.
//
//  Public API (BLEService.h) is unchanged - main.cpp / Modes.cpp
//  don't need edits.
// ============================================================
#include "BLEService.h"

#include <NimBLEDevice.h>

// Custom UUIDs - match these in the web app
#define SVC_UUID        "6e400001-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_CMD_UUID    "6e400002-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_CARD_UUID   "6e400003-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_BADGE_UUID  "6e400004-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_PID_UUID    "6e400005-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_CUSTOM_UUID "6e400006-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_STATUS_UUID "6e400007-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_UNLOCK_UUID "6e400008-c7a3-4b6f-9a83-1b00a0c8b001"

namespace Ble {

static NimBLEServer*         server   = nullptr;
static NimBLECharacteristic* chCmd    = nullptr;
static NimBLECharacteristic* chCard   = nullptr;
static NimBLECharacteristic* chBadge  = nullptr;
static NimBLECharacteristic* chPid    = nullptr;
static NimBLECharacteristic* chCustom = nullptr;
static NimBLECharacteristic* chStatus = nullptr;
static NimBLECharacteristic* chUnlock = nullptr;

static bool   connected = false;
static bool   unlocked  = false;
static String cmdQueue;
static bool   cmdReady = false;

static String cardData  = "{\"name\":\"Inky G.\",\"title\":\"Maker\","
                          "\"contact\":\"i@inkyg.com\","
                          "\"tag\":\"inkyg.com\"}";
static String badgeData = "{\"l1\":\"INKY G.\",\"l2\":\"say hi @ inkyg.com\"}";
static String customData = "Hello from\nInky's robot";

static float kp = 0.40, ki = 0.0, kd = 2.5;

// ------- server callbacks -------
class SrvCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    connected = true;
    Serial.println(F("[ble] connect"));
  }
  void onDisconnect(NimBLEServer*) override {
    connected = false;
    Serial.println(F("[ble] disconnect - re-advertising"));
    NimBLEDevice::startAdvertising();
  }
};

// ------- characteristic callbacks -------
class CmdCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    String v = c->getValue().c_str();
    if (v.length()) { cmdQueue = v; cmdReady = true; }
  }
};
class CardCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override { cardData = c->getValue().c_str(); }
};
class BadgeCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override { badgeData = c->getValue().c_str(); }
};
class CustomCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override { customData = c->getValue().c_str(); }
};
class PidCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    String s = c->getValue().c_str();
    int a = s.indexOf(','), b = s.indexOf(',', a + 1);
    if (a > 0 && b > a) {
      kp = s.substring(0, a).toFloat();
      ki = s.substring(a + 1, b).toFloat();
      kd = s.substring(b + 1).toFloat();
    }
  }
};
class UnlockCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    String v = c->getValue().c_str();
    unlocked = (v == "1" || v.equalsIgnoreCase("unlock"));
    Serial.printf("[ble] unlock = %d\n", unlocked);
  }
};

void begin() {
  NimBLEDevice::init("ROBOT");
  // Optional: bump TX power for better range. Default is fine for testing.
  // NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  server = NimBLEDevice::createServer();
  server->setCallbacks(new SrvCB());

  NimBLEService* svc = server->createService(SVC_UUID);

  auto mkChr = [&](const char* uuid, uint32_t props,
                   NimBLECharacteristicCallbacks* cb,
                   const String& initial = "") -> NimBLECharacteristic* {
    auto* c = svc->createCharacteristic(uuid, props);
    if (cb) c->setCallbacks(cb);
    if (initial.length()) c->setValue(initial.c_str());
    return c;
  };

  chCmd    = mkChr(CHR_CMD_UUID,
                   NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR,
                   new CmdCB());
  chCard   = mkChr(CHR_CARD_UUID,
                   NIMBLE_PROPERTY::READ  | NIMBLE_PROPERTY::WRITE,
                   new CardCB(),  cardData);
  chBadge  = mkChr(CHR_BADGE_UUID,
                   NIMBLE_PROPERTY::READ  | NIMBLE_PROPERTY::WRITE,
                   new BadgeCB(), badgeData);
  chPid    = mkChr(CHR_PID_UUID,
                   NIMBLE_PROPERTY::READ  | NIMBLE_PROPERTY::WRITE,
                   new PidCB(),
                   String(kp, 3) + "," + String(ki, 3) + "," + String(kd, 3));
  chCustom = mkChr(CHR_CUSTOM_UUID,
                   NIMBLE_PROPERTY::READ  | NIMBLE_PROPERTY::WRITE,
                   new CustomCB(), customData);
  chUnlock = mkChr(CHR_UNLOCK_UUID,
                   NIMBLE_PROPERTY::WRITE,
                   new UnlockCB());

  // STATUS: read + notify. NimBLE auto-adds the 0x2902 CCCD; we do
  // NOT manually addDescriptor() like bluedroid required.
  chStatus = svc->createCharacteristic(CHR_STATUS_UUID,
                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  chStatus->setValue("{}");

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SVC_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  NimBLEDevice::startAdvertising();
  Serial.println(F("[ble] advertising as ROBOT (NimBLE)"));
}

void tick() { /* callbacks handle everything; nothing periodic */ }

bool isConnected() { return connected; }
bool isUnlocked()  { return unlocked;  }

String cardJson()   { return cardData;   }
String badgeJson()  { return badgeData;  }
String customText() { return customData; }
void   pidValues(float& a, float& b, float& c) { a = kp; b = ki; c = kd; }

bool popCommand(String& out) {
  if (!cmdReady) return false;
  out = cmdQueue; cmdReady = false; return true;
}

void pushStatus(const String& json) {
  if (!chStatus) return;
  chStatus->setValue(json.c_str());
  if (connected) chStatus->notify();
}

} // namespace
