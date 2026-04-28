# DA Line Follower Robot Contest – Official Rules

## Overview
Build a robot that follows a black line on a white surface. Fastest time wins.  
This is an autonomous-only competition held at De Anza College.

---

## Eligibility & Teams
- **Team Size:** Teams may consist of **1 to 4 members**. Solo builders are welcome. 
- Cross-institution teams are allowed, provided at least one member handles registration.

## Robot Specifications

### Size & Weight
- **Max footprint:** 8 x 8 inches (20cm x 20cm). No height limit.
- **Max weight:** 3 kg (6.6 lbs).
- Robots are measured and weighed at rest before the run. Robots exceeding limits are disqualified.

### Power & Autonomy
- **Max voltage:** 12.6V (3S LiPo maximum). Power sources must be situated entirely inside the bot. 
- **Autonomy:** Robot must be fully autonomous. No remote control, Bluetooth input, or human intervention once started.

### Budget & Cost Limit
- **Max Cost:** The total market value of the robot's components may not exceed **$150 USD**.
- **Labor & Structure:** Labor is considered free. Standard 3D printer filament or scrap materials are exempt from the cost limit.
- **Proof:** If your robot utilizes highly expensive commercial components, organizers reserve the right to ask for a basic Bill of Materials (BOM) or purchase receipts.

---

## Track Specifications & Environment

Builders need to know physical limits to tune their PID controllers. The track will adhere to these geometry constraints:
- **Surface & Line:** White mat or paper with a black line, 3/4 to 1 inch wide (1.8-2.5 cm).
- **Turns & Radii:** The minimum arc radius for any curve is 7.5cm. Angles greater than or equal to 90 degrees may exist.
- **Line Spacing:** The distance between the center points of adjacent parallel line segments will be no less than 15cm.
- **Intersections:** There are no crossing lines or intersections.
- **Borders:** The distance from the track line to the nearest field edge is at least 15cm.

> **⚠️ WARNING - Ambient Environment:** The track environment is ambient. Teams must not assume perfectly consistent lighting (sunlight or fluorescent), track friction, or tape reflectiveness. Calibrate your sensors to handle variability.

---

## Competition Format

### Check-In & Calibration
- **Inspection:** Robots must pass a size, weight, and power inspection at least 30 minutes before your scheduled block.
- **Calibration Slot:** Each team receives a strict 5-minute window on the official track prior to racing to calibrate IR sensors to ambient lighting and track reflectiveness. *No practice is allowed outside this slot.*

### At-Track Time & Runs
- **The 5-Minute Window:** Once called to the track for official runs, each team has a **strict 5-minute total window**. All 3 runs, including physical placement, code tweaks, and sensor adjustments, must be completed within this block. 
- **Runs:** You get up to 3 runs within your window. The best single time counts toward the final ranking.
- **Hardware Locking:** You may adjust switches, change batteries, and upload new code between runs, but you **may not physically add or remove hardware** (sensors, weights, chassis parts) once your 5-minute track window begins.

### Start/Finish Physics
- Place the robot completely behind the start line to avoid triggering the timer prematurely. 
- The operator may press a single start button to begin.
- The timer begins when the **furthest front edge** of the robot breaks the start line, and stops when the furthest front edge breaks the finish line.

---

## Penalties & Edge Cases

The referee's decision is final on all edge cases.

| Violation | Definition & Penalty |
|-----------|---------|
| **Off-Line (Self-Recover)** | If the robot's *entire vertical projection* completely leaves the black track line, but it organically finds the line again: **+5 sec penalty**. |
| **Off-Line (DNF)** | If the robot spins in place, reverses direction, or leaves the track and does not recover within 10 seconds: **Run marked DNF**. |
| **Field Departure** | If any wheel completely leaves the surface of the competition mat: **Run marked DNF**. |
| **Manual Reset** | A human touches the robot after the start button is pressed: **Run marked DNF**. |
| **Wireless Control** | Any detected remote control or Bluetooth input: **Disqualified from event**. |

---

## Scoring & Awards

| Place | Criteria |
|-------|----------|
| **1st (Sprinter)** | Fastest single valid run time. |
| **2nd, 3rd** | Second and third fastest times. |
| **Innovation Award** | Judged by organizers — best chassis design, unique sensor integration, or clever programming approach. |

*Ties are broken by the team's second-best run time.*

---

## Allowed Components
- Any microcontroller (Arduino, ESP32, Raspberry Pi, STM32, etc.)
- Any motor driver and motor type
- Any sensor type (IR, camera, TOF, etc.)
- 3D printed, laser cut, or off-the-shelf chassis

## Prohibited
- Remote control input of any kind.
- Batteries exceeding 12.6V.
- Mechanisms that damage or leave residue on the track surface.

---

## Code of Conduct
- **The Spirit:** Help each other out. If you see a fellow builder struggling with their sensor calibration, lend a hand. This event is about learning and building.
- No sabotage of other robots or equipment.

## Entry
- **Fee:** $5 (Collected strictly as a "materials contribution" to cover low-tack track tape, badges, and venue supplies).
- Pre-registration required via the Luma event page. Walk-ins may be accepted depending on capacity.
- **Important:** All participants must read and agree to the [Event Policies & Terms of Participation](event-policies.md).
