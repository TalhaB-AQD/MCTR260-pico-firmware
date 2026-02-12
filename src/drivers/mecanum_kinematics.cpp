/**
 * @file mecanum_kinematics.cpp
 * @brief Mecanum wheel inverse kinematics implementation
 *
 * Converts joystick input to individual wheel speeds for omnidirectional movement.
 */

#include "mecanum_kinematics.h"
#include <math.h>

void mecanum_calculate(float vx, float vy, float omega,
                       float speedMultiplier,
                       WheelSpeeds* output) {
    // NOTE: Deadzone filtering is handled by the Flutter app at input level
    // NOTE: Deadzone filtering is handled by the Flutter app at input level
    // This function does pure kinematics math only
    
    // If all inputs are zero, output zero
    if (vx == 0 && vy == 0 && omega == 0) {
        output->frontLeft = 0;
        output->frontRight = 0;
        output->backLeft = 0;
        output->backRight = 0;
        return;
    }
    
    // ==========================================================================
    // MECANUM INVERSE KINEMATICS
    // ==========================================================================
    // For a mecanum robot with rollers at 45 degrees:
    //   FL = vy + vx + omega
    //   FR = vy - vx - omega
    //   BL = vy - vx + omega
    //   BR = vy + vx - omega
    //
    // Note: Signs may need adjustment based on motor/wheel orientation
    // ==========================================================================
    
    float fl = vy + vx + omega;
    float fr = vy - vx - omega;
    float bl = vy - vx + omega;
    float br = vy + vx - omega;
    
    // Find the maximum absolute value
    float maxVal = fabsf(fl);
    if (fabsf(fr) > maxVal) maxVal = fabsf(fr);
    if (fabsf(bl) > maxVal) maxVal = fabsf(bl);
    if (fabsf(br) > maxVal) maxVal = fabsf(br);
    
    // Normalize to -100..+100 range if any value exceeds it
    if (maxVal > 100.0f) {
        float scale = 100.0f / maxVal;
        fl *= scale;
        fr *= scale;
        bl *= scale;
        br *= scale;
    }
    
    // Apply speed multiplier
    output->frontLeft = fl * speedMultiplier;
    output->frontRight = fr * speedMultiplier;
    output->backLeft = bl * speedMultiplier;
    output->backRight = br * speedMultiplier;
}
