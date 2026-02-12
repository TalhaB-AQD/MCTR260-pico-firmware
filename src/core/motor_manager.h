/**
 * @file motor_manager.h
 * @brief Central motor registry — init, stop, and access for all motors
 *
 * INIT SEQUENCE: motors_init() must be called BEFORE BLE starts (BLE takes
 * over Core 0's scheduling). Order: I2C → MCP23017 → microstepping → motors.
 *
 * MULTICORE I2C CAVEAT: motors_stop_all() writes to MCP23017 from Core 0.
 * If Core 1 is also writing (simple_stepper), I2C bus contention can occur.
 * This is acceptable for emergency stop (safety > timing precision).
 */

#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

#include "motor_base.h"
#include <stdint.h>

// Maximum number of motors
#define NUM_MOTORS 4

/**
 * @brief Initialize all motors based on project_config.h
 * @return true if successful
 */
bool motors_init();

/**
 * @brief Get a motor by index
 * @param index Motor index (0-3)
 * @return Pointer to motor, or nullptr if not found
 */
MotorBase *motor_get(int index);

/**
 * @brief Set target for a DC motor (PWM value)
 * @param index Motor index
 * @param pwm PWM value (-255 to +255)
 */
void motor_set_pwm(int index, int16_t pwm);

/**
 * @brief Set target speed for a stepper motor
 * @param index Motor index
 * @param stepsPerSec Target speed
 */
void motor_set_speed(int index, float stepsPerSec);

/**
 * @brief Update all motor control loops (call at 50Hz)
 * @param dtSec Time since last update
 */
void motors_update(float dtSec);

/**
 * @brief Emergency stop all motors
 */
void motors_stop_all();

/**
 * @brief Check if any motors are steppers
 */
bool motors_has_steppers();

/**
 * @brief Check if any motors are DC
 */
bool motors_has_dc();

#endif // MOTOR_MANAGER_H
