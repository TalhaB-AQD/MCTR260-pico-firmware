# Documentation Overhaul - Implementation Plan

> **Temporary file.** Delete after all items are complete.

---

## Phase 1: Remove Wrong-Codebase Documents

- [ ] Delete `motion_control_reference.md` (ESP32 document: FreeRTOS, esp_err_t, NVS, PID/encoder APIs)
- [ ] Delete `motor_manager_reference.md` (ESP32 document: esp_timer, motion:: namespace, autotuning)
- [ ] Delete `wifi_provisioning.md` (ESP32 WiFi feature that does not exist on Pico)
- [ ] Grep all remaining `.md` files for cross-references to deleted files; fix any broken links

## Phase 2: Content Accuracy Validation (All Surviving Documents)

Cross-reference every technical claim, code snippet, pin number, UUID, constant, and API name against the actual source code. Each document gets its own checklist.

### ARCHITECTURE.md
- [ ] Fix Core 1 mutex claim: `mutex_enter_blocking` → `mutex_try_enter` in ASCII diagram (~line 308)
- [ ] Fix Core 1 mutex claim in sequence diagram (~line 141)
- [ ] Rewrite "Why does Core 1 use `mutex_enter_blocking()`?" subsection with correct non-blocking explanation
- [ ] Add `g_emergencyStop` to inter-core shared variables list
- [ ] Add note to Vehicle Types table that only `mecanum` is currently routed; others parsed but not handled
- [ ] Verify all timing numbers (500µs loop, 20ms BLE latency, 100µs I2C batch, 1µs mutex hold) against code constants

### README.md
- [ ] Verify motor driver table specs (max current, voltage, pin counts) against datasheets
- [ ] Verify Quick Start commands (`pio run -t upload`, `pio run -t monitor`) still work
- [ ] Verify Feature Status table reflects current code state (e.g., are encoders/PID still "planned"?)
- [ ] Verify keyword list for BLE scan priority matches Flutter app filter logic

### GETTING_STARTED.md
- [ ] Verify expected boot sequence output matches current Serial.println() calls in `main.cpp`
- [ ] Verify step-by-step PlatformIO workflow still applies (no changes to `platformio.ini` structure)
- [ ] Verify JSON example in "What Just Happened?" matches `command_packet.h` struct

### CONFIGURATION_REFERENCE.md
- [ ] Verify every `#define` name, default value, and allowed range against `project_config.h`
- [ ] Verify GPIO pin tables (DRV8871, DRV8833, L298N) match `project_config.h` assignments
- [ ] Verify microstepping truth tables (TMC2209, A4988/DRV8825) against driver datasheets
- [ ] Verify vehicle geometry diagram and measurement instructions match kinematics code

### WIRING_GUIDE.md
- [ ] Verify MCP23017 Port A/B pin tables against `#define` constants in source code
- [ ] Verify I2C addresses (0x20, 0x21) match `project_config.h` or `mcp23017.cpp`
- [ ] Verify stepper driver signal descriptions (EN active-LOW, PDN must be HIGH) against `main.cpp` init code
- [ ] Verify power distribution diagram matches MECHA PICO PCB topology

### BLE_PROTOCOL.md
- [ ] Verify all UUIDs against `ble_config.h`
- [ ] Verify JSON schema examples against `command_parser.cpp` parse logic
- [ ] Verify connection parameter values (min/max interval, MTU) against `ble_config.h`
- [ ] Verify MAC derivation algorithm description against `ble_controller.cpp` implementation
- [ ] Verify messaging system (telemetry, message types) against actual BLE notification code

### TROUBLESHOOTING.md
- [ ] Verify serial tag table (`[BLE]`, `[CMD]`, `[MotorStepper]`, etc.) matches actual `Serial.printf` prefixes in code
- [ ] Verify expected boot sequence matches current firmware output
- [ ] Verify I2C scan code snippet compiles on Pico (uses `i2c_read_blocking` from Pico SDK)
- [ ] Verify LED behavior descriptions (blink on disconnect, solid on connect) match `main.cpp` `loop()`

## Phase 3: Content Depth and Breadth Improvements

