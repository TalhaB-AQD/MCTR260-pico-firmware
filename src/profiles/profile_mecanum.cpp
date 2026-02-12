/**
 * @file profile_mecanum.cpp
 * @brief Mecanum drive motion profile implementation
 *
 * Converts joystick commands to mecanum wheel speeds.
 * Sends speed commands to Core 1 via shared memory (multicore architecture).
 */

#include "profile_mecanum.h"
#include "drivers/mecanum_kinematics.h"
#include "drivers/mcp23017.h"
#include "core/motor_manager.h"
#include "core/motor_dc.h"
#include "core/motor_stepper.h"
#include "project_config.h"

#include <Arduino.h>
#include "pico/mutex.h"

// =============================================================================
// INTER-CORE COMMUNICATION (defined in main.cpp)
// =============================================================================
extern mutex_t g_speedMutex;
extern volatile float g_targetSpeeds[4];
extern volatile bool g_speedsUpdated;

void profile_mecanum_apply(const control_command_t* cmd) {
    // Extract inputs from command
    // Right joystick: forward (Y) and strafe (X)
    // Left control: rotation (dial value or joystick X)
    
    float vx = 0;  // Strafe
    float vy = 0;  // Forward
    float omega = 0;  // Rotation
    
    if (cmd->right.isJoystick) {
        vx = cmd->right.x;   // Strafe from right X
        vy = cmd->right.y;   // Forward from right Y
    }
    
    if (cmd->left.isJoystick) {
        omega = cmd->left.x;  // Rotation from left joystick X
    } else {
        omega = cmd->left.value;  // Rotation from dial
    }
    
    // Speed multiplier (0-100 → 0.0-1.0)
    float speedMult = cmd->speed / 100.0f;
    
    // Calculate wheel speeds
    WheelSpeeds wheels;
    mecanum_calculate(vx, vy, omega, speedMult, &wheels);
    
    // Apply to motors based on type
    bool hasDC = motors_has_dc();
    bool hasSteppers = motors_has_steppers();
    
    // Debug: log motor type detection (once only)
    static bool debugDone = false;
    if (!debugDone) {
        Serial.printf("[Mecanum] Motor detection: DC=%d, Steppers=%d\n", hasDC, hasSteppers);
        debugDone = true;
    }
    
    if (hasDC) {
        // DC motors: convert -100..+100 to -255..+255 PWM
        motor_set_pwm(0, (int16_t)(wheels.frontLeft * 2.55f));
        motor_set_pwm(1, (int16_t)(wheels.frontRight * 2.55f));
        motor_set_pwm(2, (int16_t)(wheels.backLeft * 2.55f));
        motor_set_pwm(3, (int16_t)(wheels.backRight * 2.55f));
    } else if (hasSteppers) {
        // NOTE: stepperEnableAll() removed - already enabled in setup()
        // and calling it here causes I2C contention with Core 1
        
        // Steppers: convert -100..+100 to steps/sec
        float scale = STEPPER_MAX_SPEED / 100.0f;
        float s0 = wheels.frontLeft * scale;
        float s1 = wheels.frontRight * scale;
        float s2 = wheels.backLeft * scale;
        float s3 = wheels.backRight * scale;
        
        // =================================================================
        // DIAGNOSTIC: Track direction changes at BOTH center and rim
        // =================================================================
        static float prevS0 = 0, prevS1 = 0;
        static unsigned long lastDirChangeTime[4] = {0};
        static int8_t prevDir[4] = {0};
        static unsigned long lastDebug = 0;
        
        float maxSpeed = fmaxf(fmaxf(fabsf(s0), fabsf(s1)), fmaxf(fabsf(s2), fabsf(s3)));
        bool isCenter = (maxSpeed < 500 && maxSpeed > 10);
        bool isRim = (maxSpeed > 2000);
        
        // Check for direction changes on motor 0 (FR motor as example)
        int8_t curDir0 = (s0 > 10) ? 1 : ((s0 < -10) ? -1 : 0);
        unsigned long now = millis();
        
        if (curDir0 != 0 && curDir0 != prevDir[0] && prevDir[0] != 0) {
            // Direction changed - log comparison data
            unsigned long timeSinceLastChange = now - lastDirChangeTime[0];
            const char* location = isCenter ? "CENTER" : (isRim ? "RIM" : "MID");
            
            if (now - lastDebug > 30) {  // Max 33Hz logging
                Serial.printf("[DIAG] %s: s0=%.0f prevS0=%.0f | speed=%.0f | timeSinceRev=%lums\n",
                              location, s0, prevS0, maxSpeed, timeSinceLastChange);
                lastDebug = now;
            }
            lastDirChangeTime[0] = now;
        }
        
        prevDir[0] = curDir0;
        prevS0 = s0;
        
        // =================================================================
        // MULTICORE: Send speeds to Core 1 via shared memory
        // =================================================================
        // Use try_enter to avoid blocking Core 0 if Core 1 has the mutex
        if (mutex_try_enter(&g_speedMutex, nullptr)) {
            g_targetSpeeds[0] = s0;
            g_targetSpeeds[1] = s1;
            g_targetSpeeds[2] = s2;
            g_targetSpeeds[3] = s3;
            g_speedsUpdated = true;
            mutex_exit(&g_speedMutex);
        }
        // If mutex busy, skip this update - Core 1 will get the next one
        
    } else {
        // No motors detected - log warning once
        static bool warnDone = false;
        if (!warnDone) {
            Serial.println("[Mecanum] WARNING: No motors detected!");
            warnDone = true;
        }
    }
}
