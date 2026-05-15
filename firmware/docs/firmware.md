# Robot Firmware + Web BLE App

Custom robot firmware for a 16-pin ESP32-class module driving a DRV8833 motor
driver, 4× QRE1113GR IR reflectance sensors, an SSD1306 0.96" OLED, and a
single user button — paired with a single-page Web Bluetooth control app.

```
┌─────────────────────┐   BLE GATT   ┌─────────────────────┐
│  ROBOT firmware     │ ───────────► │  Web BLE app        │
│  (Arduino / ESP32)  │              │  (single-page HTML) │
│                     │              │                     │
│  · menu state       │              │  · pair / unlock    │
│  · PID line follow  │              │  · edit business    │
│  · BLE GATT server  │              │    card / badge     │
│  · OLED renderer    │              │  · tune PID         │
│  · motor driver     │              │  · jog / telemetry  │
└─────────────────────┘              └─────────────────────┘
```

## Hardware (from your netlist)

| Net          | Component             | Notes                          |
|--------------|-----------------------|--------------------------------|
| AIN1/AIN2    | DRV8833 motor A in    | drives motor at H7 (left)      |
| BIN1/BIN2    | DRV8833 motor B in    | drives motor at H8 (right)     |
| DRV_SLEEP    | DRV8833 nSLEEP        | HIGH = enabled                 |
| IR-OUT1..4   | QRE1113GR analog out  | 4 reflectance sensors          |
| BUTTON1      | tactile switch SW4    | active-low, RC-debounced       |
| SDA / SCL    | I2C bus               | SSD1306 OLED at 0x3C           |

## Firmware

### Libraries

In Arduino IDE → Library Manager, install:

- `Adafruit GFX Library`
- `Adafruit SSD1306`
- ESP32 board support package (provides BLE classes & ledc PWM)

### Pin mapping

The netlist gives *logical* pin numbers (U1.1 … U1.16) but not GPIO numbers
because those depend on which 16-pin module you have. Edit `Pins.h`:

```c
#define PIN_AIN1   26
#define PIN_AIN2   27
// …etc.
```

Cross-reference your module's datasheet to map U1.x → GPIO.

### Building

1. Open `firmware/firmware.ino` in Arduino IDE.
2. Select your board (e.g. "ESP32 Dev Module" or your specific module).
3. Set Partition Scheme to one that fits BLE + your sketch (e.g. "Minimal SPIFFS" or "Huge APP").
4. Upload. Open Serial Monitor @ 115200 baud to see boot logs.

### Button UX

| Gesture       | Action                       |
|---------------|------------------------------|
| short press   | next menu item / cycle       |
| long press    | select item / confirm        |
| double press  | back / exit current mode     |

### Menu structure

```
MENU
 ├─ Line: B on W      (black line, white surface — default)
 ├─ Line: W on B      (white line, black surface)
 ├─ Calibrate IR      (sweep robot over line & off, long-press to finish)
 ├─ Business Card     (renders identity on OLED)
 ├─ Event Badge       (animated marquee)
 ├─ BLE Connect       (advertise, wait for app)
 ├─ IR Test           (live 4-bar reflectance display)
 ├─ Motor Test        (cycle: stop, A fwd/rev, B fwd/rev, both fwd/rev)
 ├─ PID Tune  *       (BLE-only: shows live Kp/Ki/Kd)
 └─ Custom Mode *     (BLE-only: shows arbitrary text from app)
```

`*` items are hidden behind a BLE unlock — they appear as `*` in the menu and
will refuse to launch until you connect with the web app and tap
**Unlock pro features**.

### PID line follower

The line follower computes a weighted position from the 4 IR sensors
(weights `[-1500, -500, +500, +1500]`), runs that error through a classical
PID (`Kp · e + Ki · ∫e dt + Kd · de/dt`), and applies the output as a
differential to a base motor speed.

Both line polarities are supported by inverting the per-sensor normalized
value. Run **Calibrate IR** first — sweep the robot side-to-side over the
line and off it for ~3 seconds, then long-press to confirm.

Default gains (`Kp 0.4 / Ki 0.0 / Kd 2.5`) work well at base speed 110/255
on smooth surfaces. Tune via the web app.

## Web app

`webapp/index.html` is a single HTML file. To use it:

### Option A — open it locally
On Chrome / Edge, Web Bluetooth requires HTTPS **or** file:// from a local
file. Just double-click `index.html` and it should work.

### Option B — host it
Drop it on GitHub Pages, Netlify, or any static host. Web Bluetooth requires
HTTPS for hosted pages.

### Browser support

| Platform | Browser          | Status |
|----------|------------------|--------|
| Desktop  | Chrome / Edge    | ✅      |
| Android  | Chrome           | ✅      |
| iOS      | Safari           | ❌ — use the **Bluefy** app instead |

### Flow

1. Power the robot — it advertises as **ROBOT**.
2. Open the web app, tap **Connect to ROBOT**, pick the device in the chooser.
3. Tap **Unlock pro features** to expose PID tune + Custom Mode in the on-device menu.
4. Edit fields → tap **Save to robot**. Changes apply on next render.
5. Jog the robot with the arrow pad; telemetry streams live at ~10 Hz.

## GATT service map

Service UUID: `6e400001-c7a3-4b6f-9a83-1b00a0c8b001`

| Characteristic       | UUID end | Properties      | Payload                                              |
|----------------------|----------|------------------|------------------------------------------------------|
| CMD                  | `…0002`  | write            | `motor:LEFT,RIGHT` &nbsp;\|&nbsp; `stop`             |
| CARD                 | `…0003`  | read / write     | `{"name":"…","title":"…","contact":"…","tag":"…"}`   |
| BADGE                | `…0004`  | read / write     | `{"l1":"…","l2":"…"}`                                |
| PID                  | `…0005`  | read / write     | `"kp,ki,kd"` (CSV floats)                            |
| CUSTOM               | `…0006`  | read / write     | UTF-8 text (≤ ~120 chars)                            |
| STATUS               | `…0007`  | read / notify    | `{"mode":"…","pos":n,"l":n,"r":n,"t":ms}`            |
| UNLOCK               | `…0008`  | write            | `"1"` to unlock, `"0"` to lock                       |

## Files

```
firmware/
  firmware.ino     main sketch — setup, loop, menu state machine
  Pins.h           GPIO map (edit to match your module!)
  Motor.h/cpp      DRV8833 wrapper (slow-decay PWM)
  Sensors.h/cpp    QRE1113 + calibration + PID position
  Display.h/cpp    SSD1306 rendering for every screen
  Modes.h/cpp      mode lifecycle (enter / tick / exit)
  BLEService.h/cpp GATT server
webapp/
  index.html       single-page Web Bluetooth control surface
```

## Quick troubleshooting

- **Won't pair** — confirm Serial Monitor shows `[ble] advertising as ROBOT`. On the phone, scan for BLE devices with nRF Connect to verify it's there.
- **Motor spins one way only** — swap that motor's two wires at H7/H8, or flip the speed sign in `Motor::drive(...)`.
- **Line follower oscillates** — drop Kp by 25%, or raise Kd. The OLED `pos` readout helps: if it overshoots the centerline, Kd is too low.
- **Sensors all read same value** — verify the QRE1113 are getting current through their LEDs (the 100Ω resistors R25/R27/R29/R31). Check 3V3 rail.
- **OLED blank** — verify SDA/SCL aren't swapped; SSD1306 address is `0x3C` (some clones are `0x3D`, change in `Display.cpp`).
