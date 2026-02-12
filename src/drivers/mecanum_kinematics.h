/**
 * @file mecanum_kinematics.h
 * @brief Mecanum wheel inverse kinematics — (vx, vy, ω) → 4 wheel speeds
 *
 * See mecanum_kinematics.cpp for the full equations and normalization logic.
 * Roller angle convention: 45° from wheel axis (standard X-configuration).
 */

#ifndef MECANUM_KINEMATICS_H
#define MECANUM_KINEMATICS_H

/**
 * @brief Wheel speeds for mecanum drive
 */
struct WheelSpeeds {
  float frontLeft;
  float frontRight;
  float backLeft;
  float backRight;
};

/**
 * @brief Calculate wheel speeds from joystick input
 *
 * @param vx Strafe velocity (-100 to +100)
 * @param vy Forward velocity (-100 to +100)
 * @param omega Rotation velocity (-100 to +100)
 * @param speedMultiplier Speed multiplier (0.0 to 1.0)
 * @param output Output wheel speeds
 *
 * @note Deadzone filtering is handled by the Flutter app at input level
 */
void mecanum_calculate(float vx, float vy, float omega, float speedMultiplier,
                       WheelSpeeds *output);

#endif // MECANUM_KINEMATICS_H
