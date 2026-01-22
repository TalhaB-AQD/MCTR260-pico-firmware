/**
 * @file profile_mecanum.cpp
 * @brief Mecanum drive motion profile implementation
 *
 * Converts joystick commands to mecanum wheel speeds.
 * Works with both DC motors (PWM) and steppers (steps/sec).
 */

#include "profile_mecanum.h"
#include "drivers/mecanum_kinematics.h"
#include "drivers/mcp23017.h"
#include "core/motor_manager.h"
#include "core/motor_dc.h"
#include "core/motor_stepper.h"
#include "project_config.h"

#include <Arduino.h>

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
    mecanum_calculate(vx, vy, omega, speedMult, MOTOR_DEADZONE, &wheels);
    
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
        // Re-enable steppers (safety timeout disables them)
        stepperEnableAll();
        
        // Steppers: convert -100..+100 to steps/sec
        // Scale by max speed (e.g., 100% = STEPPER_MAX_SPEED)
        float scale = STEPPER_MAX_SPEED / 100.0f;
        float s0 = wheels.frontLeft * scale;
        float s1 = wheels.frontRight * scale;
        float s2 = wheels.backLeft * scale;
        float s3 = wheels.backRight * scale;
        
        // Debug: print non-zero speeds
        if (s0 != 0 || s1 != 0 || s2 != 0 || s3 != 0) {
            static unsigned long lastPrint = 0;
            if (millis() - lastPrint > 500) {
                Serial.printf("[Mecanum] Steps/sec: %.0f %.0f %.0f %.0f\n", s0, s1, s2, s3);
                lastPrint = millis();
            }
        }
        
        motor_set_speed(0, s0);
        motor_set_speed(1, s1);
        motor_set_speed(2, s2);
        motor_set_speed(3, s3);
    } else {
        // No motors detected - log warning once
        static bool warnDone = false;
        if (!warnDone) {
            Serial.println("[Mecanum] WARNING: No motors detected! Check motor initialization.");
            warnDone = true;
        }
    }
}
