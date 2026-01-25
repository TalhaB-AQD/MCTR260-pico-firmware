# Stepper Motor Control Problem Analysis and Solution Plan

## Problem Statement

**Symptom**: Stepper motors controlled via MCP23017 I2C expander exhibit:
1. **Jitter/Stuttering**: Inconsistent step timing causing jerky motion
2. **Random Stopping**: Motors stop responding after 10-30 seconds of operation
3. **Failure on Input Change**: Stopping may correlate with sudden joystick direction changes

**Working Reference**: A MicroPython test script runs the same hardware (MCP23017 → TMC2209 drivers → steppers) with perfect, continuous operation for extended periods.

**Failed Approaches**:
| Approach | Result | Reason for Failure |
|----------|--------|-------------------|
| Main loop polling (500µs) | Jitter + stops | Timing interrupted by BLE processing |
| Main loop polling (1000µs) | Immediate stop | Update rate too slow for step accumulation |
| Timer interrupt | 20%+ I2C errors | I2C operations require interrupts enabled |
| Core1 dedicated loop | BLE broken | BTstack uses core1 for Bluetooth radio |

---

## Root Cause Analysis

### Why Python Works But C++ Fails

| Factor | Python Test | C++ Firmware |
|--------|-------------|--------------|
| CPU Usage | 100% dedicated to stepper loop | Shared with BTstack, Serial, main app |
| Timing | `time.sleep()` - blocking, precise | `micros()` polling - non-blocking, variable |
| I2C Bus | Exclusive access | Shared (potential contention) |
| Interrupts | Minimal | BTstack uses interrupts extensively |

### The Core Issue

**I2C via MCP23017 is fundamentally unsuitable for real-time step pulse generation** because:

1. **I2C Latency**: Each I2C transaction (2 bytes @ 400kHz) takes ~50-75µs minimum
2. **Non-deterministic**: I2C can be delayed by bus contention, clock stretching
3. **Interrupt dependency**: I2C operations rely on interrupts; competing with BTstack causes failures
4. **Two transactions per step**: HIGH then LOW = ~150µs minimum per step cycle

At 2000 steps/sec, we need a step every 500µs. With 150µs for I2C alone, plus BTstack interrupts, there's no margin.

---

## Industry Standard Solutions

### Option 1: PIO (Programmable I/O) State Machines

The RP2040 has hardware PIO that generates precise pulses independent of CPU:
- **8 state machines** running at up to 133MHz
- **Deterministic timing** unaffected by interrupts
- **Used by FastAccelStepper library** (200,000 steps/sec capability)

**Problem**: PIO drives GPIO pins directly. Our hardware routes step/dir through MCP23017, not direct GPIO.

### Option 2: Direct GPIO (Hardware Change)

Route step/dir signals through Pico GPIO instead of MCP23017:
- Enables PIO or direct GPIO control
- Eliminates I2C timing issues
- Industry standard approach

**Problem**: Requires PCB modification.

### Option 3: DMA-Driven I2C

Use DMA to stream I2C commands to MCP23017:
- Reduces CPU involvement
- May improve consistency

**Problem**: Still limited by I2C bus speed and doesn't solve fundamental timing issues.

### Option 4: Reduce Step Rate / Accept Limitations

Lower the maximum step rate to what I2C can reliably handle:
- At 400kHz I2C, practical max is ~500-1000 steps/sec
- Increase microstepping to compensate (lower mechanical speed but smoother)

**Problem**: Significantly limits robot speed.

---

## Recommended Solution

### Primary Recommendation: Use FastAccelStepper with Hybrid Approach

Since PIO cannot directly control MCP23017, implement a **hybrid architecture**:

1. **MCP23017 for configuration only**:
   - Enable/disable drivers (STPR_ALL_EN)
   - Set microstepping mode (MS1, MS2)
   - Other non-timing-critical functions

2. **Bit-bang step pulses via I2C at reduced rate**:
   - Accept that I2C limits step rate
   - Target max 800 steps/sec per motor (conservative)
   - Use 1/16 or 1/32 microstepping to improve smoothness

3. **Optimize I2C for minimum latency**:
   - Increase I2C clock to 1MHz (if hardware supports)
   - Use blocking I2C (eliminates interrupt dependency)
   - Disable I2C clock stretching

4. **Cooperative scheduling with BTstack**:
   - Use `async_context` for stepper updates
   - Integrate with BTstack's event loop

### Implementation Plan

#### Phase 1: Stabilize Current Approach
1. Increase I2C clock speed from 400kHz to 1MHz
2. Use blocking I2C calls (not interrupt-driven)
3. Reduce target step rate to 800 steps/sec max
4. Increase microstepping from 1/8 to 1/16 for smoothness

#### Phase 2: Optimize Timing
1. Remove all Serial.printf from critical path
2. Minimize time in stepper update function
3. Use `tight_loop_contents()` between updates
4. Profile actual loop timing and I2C transaction time

#### Phase 3: BTstack Integration
1. Research BTstack async_context integration
2. Schedule stepper updates via BTstack's run loop
3. Ensure cooperative multitasking

#### Phase 4: Validation
1. Run continuous test for 10+ minutes
2. Measure actual step rate vs commanded
3. Log I2C error rate (target: 0%)
4. Compare motion quality to Python test

---

## Alternative: Hardware Modification

If software solutions prove inadequate, modify hardware to route step/dir through direct GPIO:

| Pin | Function | Notes |
|-----|----------|-------|
| GP2 | M1_STEP | Direct GPIO |
| GP3 | M1_DIR | Direct GPIO |
| GP6 | M2_STEP | Direct GPIO |
| GP7 | M2_DIR | Direct GPIO |
| GP10 | M3_STEP | Direct GPIO |
| GP11 | M3_DIR | Direct GPIO |
| GP12 | M4_STEP | Direct GPIO |
| GP13 | M4_DIR | Direct GPIO |

This would enable use of PIO/FastAccelStepper for high-performance control.

---

## Questions for Team Review

1. **Is hardware modification acceptable?** Direct GPIO would solve all timing issues.

2. **What is the minimum acceptable step rate?** With I2C constraints, 800 steps/sec is realistic. At 1/8 microstepping with 200-step motors, this is ~60 RPM.

3. **Can we test with I2C at 1MHz?** The MCP23017 datasheet specifies 400kHz max for 2.7V, but 1.7MHz at 5V. At 3.3V, 1MHz may work.

4. **Is the Python test truly identical?** Verify no other processing is happening during the Python test.

---

## Files to Modify

| File | Changes |
|------|---------|
| `mcp23017.cpp` | Increase I2C clock, use blocking writes |
| `simple_stepper.cpp` | Cap max speed, optimize timing |
| `main.cpp` | Remove unnecessary debug output |
| `platformio.ini` | (if adding FastAccelStepper library) |

---

## Success Criteria

1. Motors run continuously for 10+ minutes without stopping
2. No visible jitter at speeds up to 800 steps/sec
3. I2C error rate < 0.1%
4. BLE remains fully functional throughout
