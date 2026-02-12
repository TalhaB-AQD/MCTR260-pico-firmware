/**
 * @file simple_stepper.cpp
 * @brief Main-loop based stepper with diagnostic logging
 * 
 * Adds detailed diagnostics to understand failure mode
 */

#include "simple_stepper.h"
#include "../drivers/mcp23017.h"
#include "../project_config.h"  // For STEPPER_* constants
#include <Arduino.h>

// Step/dir bit definitions for Port B
#define M1_DIR  0x01
#define M1_STEP 0x02
#define M2_DIR  0x04
#define M2_STEP 0x08
#define M3_DIR  0x10
#define M3_STEP 0x20
#define M4_DIR  0x40
#define M4_STEP 0x80

// Motor state
struct MotorState {
    float targetSpeed;   // Commanded speed from joystick
    float currentSpeed;  // Ramped speed (with acceleration limit)
    float accumulator;
};

static MotorState motors[4] = {0};
static const int8_t DIR_INVERT[4] = {-1, 1, -1, 1};
static unsigned long lastUpdateTime = 0;

// Diagnostics
static unsigned long updateCount = 0;
static unsigned long stepCount = 0;
static unsigned long lastDiagTime = 0;
static unsigned long maxLoopTime = 0;
static bool motorsActive = false;

// Derived constants from config
// Updates per second = 1,000,000 / STEPPER_PULSE_INTERVAL_US
// Acceleration per update = STEPPER_ACCELERATION / updates_per_sec
static const float ACCEL_PER_UPDATE = STEPPER_ACCELERATION / (1000000.0f / STEPPER_PULSE_INTERVAL_US);

void simple_stepper_init() {
    lastUpdateTime = micros();
    lastDiagTime = millis();
    for (int i = 0; i < 4; i++) {
        motors[i].targetSpeed = 0;
        motors[i].currentSpeed = 0;
        motors[i].accumulator = 0;
    }
    Serial.println("[SimpleStepper] Init with acceleration ramping");
}

void simple_stepper_set_speed(uint8_t motor, float stepsPerSec) {
    if (motor >= 4) return;
    
    float newSpeed = stepsPerSec * DIR_INVERT[motor];
    float oldSpeed = motors[motor].targetSpeed;
    
    // Detect direction change (signs opposite)
    // Only reset accumulator - let currentSpeed ramp naturally through zero
    // The zero-crossing detector in update() handles the transition smoothly
    bool signsOpposite = (newSpeed > 0 && oldSpeed < 0) || (newSpeed < 0 && oldSpeed > 0);
    
    if (signsOpposite) {
        motors[motor].accumulator = 0;
        // NOTE: Do NOT reset currentSpeed here - let ramping handle deceleration
        // Resetting causes a velocity discontinuity (jerk) at direction changes
    }
    
    motors[motor].targetSpeed = newSpeed;
}

void simple_stepper_update() {
    unsigned long now = micros();
    unsigned long elapsed = now - lastUpdateTime;
    
    if (elapsed < STEPPER_PULSE_INTERVAL_US) {
        return;
    }
    
    // Track max loop time
    if (elapsed > maxLoopTime) {
        maxLoopTime = elapsed;
    }
    
    lastUpdateTime = now;
    updateCount++;
    
    float dt = elapsed / 1000000.0f;
    
    uint8_t stepMask = 0;
    uint8_t dirMask = 0;
    bool anyActive = false;
    
    for (int i = 0; i < 4; i++) {
        float target = motors[i].targetSpeed;
        float current = motors[i].currentSpeed;
        
        // Apply acceleration ramping
        float diff = target - current;
        if (fabsf(diff) > ACCEL_PER_UPDATE) {
            current += (diff > 0) ? ACCEL_PER_UPDATE : -ACCEL_PER_UPDATE;
        } else {
            current = target;
        }
        
        // CRITICAL: Prevent zero-crossing during deceleration
        // If target is zero (or near-zero) and we're decelerating, clamp at zero
        if (fabsf(target) < STEPPER_SPEED_DEADZONE) {
            // Target is stop - decelerate but don't cross zero
            if ((motors[i].currentSpeed > 0 && current <= 0) ||
                (motors[i].currentSpeed < 0 && current >= 0)) {
                current = 0;  // Clamp at zero, don't cross
            }
        }
        
        motors[i].currentSpeed = current;
        
        float absSpeed = fabsf(current);
        
        // Skip motors below deadzone
        if (absSpeed < STEPPER_SPEED_DEADZONE) {
            motors[i].accumulator = 0;
            continue;
        }
        
        anyActive = true;
        
        // Direction bit
        if (current > 0) {
            switch (i) {
                case 0: dirMask |= M1_DIR; break;
                case 1: dirMask |= M2_DIR; break;
                case 2: dirMask |= M3_DIR; break;
                case 3: dirMask |= M4_DIR; break;
            }
        }
        // (If current < 0, direction bit stays 0 = reverse direction)
        
        // Accumulator for step timing
        motors[i].accumulator += absSpeed * dt;
        if (motors[i].accumulator >= 1.0f) {
            motors[i].accumulator -= 1.0f;
            if (motors[i].accumulator > 2.0f) motors[i].accumulator = 0;
            stepCount++;
            
            switch (i) {
                case 0: stepMask |= M1_STEP; break;
                case 1: stepMask |= M2_STEP; break;
                case 2: stepMask |= M3_STEP; break;
                case 3: stepMask |= M4_STEP; break;
            }
        }
    }
    
    // State tracking
    motorsActive = anyActive;
    
    // Write to port only when stepping (direction is included with step pulse)
    if (stepMask != 0) {
        mcpStepper.setPortB(dirMask | stepMask);
        delayMicroseconds(5);
        mcpStepper.setPortB(dirMask);
    }
    
    // Periodic diagnostics DISABLED on Core 1 - causes jitter
    // Enable only for debugging, then re-disable
    // if (millis() - lastDiagTime >= 10000) {
    //     Serial.printf("[Diag] updates=%lu steps=%lu maxLoop=%luus\n",
    //         updateCount, stepCount, maxLoopTime);
    //     lastDiagTime = millis();
    //     maxLoopTime = 0;
    // }
}

void simple_stepper_stop_all() {
    for (int i = 0; i < 4; i++) {
        motors[i].targetSpeed = 0;
        motors[i].currentSpeed = 0;
        motors[i].accumulator = 0;
    }
    mcpStepper.setPortB(0);
    motorsActive = false;
}
