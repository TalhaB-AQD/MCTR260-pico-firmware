/**
 * @file motor_dc.h
 * @brief DC motor control class — config structs and PWM interface
 *
 * See motor_dc.cpp for H-bridge driver operation details (DRV8871, DRV8833,
 * L298N). WIRING PINS:  DRV8871/DRV8833 need pinA+pinB only. L298N needs
 * pinA+pinB+pinEnable. DIRECTION:    Set cfg_.direction to -1 to reverse a
 * physically flipped motor.
 */

#ifndef MOTOR_DC_H
#define MOTOR_DC_H

#include "motor_base.h"

/**
 * @brief DC motor driver type
 */
enum class DCDriverType {
  DRV8871, // 2 PWM pins (IN1, IN2)
  DRV8833, // 2 PWM pins per channel
  L298N    // Enable + 2 direction pins
};

/**
 * @brief DC motor configuration
 */
struct MotorDCConfig {
  uint8_t index; // Motor index (0-3 for mecanum)
  bool enabled;  // Master on/off

  DCDriverType driverType;
  int8_t pinA;      // First pin (PWM or IN1)
  int8_t pinB;      // Second pin (direction or IN2)
  int8_t pinEnable; // Enable pin (L298N only, -1 if not used)

  int8_t direction; // 1 or -1 to reverse motor
};

/**
 * @brief DC motor with open-loop PWM control
 */
class MotorDC : public MotorBase {
public:
  explicit MotorDC(const MotorDCConfig &config);

  // MotorBase interface
  bool init() override;
  void stop() override;
  void update(float dtSec) override;
  uint8_t getIndex() const override;
  bool isEnabled() const override;
  MotorType getType() const override;

  /**
   * @brief Set target PWM (-255 to +255)
   * @param pwm PWM value, negative for reverse
   */
  void setTarget(int16_t pwm);

  /**
   * @brief Get current target PWM
   */
  int16_t getTarget() const;

private:
  MotorDCConfig cfg_;
  int16_t targetPWM_;
  int16_t currentPWM_;

  void applyPWM(int16_t pwm);
};

#endif // MOTOR_DC_H
