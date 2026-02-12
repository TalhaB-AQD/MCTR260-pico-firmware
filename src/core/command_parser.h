/**
 * @file command_parser.h
 * @brief JSON command parsing — BLE JSON string → control_command_t struct
 *
 * See command_parser.cpp for full protocol format and ArduinoJson details.
 * Thread safety: called only on Core 0 (BLE callback context).
 */

#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "command_packet.h"

/**
 * @brief Parse JSON command string into control_command_t
 * @param json JSON string to parse
 * @param cmd Output command structure
 * @return true if parsing succeeded
 */
bool command_parse(const char *json, control_command_t *cmd);

/**
 * @brief Get the current (most recent) command
 */
const control_command_t *command_get_current(void);

/**
 * @brief Get the last speed setting (for safety validation)
 */
uint8_t command_get_last_speed(void);

/**
 * @brief Check if the current command is a heartbeat
 */
bool command_is_heartbeat(void);

#endif // COMMAND_PARSER_H
