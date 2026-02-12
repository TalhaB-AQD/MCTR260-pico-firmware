/**
 * @file command_packet.h
 * @brief Control command structure — the data contract between Flutter and
 * firmware
 *
 * This struct mirrors the JSON format sent by the Flutter app. Both the ESP32
 * and Pico firmwares use the same struct so either can be a drop-in
 * replacement.
 *
 * VEHICLE TYPE → CONTROL MAPPING (Flutter app side):
 *   mecanum:     left = dial (rotation ω)    right = joystick (vx, vy)
 *   fourwheel:   left = dial (steering)      right = slider (throttle)
 *   tracked:     left = slider (L track)     right = slider (R track)
 *   dual:        left = joystick             right = joystick
 */

#ifndef COMMAND_PACKET_H
#define COMMAND_PACKET_H

#include <cstring>
#include <stdint.h>

// =============================================================================
// COMMAND PACKET STRUCTURE
// =============================================================================

// Maximum number of auxiliary channels
#define AUX_CHANNEL_COUNT 6

// Maximum number of toggle switches
#define TOGGLE_COUNT 6

/**
 * @brief Joystick input data
 */
typedef struct {
  bool isJoystick; // true = joystick, false = dial
  float x;         // -100 to +100 for joystick
  float y;         // -100 to +100 for joystick
  float value;     // -100 to +100 for dial
} joystick_input_t;

/**
 * @brief Complete control command from app
 *
 * Matches the JSON structure sent by the Flutter app:
 * {
 *   "type": "control",
 *   "vehicle": "mecanum",
 *   "left": {"control": "joystick", "x": 0.0, "y": 50.0},
 *   "right": {"control": "dial", "value": 0.0},
 *   "speed": 80,
 *   "aux": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
 *   "toggles": [false, false, false, false, false, false]
 * }
 */
typedef struct {
  char type[16];                // "control", "heartbeat", etc.
  char vehicle[16];             // "mecanum", "fourwheel", "tracked", "dual"
  joystick_input_t left;        // Left control input
  joystick_input_t right;       // Right control input
  uint8_t speed;                // Speed multiplier (0-100%)
  float aux[AUX_CHANNEL_COUNT]; // Auxiliary channels
  bool toggles[TOGGLE_COUNT];   // Toggle switches
} control_command_t;

// =============================================================================
// DEFAULT VALUES
// =============================================================================

/**
 * @brief Initialize command to safe defaults (all zeros)
 */
static inline void command_init(control_command_t *cmd) {
  memset(cmd, 0, sizeof(control_command_t));
  strcpy(cmd->type, "none");
  strcpy(cmd->vehicle, "mecanum");
}

#endif // COMMAND_PACKET_H
