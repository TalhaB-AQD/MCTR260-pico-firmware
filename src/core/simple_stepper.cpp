/**
 * @file simple_stepper.cpp
 * @brief Main-loop based stepper with diagnostic logging
 * 
 * Adds detailed diagnostics to understand failure mode
 */

#include "simple_stepper.h"
#include "../drivers/mcp23017.h"
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
    float targetSpeed;
    float accumulator;
};

static MotorState motors[4] = {0};
static const int8_t DIR_INVERT[4] = {-1, 1, -1, 1};
static unsigned long lastUpdateTime = 0;
static const unsigned long UPDATE_INTERVAL_US = 500;  // Back to 500us

// Diagnostics
static unsigned long updateCount = 0;
static unsigned long stepCount = 0;
static unsigned long lastDiagTime = 0;
static unsigned long maxLoopTime = 0;
static bool motorsActive = false;

void simple_stepper_init() {
    lastUpdateTime = micros();
    lastDiagTime = millis();
    for (int i = 0; i < 4; i++) {
        motors[i].targetSpeed = 0;
        motors[i].accumulator = 0;
    }
    Serial.println("[SimpleStepper] Init (500us) with diagnostics");
}

void simple_stepper_set_speed(uint8_t motor, float stepsPerSec) {
    if (motor >= 4) return;
    motors[motor].targetSpeed = stepsPerSec * DIR_INVERT[motor];
}

void simple_stepper_update() {
    unsigned long now = micros();
    unsigned long elapsed = now - lastUpdateTime;
    
    if (elapsed < UPDATE_INTERVAL_US) {
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
        float speed = motors[i].targetSpeed;
        float absSpeed = fabsf(speed);
        
        if (absSpeed < 1.0f) {
            motors[i].accumulator = 0;
            continue;
        }
        
        anyActive = true;
        
        // Direction
        if (speed > 0) {
            switch (i) {
                case 0: dirMask |= M1_DIR; break;
                case 1: dirMask |= M2_DIR; break;
                case 2: dirMask |= M3_DIR; break;
                case 3: dirMask |= M4_DIR; break;
            }
        }
        
        // Accumulator
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
    
    // State transition logging DISABLED - contributes to timing issues
    // if (anyActive && !motorsActive) {
    //     motorsActive = true;
    //     Serial.printf("[Stepper] ACTIVE at %lu ms\n", millis());
    // } else if (!anyActive && motorsActive) {
    //     motorsActive = false;
    //     Serial.printf("[Stepper] STOPPED at %lu ms\n", millis());
    // }
    motorsActive = anyActive;
    
    if (stepMask != 0) {
        mcpStepper.setPortB(dirMask | stepMask);
        delayMicroseconds(5);
        mcpStepper.setPortB(dirMask);
    }
    
    // Periodic diagnostics (every 10 seconds - reduced frequency)
    if (millis() - lastDiagTime >= 10000) {
        Serial.printf("[Diag] updates=%lu steps=%lu maxLoop=%luus\n",
            updateCount, stepCount, maxLoopTime);
        lastDiagTime = millis();
        maxLoopTime = 0;
    }
}

void simple_stepper_stop_all() {
    for (int i = 0; i < 4; i++) {
        motors[i].targetSpeed = 0;
        motors[i].accumulator = 0;
    }
    mcpStepper.setPortB(0);
    motorsActive = false;
}
