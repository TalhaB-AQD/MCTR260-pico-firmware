/**
 * @file command_parser.cpp
 * @brief JSON command parsing implementation
 *
 * PROTOCOL:
 *   The Flutter app sends JSON commands over BLE with this structure:
 *     {"type":"control", "vehicle":"mecanum", "left":{...}, "right":{...}, ...}
 *     {"type":"heartbeat"}
 *
 *   This parser uses ArduinoJson (v7) with a JsonDocument that auto-allocates
 *   on the stack. The parsed result is stored in a control_command_t struct
 *   (see command_packet.h for the exact field layout and example JSON).
 *
 *   The parser is stateful: it keeps the last valid command in
 * s_currentCommand. This allows other modules to call command_get_current() at
 * any time to read the most recent command — useful for telemetry reporting and
 * safety logic.
 *
 * ESP32 PARITY:
 *   The JSON format and struct layout are identical to the ESP32 firmware.
 *   Both firmwares can be controlled by the same Flutter app without changes.
 */

#include "command_parser.h"
#include <Arduino.h>
#include <ArduinoJson.h>

// =============================================================================
// PRIVATE STATE
// =============================================================================

static control_command_t s_currentCommand;
static bool s_initialized = false;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

static void parseJoystickInput(JsonObject &obj, joystick_input_t *input) {
  const char *controlType = obj["control"] | "joystick";

  if (strcmp(controlType, "joystick") == 0) {
    input->isJoystick = true;
    input->x = obj["x"] | 0.0f;
    input->y = obj["y"] | 0.0f;
    input->value = 0.0f;
  } else {
    input->isJoystick = false;
    input->x = 0.0f;
    input->y = 0.0f;
    input->value = obj["value"] | 0.0f;
  }
}

// =============================================================================
// PUBLIC API
// =============================================================================

bool command_parse(const char *json, control_command_t *cmd) {
  if (!json || !cmd) {
    return false;
  }

  // Initialize on first call
  if (!s_initialized) {
    command_init(&s_currentCommand);
    s_initialized = true;
  }

  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.printf("[CMD] JSON parse error: %s\n", error.c_str());
    return false;
  }

  // Extract type
  const char *type = doc["type"] | "unknown";
  strncpy(cmd->type, type, sizeof(cmd->type) - 1);
  cmd->type[sizeof(cmd->type) - 1] = '\0';

  // Handle heartbeat
  if (strcmp(type, "heartbeat") == 0) {
    // Heartbeat received - no further parsing needed
    memcpy(&s_currentCommand, cmd, sizeof(control_command_t));
    return true;
  }

  // Handle control command
  if (strcmp(type, "control") == 0) {
    // Vehicle type
    const char *vehicle = doc["vehicle"] | "mecanum";
    strncpy(cmd->vehicle, vehicle, sizeof(cmd->vehicle) - 1);
    cmd->vehicle[sizeof(cmd->vehicle) - 1] = '\0';

    // Left input
    if (doc["left"].is<JsonObject>()) {
      JsonObject left = doc["left"];
      parseJoystickInput(left, &cmd->left);
    }

    // Right input
    if (doc["right"].is<JsonObject>()) {
      JsonObject right = doc["right"];
      parseJoystickInput(right, &cmd->right);
    }

    // Speed (0-100)
    cmd->speed = doc["speed"] | 100;

    // Auxiliary channels
    if (doc["aux"].is<JsonArray>()) {
      JsonArray auxArray = doc["aux"];
      for (size_t i = 0; i < AUX_CHANNEL_COUNT && i < auxArray.size(); i++) {
        cmd->aux[i] = auxArray[i] | 0.0f;
      }
    }

    // Toggles
    if (doc["toggles"].is<JsonArray>()) {
      JsonArray toggleArray = doc["toggles"];
      for (size_t i = 0; i < TOGGLE_COUNT && i < toggleArray.size(); i++) {
        cmd->toggles[i] = toggleArray[i] | false;
      }
    }

    // Store as current command
    memcpy(&s_currentCommand, cmd, sizeof(control_command_t));
    return true;
  }

  // Unknown command type
  Serial.printf("[CMD] Unknown command type: %s\n", type);
  return false;
}

const control_command_t *command_get_current(void) { return &s_currentCommand; }

uint8_t command_get_last_speed(void) { return s_currentCommand.speed; }

bool command_is_heartbeat(void) {
  return strcmp(s_currentCommand.type, "heartbeat") == 0;
}