Ensure each document is pedagogically appropriate for freshmen who have never seen embedded C++, BLE, or I2C before.

### Audience and Tone Review
- [ ] README.md: verify "What You Need" is a true bill-of-materials for the MECHA PICO kit (not generic parts)
- [ ] README.md: add MECHA PICO hardware platform callout (matching ARCHITECTURE.md)
- [ ] GETTING_STARTED.md: verify "Embedded Build Pipeline" section is understandable without prior embedded experience
- [ ] ARCHITECTURE.md: verify "Key Concepts" section covers all prerequisite knowledge (currently: BLE, dual-core, I2C, mutex, GPIO)
- [ ] ARCHITECTURE.md: evaluate whether additional concepts need explainers (e.g., volatile, interrupt, callback)

### Missing Content
- [ ] ARCHITECTURE.md: add "How to Add Your Own Code" forward-reference to EXTENDING_THE_FIRMWARE.md
- [ ] WIRING_GUIDE.md: add a section explaining how to identify pin numbers on the physical MECHA PICO board
- [ ] TROUBLESHOOTING.md: add "My custom code compiles but doesn't work" section for student extension issues
- [ ] CONFIGURATION_REFERENCE.md: add a "What NOT to Change" callout listing settings that break things silently (I2C pins, MCP addresses)

### Consistency Across Documents
- [ ] Ensure Motor numbering is consistent everywhere (M1=FL, M2=FR, M3=BL, M4=BR)
- [ ] Ensure joystick/dial/slider mapping is described identically in ARCHITECTURE.md, CONFIGURATION_REFERENCE.md, and BLE_PROTOCOL.md
- [ ] Ensure safety timeout description (2000ms, dual-layer) is consistent across ARCHITECTURE.md, TROUBLESHOOTING.md, and CONFIGURATION_REFERENCE.md

## Phase 4: Write EXTENDING_THE_FIRMWARE.md (New Document)

- [ ] Section 1: Before You Start (safe vs. unsafe files to modify)
- [ ] Section 2: Adding a 5th Stepper Motor (worked example using Motor 5 on Port A)
- [ ] Section 3: Creating a Custom Motion Profile (step-by-step, correct Pico APIs)
- [ ] Section 4: Using Aux Channels and Toggles (`aux[6]`, `toggles[6]` data flow)
- [ ] Section 5: Adding I2C Devices (sharing bus safely with Core 1)
- [ ] Section 6: Common Pitfalls (Core 0 vs Core 1, I2C contention, mutex discipline)
- [ ] Verify: zero occurrences of ESP32-specific terms (`esp_err_t`, `esp_timer`, `NVS`, `FreeRTOS`, `motion_controller`, `motor_manager.hpp`)

## Phase 5: Update README.md and Cross-References

- [ ] Replace flat "Where to Go Next" table with numbered reading order:
  1. README → 2. GETTING_STARTED → 3. CONFIGURATION_REFERENCE → 4. WIRING_GUIDE → 5. ARCHITECTURE → 6. BLE_PROTOCOL → 7. EXTENDING_THE_FIRMWARE → 8. TROUBLESHOOTING
- [ ] Remove links to deleted documents
- [ ] Add link to new EXTENDING_THE_FIRMWARE.md
- [ ] GETTING_STARTED.md: add "Next Steps" section pointing to EXTENDING_THE_FIRMWARE.md
- [ ] CONFIGURATION_REFERENCE.md: add cross-ref in Motion Profiles section to extension guide
- [ ] BLE_PROTOCOL.md: remove any references to deleted documents (if any)

## Phase 6: Final Verification

- [ ] Grep all `.md` files for references to deleted files → zero hits
- [ ] Grep EXTENDING_THE_FIRMWARE.md for ESP32-specific terms → zero hits
- [ ] Walk the reading order: confirm every linked document exists and forward/back references work
- [ ] Spot-check ARCHITECTURE.md mutex section against `main.cpp` `loop1()`
- [ ] Confirm motor numbering consistency across all documents (grep `M1`, `Front-Left`, `Motor 0`)
- [ ] Delete this file (`_IMPLEMENTATION_PLAN.md`)
