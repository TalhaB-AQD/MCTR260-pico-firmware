# Configuration Reference

## Table of Contents

1. [What This Does](#what-this-does)
2. [Device Identity](#device-identity)
3. [Motor Type Selection](#motor-type-selection)
4. [Motor Driver Comparison](#motor-driver-comparison)
5. [Motion Profiles](#motion-profiles)
6. [Vehicle Geometry](#vehicle-geometry)
7. [GPIO Pin Assignments](#gpio-pin-assignments)
8. [Stepper Parameters](#stepper-parameters)
9. [Timing Parameters](#timing-parameters)
10. [Microstepping Reference](#microstepping-reference)
11. [Quick Configuration Examples](#quick-configuration-examples)

---

## What This Does

All configuration for your Pico W robot lives in **one file**: [`project_config.h`](file:///Users/bigsur/Documents/Projects/Software/RemoteControllerApp/firmwares/pico/src/project_config.h). This reference explains every setting, what it does, and how to choose the right values.

> [!IMPORTANT]
> After changing `project_config.h`, you must rebuild and re-upload the firmware. Changes are compile-time only.

---

## Device Identity

```cpp
#define DEVICE_NAME "PicoBot_05"
#define BLE_PASSKEY 123456
```

| Setting | Purpose | Default | Notes |
|---------|---------|---------|-------|
| `DEVICE_NAME` | BLE advertised name | `"PicoBot_05"` | Shown in app scan list |
| `BLE_PASSKEY` | 6-digit PIN for pairing | `123456` | Must match what you enter in the app |

### Device Name Keywords

> [!IMPORTANT]
> The Flutter app **pushes robots to the top of the scan list** if the device name contains any of these keywords (case-insensitive):
>
> `robot` · `rc` · `pico` · `mecanum` · `tank` · `meca` · `controller`
>
> **Pick a name that includes at least one keyword** so your robot doesn't get buried under other Bluetooth devices.

**Good names:** `PicoBot_05`, `RC_Tank_1`, `MecaRover`, `MyRobotController`

**Bad names:** `Steve`, `TestDevice`, `Board3` — these will appear at the bottom of the list

### How BLE Identity Works

The Pico W **derives its BLE MAC address** from these two values:

```
CRC16(DEVICE_NAME + BLE_PASSKEY) → 2 bytes → last 2 octets of MAC address
```

This means:
- **Change the name or PIN → the MAC address changes**
- Your phone sees a **"new device"** and you must re-pair
- This is intentional — it allows running multiple Pico robots with unique identities
- The static address prefix is always `C0:FF:EE:00:XX:YY`

> [!WARNING]
> If you rename your robot and can't connect, delete the old pairing from your phone's Bluetooth settings first.

---

## Motor Type Selection

Uncomment **exactly one** motor driver define:

```cpp
// DC Motor drivers — uncomment ONE:
// #define MOTOR_DRIVER_DRV8871
// #define MOTOR_DRIVER_DRV8833
// #define MOTOR_DRIVER_L298N

// Stepper drivers — uncomment ONE:
#define STEPPER_DRIVER_TMC2209
// #define STEPPER_DRIVER_A4988
// #define STEPPER_DRIVER_DRV8825
```

> [!CAUTION]
> Enabling more than one driver simultaneously will cause a compile error. Only one DC or one stepper driver may be active at a time.

---

## Motor Driver Comparison

### DC Motor Drivers

| Feature | DRV8871 | DRV8833 | L298N |
|---------|:-------:|:-------:|:-----:|
| **Motors per IC** | 1 | 2 | 2 |
| **ICs for 4 motors** | 4 | 2 | 2 |
| **Pico pins per motor** | 2 PWM | 2 PWM | 3 (2 dir + 1 PWM) |
| **Total Pico pins** | 8 | 8 | 12 |
| **Max current (per ch)** | 3.6A | 1.5A | 2A |
| **Max voltage** | 45V | 10.8V | 46V |
| **Built-in current limit** | ✅ | ✅ | ❌ |
| **Coast/brake control** | ❌ | ✅ | ❌ |
| **Heat dissipation** | Low | Low | High (big heatsink) |
| **Best for** | High-power single motors | Small / medium robots | Legacy, easy to source |

**Pin wiring per driver:**

```
DRV8871 / DRV8833:          L298N:
  Pico GPIO → IN1 (PWM)      Pico GPIO → ENA (PWM speed)
  Pico GPIO → IN2 (PWM)      Pico GPIO → IN1 (direction)
                              Pico GPIO → IN2 (direction)
```

### Stepper Motor Drivers

| Feature | TMC2209 | A4988 | DRV8825 |
|---------|:-------:|:-----:|:-------:|
| **Interface** | Step/Dir | Step/Dir | Step/Dir |
| **Max microsteps** | 1/256 (internal) | 1/16 | 1/32 |
| **HW microstep pins** | MS1, MS2 | MS1, MS2, MS3 | MS1, MS2, MS3 |
| **Max current** | 2.8A RMS | 2A | 2.5A |
| **StealthChop** | ✅ (silent) | ❌ | ❌ |
| **SpreadCycle** | ✅ (high torque) | ❌ | ❌ |
| **Stall detection** | ✅ (via UART) | ❌ | ❌ |
| **UART config** | ✅ (optional) | ❌ | ❌ |
| **Thermal shutdown** | ✅ | ✅ | ✅ |
| **Noise level** | Very quiet | Moderate | Moderate |
| **Best for** | Precision, quiet ops | Budget builds | Mid-range, high step |

> [!TIP]
> On the MechaPico MCB, stepper control signals go through an MCP23017 I2C expander — you do NOT wire STEP/DIR to Pico GPIO pins directly. See the [Wiring Guide](WIRING_GUIDE.md).

---

## Motion Profiles

```cpp
#define MOTION_PROFILE_MECANUM     // 4-wheel omnidirectional
// #define MOTION_PROFILE_DIRECT   // Raw per-motor control via aux channels
```

### Vehicle Types (sent by Flutter app)

The app sends a `"vehicle"` field in each JSON command. The firmware uses this to determine how to interpret the control inputs:

| JSON `"vehicle"` value | App Display Name | Left Control | Right Control |
|----------------------|-----------------|--------------|---------------|
| `"mecanum"` | Mecanum Drive | Dial (rotation ω) | Joystick (vx, vy strafe) |
| `"fourwheel"` | Four Wheel Drive | Dial (steering) | Slider (throttle) |
| `"tracked"` | Tracked Drive | Slider (left track) | Slider (right track) |
| `"dual"` | Dual Joystick | Joystick (left) | Joystick (right) |

### Firmware Motion Profiles

| Profile | What it does | Vehicle types it handles |
|---------|-------------|-------------------------|
| **Mecanum** | Inverse kinematics → 4 independent wheel speeds | `mecanum` (primary), adaptable to others |
| **Direct** | Maps aux channels directly to individual motors — no kinematics | Any (raw control) |

---

## Vehicle Geometry

These measurements feed into the Mecanum kinematics calculations. Accuracy directly affects how straight your robot drives.

```
    ┌──── TRACK_WIDTH ────┐
    │                     │
   [FL]───────────────[FR]    ─┐
    │                     │    │  WHEELBASE
    │      (center)       │    │
    │                     │    │
   [BL]───────────────[BR]    ─┘
```

| Setting | What to measure | Default | Unit |
|---------|----------------|---------|------|
| `WHEEL_RADIUS_MM` | Diameter of one wheel ÷ 2 | `50.0f` | mm |
| `WHEELBASE_HALF_MM` | Center-to-front-axle distance | `100.0f` | mm |
| `TRACK_WIDTH_HALF_MM` | Center-to-left-wheel distance | `100.0f` | mm |

> [!TIP]
> **How to measure:** Place the robot on flat ground. Measure the **outer** diameter of a wheel with calipers and divide by 2 for WHEEL_RADIUS_MM. For WHEELBASE_HALF_MM, measure the distance between the front and rear axle centers and divide by 2. For TRACK_WIDTH_HALF_MM, measure the distance between two wheels on the same axle and divide by 2.

---

## GPIO Pin Assignments

### DC Motors — DRV8871

| Motor | GPIO A (PWM) | GPIO B (PWM) |
|-------|:------------:|:------------:|
| Front-Left | GP2 | GP3 |
| Front-Right | GP4 | GP5 |
| Back-Left | GP6 | GP7 |
| Back-Right | GP8 | GP9 |

### DC Motors — DRV8833

Two chips, each driving two motors:

| Motor | GPIO IN1 (PWM) | GPIO IN2 (PWM) |
|-------|:--------------:|:--------------:|
| Front-Left (Chip 1 A) | GP2 | GP3 |
| Front-Right (Chip 1 B) | GP4 | GP5 |
| Back-Left (Chip 2 A) | GP6 | GP7 |
| Back-Right (Chip 2 B) | GP8 | GP9 |

### DC Motors — L298N

Two modules, three pins per motor:

| Motor | Enable (PWM) | IN1 (Dir) | IN2 (Dir) |
|-------|:------------:|:---------:|:---------:|
| Front-Left (Mod 1) | GP2 | GP3 | GP4 |
| Front-Right (Mod 1) | GP5 | GP6 | GP7 |
| Back-Left (Mod 2) | GP8 | GP9 | GP10 |
| Back-Right (Mod 2) | GP11 | GP12 | GP13 |

### Stepper Motors — via MCP23017

| Signal | Pin | Notes |
|--------|-----|-------|
| I2C SDA | GP4 | I2C0 data line |
| I2C SCL | GP5 | I2C0 clock line |
| I2C INT | GP22 | Interrupt (optional, unused currently) |

Individual motor STEP/DIR pins are on the MCP23017, not on Pico GPIO. See [Wiring Guide](WIRING_GUIDE.md) for the full MCP23017 pin table.

---

## Stepper Parameters

| Setting | Purpose | Default | Notes |
|---------|---------|---------|-------|
| `STEPPER_STEPS_PER_REV` | Base steps per revolution | `200` | Standard 1.8° motors = 200 |
| `STEPPER_MICROSTEPPING` | Microstep divisor | `8` | TMC2209 default is 8 |
| `STEPPER_MAX_SPEED` | Max step rate | `4000.0` steps/sec | Higher needs faster I2C |
| `STEPPER_ACCELERATION` | Ramp rate | `8000.0` steps/sec² | Prevents missed steps at startup |
| `STEPPER_PULSE_INTERVAL_US` | Core 1 loop period | `500` µs | 500µs = 2kHz update rate |
| `STEPPER_SPEED_DEADZONE` | Minimum active speed | `10.0` steps/sec | Below this → motor stopped |

### Effective Steps Per Revolution

```
Effective steps = STEPPER_STEPS_PER_REV × STEPPER_MICROSTEPPING
Example: 200 × 8 = 1,600 effective steps per revolution
```

### Speed vs. Pulse Interval

```
Max supported speed = 1,000,000 ÷ (STEPPER_PULSE_INTERVAL_US × 2)
Default: 1,000,000 ÷ (500 × 2) = 1,000 steps/sec per motor at full resolution
```

If you need higher speeds, reduce `STEPPER_PULSE_INTERVAL_US` — but keep it above ~100µs or I2C writes won't complete in time.

---

## Timing Parameters

| Setting | Purpose | Default | Notes |
|---------|---------|---------|-------|
| `MOTOR_UPDATE_INTERVAL_MS` | DC motor update rate | `20` ms (50Hz) | Lower = smoother but more CPU |
| `SAFETY_TIMEOUT_MS` | Emergency stop timeout | `2000` ms | Stop if no command in 2 seconds |
| `TELEMETRY_INTERVAL_MS` | BLE telemetry rate | `500` ms | Battery, speed data sent to app |

> [!IMPORTANT]
> `SAFETY_TIMEOUT_MS` must be larger than the app's heartbeat interval (typically 1000ms). If set too low, the robot will stutter during normal BLE latency spikes.

---

## Microstepping Reference

### TMC2209

| MS1 | MS2 | Microsteps | Effective steps/rev (200-step motor) |
|:---:|:---:|:----------:|:------------------------------------:|
| 0 | 0 | 8 | 1,600 |
| 1 | 0 | 2 | 400 |
| 0 | 1 | 4 | 800 |
| 1 | 1 | 16 | 3,200 |

Default on MechaPico MCB: **MS1=0, MS2=0 → 8 microsteps** (set via MCP23017 GPA4/GPA5)

### A4988 / DRV8825

| MS1 | MS2 | MS3 | Microsteps | Effective steps/rev |
|:---:|:---:|:---:|:----------:|:-------------------:|
| 0 | 0 | 0 | Full step | 200 |
| 1 | 0 | 0 | Half step | 400 |
| 0 | 1 | 0 | 1/4 step | 800 |
| 1 | 1 | 0 | 1/8 step | 1,600 |
| 1 | 1 | 1 | 1/16 step | 3,200 |

> [!WARNING]
> TMC2209 and A4988 have **different** MS1/MS2 truth tables! A "0,0" gives 8 microsteps on TMC2209 but full steps on A4988. Make sure `STEPPER_MICROSTEPPING` matches what your hardware actually does.

---

## Quick Configuration Examples

### Example 1: DC Robot with DRV8871 Drivers

```cpp
#define DEVICE_NAME "DC_Robot_01"
#define BLE_PASSKEY 654321
#define MOTOR_DRIVER_DRV8871
#define MOTION_PROFILE_MECANUM
#define WHEEL_RADIUS_MM 30.0f
#define WHEELBASE_HALF_MM 75.0f
#define TRACK_WIDTH_HALF_MM 80.0f
```

### Example 2: Stepper Robot with TMC2209 (MechaPico MCB)

```cpp
#define DEVICE_NAME "MecaPico_01"
#define BLE_PASSKEY 111111
#define STEPPER_DRIVER_TMC2209
#define MOTION_PROFILE_MECANUM
#define STEPPER_MICROSTEPPING 8
#define STEPPER_MAX_SPEED 4000.0f
#define STEPPER_ACCELERATION 8000.0f
```

### Example 3: Budget Stepper with A4988

```cpp
#define DEVICE_NAME "RC_Tank_01"
#define BLE_PASSKEY 999999
#define STEPPER_DRIVER_A4988
#define MOTION_PROFILE_MECANUM
#define STEPPER_MICROSTEPPING 1      // Full step for max torque
#define STEPPER_MAX_SPEED 800.0f     // Lower for full-step mode
#define STEPPER_ACCELERATION 2000.0f
```
