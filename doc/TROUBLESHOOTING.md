# Troubleshooting Guide

## Table of Contents

1. [BLE Connectivity Issues](#ble-connectivity-issues)
2. [Motor Issues](#motor-issues)
3. [Stepper-Specific Issues](#stepper-specific-issues)
4. [MCP23017 / I2C Issues](#mcp23017--i2c-issues)
5. [Safety System Issues](#safety-system-issues)
6. [Serial Debug Output Guide](#serial-debug-output-guide)
7. [LED Status Reference](#led-status-reference)
8. [Build & Upload Issues](#build--upload-issues)

---

## BLE Connectivity Issues

| Issue | Likely Cause | Solution |
|-------|-------------|----------|
| Robot not appearing in scan list | BLE not advertising | Check serial output for `[BLE] Advertising started`. If missing, verify BTstack init in serial log |
| Robot appears but won't connect | Stale pairing on phone | Delete the old pairing from phone's Bluetooth settings, then re-scan |
| Robot appears low in scan list | Name missing priority keyword | Include `robot`, `rc`, `pico`, `mecanum`, `tank`, `meca`, or `controller` in `DEVICE_NAME` — the app filters these to the top |
| PIN dialog not appearing | Security not configured | Verify `BLE_PASSKEY` is set in `project_config.h`. Check serial for `[BLE] Security configured` |
| Wrong PIN rejected | Mismatch | Ensure `BLE_PASSKEY` in firmware matches what you enter in the app |
| Connected but no motor response | Commands not received | Check serial for `[CMD] Received:` messages. If missing, verify UUIDs match Flutter app |
| Connects then disconnects immediately | UUID mismatch | Compare UUIDs in `ble_config.h` with Flutter `ble_constants.dart` |
| "New device" after firmware update | MAC address changed | You changed `DEVICE_NAME` or `BLE_PASSKEY` — delete old pairing, re-pair |
| Intermittent disconnects | BLE range / interference | Move closer, reduce WiFi interference, check `SAFETY_TIMEOUT_MS` isn't too low |

> [!TIP]
> **Quick BLE diagnostic:** Open serial monitor at 115200 baud. You should see:
> ```
> [BLE] MAC: C0:FF:EE:00:XX:YY
> [BLE] Device name: PicoBot_05
> [BLE] Advertising started
> ```
> If any of these are missing, BLE init failed.

---

## Motor Issues

| Issue | Likely Cause | Solution |
|-------|-------------|----------|
| No motors moving at all | Wrong motor type enabled | Check `project_config.h` — only ONE driver define should be uncommented |
| DC motors not spinning | Wrong GPIO pins | Verify pin numbers in `project_config.h` match your physical wiring |
| DC motors spin wrong direction | Direction swap needed | Swap `pinA`/`pinB` in config, or physically swap motor wires |
| DC motors make whining noise | PWM frequency too low | Firmware uses 20kHz by default — if modified, restore `analogWriteFreq(20000)` |
| Motor speed doesn't match joystick | Geometry values wrong | Re-measure `WHEEL_RADIUS_MM`, `WHEELBASE_HALF_MM`, `TRACK_WIDTH_HALF_MM` |
| Robot drifts instead of going straight | Geometry asymmetry | Verify all 4 wheels are identical size and track width is measured accurately |
| Only some motors work | Partial wiring | Check each motor's GPIO connections individually. See serial for `[MotorDC] Motor X initialized` |

---

## Stepper-Specific Issues

| Issue | Likely Cause | Solution |
|-------|-------------|----------|
| Steppers not moving | Enable pin not LOW | Check serial for `stepperEnableAll()`. Verify GPA3 (STPR_ALL_EN) is LOW |
| Steppers vibrate but don't rotate | Wrong microstepping config | Verify `STEPPER_MICROSTEPPING` matches MS1/MS2 pin states |
| Audible clicking/rattling | Missed steps | Reduce `STEPPER_MAX_SPEED` or increase `STEPPER_ACCELERATION` |
| Jittery motion | Core 1 timing issue | Check I2C bus for contention. Ensure no Core 0 code touches I2C during motor operation |
| Very slow stepper response | `STEPPER_PULSE_INTERVAL_US` too high | Default is 500µs. Reducing to 250µs doubles max speed but increases CPU load |
| Steppers lose position over time | Acceleration too aggressive | Lower `STEPPER_ACCELERATION` or use higher microstepping for smoother ramp |
| TMC2209 not responding to STEP | PDN pin is LOW (UART mode) | Ensure GPA7 (`STPR_ALL_PDN`) is HIGH for standalone step/dir mode |
| One motor works, others don't | Wiring on MCP23017 | Check specific motor's STEP/DIR bits — see [Wiring Guide](WIRING_GUIDE.md#port-b--motor-stepdir-signals) |

> [!IMPORTANT]
> **TMC2209 vs A4988 microstepping:** These have DIFFERENT truth tables! MS1=0, MS2=0 gives **8 microsteps** on TMC2209 but **full step** on A4988. Wrong match = wrong speed. See [Microstepping Reference](CONFIGURATION_REFERENCE.md#microstepping-reference).

---

## MCP23017 / I2C Issues

| Issue | Likely Cause | Solution |
|-------|-------------|----------|
| `[MCP23017] Init failed at 0x20` | I2C not connected | Check SDA (GP4) and SCL (GP5) wiring |
| `[MCP23017] Init failed at 0x21` | Wrong I2C address | Verify A0 jumper on PCB matches 0x21 address |
| Intermittent I2C failures | Missing pull-ups | Add 4.7kΩ pull-up resistors from SDA and SCL to 3.3V |
| I2C works at startup, fails later | Bus contention | Both cores may be accessing I2C simultaneously — check that Core 0 doesn't call MCP23017 functions during motor operation |
| All steppers fail simultaneously | Shared signals wrong | Check GPA3 (EN), GPA7 (PDN) — both must be correct for any stepper to work |
| Slow I2C (motors can't reach full speed) | Wrong I2C frequency | Verify `MCP23017_I2C_FREQ` is 400000 (400kHz). At 100kHz, step rate is limited |

> [!TIP]
> **I2C bus scan:** Add this to `setup()` to scan for connected devices:
> ```cpp
> for (uint8_t addr = 0x08; addr < 0x78; addr++) {
>     uint8_t data;
>     if (i2c_read_blocking(i2c0, addr, &data, 1, false) >= 0) {
>         Serial.printf("I2C device found at 0x%02X\n", addr);
>     }
> }
> ```
> You should see 0x20 and 0x21 for a MechaPico MCB.

---

## Safety System Issues

| Issue | Likely Cause | Solution |
|-------|-------------|----------|
| Motors stop every ~2 seconds | Safety timeout triggering | App heartbeat interval must be less than `SAFETY_TIMEOUT_MS` (default 2000ms) |
| Motors don't stop on disconnect | Watchdog disabled | Check serial for `[Safety]` messages. Verify `safety_init()` is called in `setup()` |
| False emergency stops | BLE latency spikes | Increase `SAFETY_TIMEOUT_MS` to 3000-5000ms. Reduce if real safety response is too slow |
| Motors keep running after app closes | BLE disconnect not detected | Check `onDisconnect` callback in `ble_controller.cpp` — should call `motor_manager.stopAll()` |

> [!NOTE]
> The safety system has two layers:
> 1. **Heartbeat watchdog:** Times out if no command arrives within `SAFETY_TIMEOUT_MS`
> 2. **BLE disconnect callback:** Immediately stops motors when BLE connection drops
>
> Both should trigger `stop_all_motors`. If one fails, the other is a backup.

---

## Serial Debug Output Guide

Connect at **115200 baud** to see debug output. Each subsystem uses a `[TAG]` prefix:

| Tag | Source | What It Shows |
|-----|--------|---------------|
| `[BLE]` | `ble_controller.cpp` | Connection events, MAC address, advertising state |
| `[CMD]` | `command_parser.cpp` | Received JSON commands and parsed values |
| `[MotorDC]` | `motor_dc.cpp` | DC motor initialization and PWM values |
| `[MotorStepper]` | `motor_stepper.cpp` | Stepper initialization and speed changes |
| `[MotorManager]` | `motor_manager.cpp` | Motor registry and init sequence |
| `[Safety]` | `safety.cpp` | Watchdog feed events and timeout triggers |
| `[SimpleStepper]` | `simple_stepper.cpp` | Core 1 step pulse generation |
| `[Mecanum]` | `profile_mecanum.cpp` | Kinematics output (wheel speeds) |
| `[MCP23017]` | `mcp23017.cpp` | I2C init and register write results |

### Expected Boot Sequence

```
[MCP23017] Initializing I2C: SDA=4, SCL=5, freq=400000
[MCP23017] Stepper MCP (0x20) initialized
[MCP23017] DC Motor MCP (0x21) initialized
[MotorManager] Registering 4 stepper motors
[MotorStepper] Motor 0 initialized (TMC2209)
[MotorStepper] Motor 1 initialized (TMC2209)
[MotorStepper] Motor 2 initialized (TMC2209)
[MotorStepper] Motor 3 initialized (TMC2209)
[BLE] MAC: C0:FF:EE:00:XX:YY
[BLE] Device name: PicoBot_05
[BLE] Advertising started
[Safety] Watchdog initialized (timeout: 2000ms)
```

If any line is missing, the corresponding subsystem failed to initialize.

---

## LED Status Reference

| LED | State | Meaning |
|-----|-------|---------|
| Pico built-in LED | ON (solid) | BLE device connected |
| Pico built-in LED | OFF | No BLE connection / advertising |
| LED_BAR_1-5 | User-defined | Available for custom status indicators |

---

## Build & Upload Issues

| Issue | Likely Cause | Solution |
|-------|-------------|----------|
| `compile error: multiple motor drivers` | Two drivers uncommented | Only ONE motor driver define should be active in `project_config.h` |
| `undefined reference to Arduino.h` | Wrong PlatformIO platform | Ensure `platformio.ini` uses `platform = raspberrypi` with `framework = arduino` |
| Upload fails: "port busy" | Serial monitor running | Close the serial monitor (PlatformIO or other), then retry upload |
| Upload fails: "no device found" | USB connection | Hold BOOTSEL button on Pico while plugging in USB, then upload |
| `pico/mutex.h not found` | earlephilhower core missing | Run `pio pkg install` to ensure the RP2040 Arduino core is installed |
