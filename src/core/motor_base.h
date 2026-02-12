/**
 * @file motor_base.h
 * @brief Abstract base class for all motor types
 *
 * MOTOR ABSTRACTION:
 *   All motors (DC, stepper, future servo) share this interface so the rest
 *   of the firmware can call init(), stop(), and update() without knowing
 *   which hardware is underneath.
 *
 *   CLASS HIERARCHY:
 *     MotorBase (abstract)
 *     ├── MotorDC      — PWM control via H-bridge (DRV8871, DRV8833, L298N)
 *     └── MotorStepper — Step/dir control via MCP23017 (TMC2209, A4988,
 * DRV8825)
 *
 *   WHY setTarget() IS NOT IN THE BASE CLASS:
 *     DC motors take a PWM value (-255 to +255).
 *     Steppers take a speed in steps/sec (float).
 *     Servos would take an angle (0-180°).
 *     Forcing these into one signature would require ugly unions or casts.
 *     Instead, motor_manager.cpp casts to the specific subclass when needed.
 */

#ifndef MOTOR_BASE_H
#define MOTOR_BASE_H

#include <stdint.h>

/**
 * @brief Motor type enumeration
 */
enum class MotorType {
  DC,      // DC motor with PWM control
  Stepper, // Stepper with step/dir control
  Servo    // Future: servo motor
};

/**
 * @brief Abstract base class for all motor types
 */
class MotorBase {
public:
  virtual ~MotorBase() = default;

  /**
   * @brief Initialize motor hardware
   * @return true if successful
   */
  virtual bool init() = 0;

  /**
   * @brief Stop motor immediately (emergency stop)
   */
  virtual void stop() = 0;

  /**
   * @brief Update motor control (call at 50Hz)
   * @param dtSec Time since last update in seconds
   */
  virtual void update(float dtSec) = 0;

  /**
   * @brief Get motor index (0-based)
   */
  virtual uint8_t getIndex() const = 0;

  /**
   * @brief Check if motor is enabled
   */
  virtual bool isEnabled() const = 0;

  /**
   * @brief Get motor type
   */
  virtual MotorType getType() const = 0;
};

#endif // MOTOR_BASE_H
