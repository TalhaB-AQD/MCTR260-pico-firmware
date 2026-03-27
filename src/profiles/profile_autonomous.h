/**
 * @file profile_autonomous.h
 * @brief Mode 2 Autonomous Route Navigation profile.
 *
 * Executes a preplanned sequence of timed movement segments using
 * dead-reckoning (open-loop). The route is defined as a table of
 * (vx, vy, omega, speed, duration) entries in the .cpp file.
 *
 * TRIGGERING:
 *   Toggle 0 on the Flutter app starts the autonomous sequence.
 *   Once running, no further manual input is needed.
 *   Toggle 0 again to reset and re-arm.
 *
 * CALIBRATION:
 *   Durations must be tuned experimentally on the actual arena surface.
 *   Measure how long the robot takes to travel 1m at a given speed,
 *   then scale durations accordingly.
 *
 * @see profile_mecanum.h for comparison (manual control profile)
 */

#ifndef PROFILE_AUTONOMOUS_H
#define PROFILE_AUTONOMOUS_H

#include "core/command_packet.h"

/**
 * @brief Process one control cycle of the autonomous profile.
 *
 * Called from onBleCommand() on every BLE update (~50Hz).
 * Monitors toggles[0] for the start trigger, then drives through
 * the preplanned route segments using timed open-loop commands.
 *
 * @param cmd  Current control command from the app (used for toggle state).
 */
void profile_autonomous_apply(const control_command_t *cmd);

/**
 * @brief Emergency stop for autonomous mode.
 *
 * Resets the autonomous state machine. Called on BLE disconnect.
 */
void profile_autonomous_stop();

#endif // PROFILE_AUTONOMOUS_H
