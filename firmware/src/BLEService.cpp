// ============================================================
//  BLEService.cpp - NimBLE GATT server for the Web Bluetooth app
//
//  PID characteristic removed; characteristic slot layout changed
//  (see UUIDs below). Custom + Status + Unlock shifted by one slot.
// ============================================================
#include "BLEService.h"
#include <NimBLEDevice.h>

// Custom UUIDs - match these in the web app
#define SVC_UUID        "6e400001-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_CMD_UUID    "6e400002-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_CARD_UUID   "6e400003-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_BADGE_UUID  "6e400004-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_CUSTOM_UUID "6e400005-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_STATUS_UUID "6e400006-c7a3-4b6f-9a83-1b00a0c8b001"
#define CHR_UNLOCK_UUID "6e400007-c7a3-4b6f-9a83-1b00a0c8b001"

namespace Ble {

static NimBLEServer*         server   = nullptr;
static NimBLECharacteristic* chCmd    = nullptr;
static NimBLECharacteristic* chCard   = nullptr;
static NimBLECharacteristic* chBadge  = nullptr;
static NimBLECharacteristic* chCustom = nullptr;
static NimBLECharacteristic* chStatus = nullptr;
static NimBLECharacteristic* chUnlock = nullptr;

static bool   connected = false;
static bool   unlocked  = false;
static String cmdQueue;
static bool   cmdReady = false;

static String cardData  = "{\"name\":\"Inky G.\",\"title\":\"Maker\","
                          "\"contact\":\"i@inkyg.com\"}";
static String badgeData = "{\"l1\":\"INKY G.\",\"l2\":\"say hi @ inkyg.com\"}";
static String customData = "Hello from\nBADGE";

// clock: when "time:<epoch>" arrives we record (clockBaseS, clockBaseMs)
static uint32_t clockBaseS  = 0;
static uint32_t clockBaseMs = 0;
static bool     clockSet    = false;

// countdown user-configured target seconds
static uint32_t cdSeconds = 30;

// ------- helpers ------------------------------------------------------
static void enqueueCmd(const String& v) {
  if (!v.length()) return;

  // "time:N" sets clock; "cd:N" sets countdown target; both consumed here.
  if (v.startsWith("time:")) {
    uint32_t s  = (uint32_t) strtoul(v.c_str() + 5, nullptr, 10);
    clockBaseS  = s;
    clockBaseMs = millis();
    clockSet    = true;
    return;
  }
  if (v.startsWith("cd:")) {
    uint32_t s = (uint32_t) strtoul(v.c_str() + 3, nullptr, 10);
    if (s > 0) cdSeconds = s;
    return;
  }
  // anything else: drop into queue for main loop
  cmdQueue = v;
  cmdReady = true;
}

// ------- server callbacks ---------------------------------------------
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

// ------- characteristic callbacks -------------------------------------
class CmdCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    enqueueCmd(String(c->getValue().c_str()));
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
class UnlockCB : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    String v = c->getValue().c_str();
    unlocked = (v == "1" || v.equalsIgnoreCase("unlock"));
    Serial.printf("[ble] unlock = %d\n", unlocked);
  }
};

void begin() {
  NimBLEDevice::init("ROBOT");
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
  chCustom = mkChr(CHR_CUSTOM_UUID,
                   NIMBLE_PROPERTY::READ  | NIMBLE_PROPERTY::WRITE,
                   new CustomCB(), customData);
  chUnlock = mkChr(CHR_UNLOCK_UUID,
                   NIMBLE_PROPERTY::WRITE,
                   new UnlockCB());

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

void tick() { /* callbacks handle everything */ }

bool isConnected() { return connected; }
bool isUnlocked()  { return unlocked;  }

String cardJson()   { return cardData;   }
String badgeJson()  { return badgeData;  }
String customText() { return customData; }

uint32_t clockEpochSeconds() {
  if (!clockSet) {
    // uptime fallback: tick from boot
    return millis() / 1000UL;
  }
  return clockBaseS + (millis() - clockBaseMs) / 1000UL;
}
bool     clockIsSet()        { return clockSet; }
uint32_t countdownSeconds()  { return cdSeconds; }

bool peekModeCommand(String& out) {
  if (!cmdReady) return false;
  out = cmdQueue; cmdReady = false; return true;
}

void pushStatus(const String& json) {
  if (!chStatus) return;
  chStatus->setValue(json.c_str());
  if (connected) chStatus->notify();
}

} // namespace
