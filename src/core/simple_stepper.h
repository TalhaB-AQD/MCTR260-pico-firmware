/**
 * @file simple_stepper.h
 * @brief Core 1 real-time stepper engine — flat C API for direct MCP23017
 * control
 *
 * WHY THIS EXISTS ALONGSIDE MotorStepper:
 *   MotorStepper (OOP) does 8 I2C writes for 4 motors (2 per motor).
 *   simple_stepper batches all 4 motors into 2 I2C writes total.
 *   On Core 1's tight 500µs loop, this 4x reduction is critical.
 *
 * THREAD SAFETY: All functions here run ONLY on Core 1.
 *   Core 0 communicates via g_targetSpeeds[] protected by g_speedMutex.
 */

#ifndef SIMPLE_STEPPER_H
#define SIMPLE_STEPPER_H

#include <stdint.h>

// Initialize the simple stepper system with a hardware timer
void simple_stepper_init();

// Set target speed for a motor (in steps/sec, negative = reverse)
// Motors 0-3 map to M1-M4
void simple_stepper_set_speed(uint8_t motor, float stepsPerSec);

// Call this from main loop - generates step pulses based on timing
void simple_stepper_update();

// Stop all motors immediately
void simple_stepper_stop_all();

#endif // SIMPLE_STEPPER_H
