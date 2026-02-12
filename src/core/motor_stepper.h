#ifndef MOTOR_STEPPER_H
#define MOTOR_STEPPER_H

#include "motor_base.h"

/**
 * @file motor_stepper.h
 * @brief Stepper motor control class — config structs and trapezoidal profile
 *
 * HOW STEPPER PINS DIFFER FROM DC:
 *   Steppers on MechaPico MCB don't use direct Pico GPIO pins. Instead,
 *   step/dir signals route through an MCP23017 I2C GPIO expander. So
 *   cfg_.pinStep is set to -1 and generateStep() writes to MCP23017 Port B.
 *
 * TWO STEPPER IMPLEMENTATIONS EXIST:
 *   1. MotorStepper (this class) — full trapezoidal acceleration profile,
 *      OOP design, per-motor I2C writes. Used on Core 0 for position mode.
 *   2. simple_stepper.cpp — flat C functions, batched I2C writes for all
 *      4 motors simultaneously. Used on Core 1 for real-time velocity mode.
 *   See motor_stepper.cpp and simple_stepper.cpp headers for details.
 */

/**
 * @brief Stepper driver type
 */
enum class StepperDriverType {
  TMC2209, // Trinamic silent driver
  A4988,   // Basic Allegro driver
  DRV8825  // TI high-current driver
};

/**
 * @brief Stepper motor configuration
 *
 * Note: On MechaPico MCB, stepper motors are controlled via MCP23017 I2C
 * GPIO expander. The motorIndex maps to physical pins on the expander.
 * See drivers/mcp23017.h for pin mappings.
 */
struct MotorStepperConfig {
  uint8_t index; // Motor index (0-4 for M1-M5 on MCP23017)
  bool enabled;  // Master on/off

  StepperDriverType driverType;

  // For MCP23017-based control (MechaPico MCB):
  // motorIndex 0-4 maps to M1-M5 on the GPIO expander
  // Direct GPIO pins are NOT used - set to -1
  int8_t pinStep;   // Legacy: Step pulse pin (-1 for MCP23017)
  int8_t pinDir;    // Legacy: Direction pin (-1 for MCP23017)
  int8_t pinEnable; // Legacy: Enable pin (-1 for MCP23017, shared enable used)

  uint16_t stepsPerRev;  // Full steps per revolution (typically 200)
  uint8_t microstepping; // 1, 2, 4, 8, 16, 32
  int8_t direction;      // 1 or -1 to reverse direction

  float maxSpeed;     // Maximum steps per second
  float acceleration; // Steps per second squared
};

/**
 * @brief Motion mode for stepper
 */
enum class StepperMode {
  Velocity, // Continuous rotation at target speed
  Position  // Move to target position
};

/**
 * @brief Stepper motor with trapezoidal acceleration
 */
class MotorStepper : public MotorBase {
public:
  explicit MotorStepper(const MotorStepperConfig &config);

  // MotorBase interface
  bool init() override;
  void stop() override;
  void update(float dtSec) override;
  uint8_t getIndex() const override;
  bool isEnabled() const override;
  MotorType getType() const override;

  /**
   * @brief Set target speed (velocity mode)
   * @param stepsPerSec Target speed in steps/second (negative for reverse)
   */
  void setTargetSpeed(float stepsPerSec);

  /**
   * @brief Move to absolute position (position mode)
   * @param targetSteps Target position in steps
   */
  void moveTo(int32_t targetSteps);

  /**
   * @brief Move relative to current position
   * @param steps Number of steps to move (negative for reverse)
   */
  void moveRelative(int32_t steps);

  /**
   * @brief Get current position in steps
   */
  int32_t getPosition() const;

  /**
   * @brief Get current speed in steps/second
   */
  float getCurrentSpeed() const;

  /**
   * @brief Check if motor is currently moving
   */
  bool isMoving() const;

  /**
   * @brief Set current position as home (zero)
   */
  void setHome();

  /**
   * @brief Update timing and acceleration (call before batched step generation)
   * @param dtSec Delta time in seconds
   * @return true if a step is needed this cycle
   */
  bool updateTiming(float dtSec);

  /**
   * @brief Get the step bit mask for this motor (for batched writes)
   */
  uint8_t getStepBit() const;

  /**
   * @brief Get the direction bit mask for this motor
   */
  uint8_t getDirBit() const;

  /**
   * @brief Get the desired direction for this motor
   */
  bool getDesiredDirection() const;

  /**
   * @brief Acknowledge that a step was generated (update position and timing)
   */
  void acknowledgeStep();

private:
  MotorStepperConfig cfg_;

  // Motion state
  StepperMode mode_;
  int32_t currentPosition_; // Current position in microsteps
  int32_t targetPosition_;  // Target position (position mode)
  float targetSpeed_;       // Target speed (velocity mode)
  float currentSpeed_;      // Current speed (ramping)

  // Step timing
  unsigned long lastStepTime_; // Last step timestamp (microseconds)
  unsigned long stepInterval_; // Current step interval (microseconds)
  bool lastDirection_;         // Cached direction to avoid redundant I2C writes

  // Internal methods
  void calculateStepInterval();
  void generateStep();
};

#endif // MOTOR_STEPPER_H
