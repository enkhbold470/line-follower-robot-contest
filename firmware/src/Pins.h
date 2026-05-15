// ============================================================
//  Pins.h - GPIO map for ESP32-C3 (Wemos LOLIN C3 mini)
//
//  ESP32-C3 constraints honored:
//   - ADC1 only, on GPIO0..GPIO4 (used for the 4 IR sensors)
//   - GPIO18/19 reserved for native USB (CDC + JTAG); not used
//   - GPIO11..17 are SPI-flash internal; not exposed
//
//  Strap-pin caveats with this map:
//   - GPIO2  (IR3)        - MUST be HIGH at reset. QRE1113GR output
//                            is HIGH when no reflection / unpowered.
//                            RISK: if robot starts on a *dark* line
//                            or VCC_SENSORS comes up before MCU,
//                            GPIO2 may read LOW -> boot fails.
//                            Mitigation: add 10k pullup on IR3 line,
//                            or power sensors AFTER MCU is running.
//   - GPIO8  (SDA)        - must be HIGH at boot. I2C bus pullups
//                            handle this. Safe.
//   - GPIO9  (SCL)        - download-mode strap; LOW at boot enters
//                            serial bootloader. I2C bus pullups
//                            keep it HIGH. Safe.
//
//  Net (from netlist)  ->  Variable        ->  GPIO
//  ------------------      -----------         ----
//  AIN1                    PIN_AIN1            5
//  AIN2                    PIN_AIN2            6
//  BIN1                    PIN_BIN1           21
//  BIN2                    PIN_BIN2           20
//  DRV_SLEEP               PIN_DRV_SLEEP       7
//  IR-OUT1                 PIN_IR1             0     (ADC1_CH0)
//  IR-OUT2                 PIN_IR2             1     (ADC1_CH1)
//  IR-OUT3                 PIN_IR3             2     (ADC1_CH2, strap)
//  IR-OUT4                 PIN_IR4             3     (ADC1_CH3)
//  BUTTON1                 PIN_BUTTON          4
//  SDA                     PIN_SDA             8     (strap, pull-up safe)
//  SCL                     PIN_SCL             9     (strap, pull-up safe)
//
//  Free / unused: GPIO10
// ============================================================
#pragma once

// ---- DRV8833 motor driver ----
#define PIN_AIN1         5
#define PIN_AIN2         6
#define PIN_BIN1        21
#define PIN_BIN2        20
#define PIN_DRV_SLEEP   7

// ---- QRE1113GR IR reflectance sensors (ADC1 only on C3) ----
#define PIN_IR1          0
#define PIN_IR2          1
#define PIN_IR3          2
#define PIN_IR4          3

// ---- single user button (active-low, INPUT_PULLUP) ----
#define PIN_BUTTON       4

// ---- I2C (SSD1306 OLED at 0x3C) ----
#define PIN_SDA          8
#define PIN_SCL          9

// ---- LEDC PWM channels for DRV8833 (Arduino-ESP32 v2.x channel API) ----
#define PWM_CH_AIN1      0
#define PWM_CH_AIN2      1
#define PWM_CH_BIN1      2
#define PWM_CH_BIN2      3
#define PWM_FREQ     500   // 20 kHz - above audible
#define PWM_RES          8   // 8-bit (0..255)
