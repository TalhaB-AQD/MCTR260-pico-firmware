/**
 * @file ble_config.h
 * @brief BLE UUIDs and constants for Pico W
 *
 * UUID COORDINATION (three codebases must agree!):
 *   These UUIDs are shared between three codebases:
 *     1. ESP32 firmware  → esp32_firmware/src/ble_config.h
 *     2. Pico firmware   → this file
 *     3. Flutter app     → lib/services/ble_service.dart
 *
 *   If you change a UUID here, you MUST change it in all three places.
 *   A UUID mismatch will cause the app to connect but fail to discover
 *   characteristics — commands won't be received and telemetry won't be sent.
 */

#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

// =============================================================================
// SERVICE UUIDs (Must match ESP32 and Flutter app)
// =============================================================================

// Main control service
#define CONTROL_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// =============================================================================
// CHARACTERISTIC UUIDs
// =============================================================================

// Control commands (app writes JSON here)
#define CONTROL_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Telemetry/messages (firmware sends notifications here)
#define TELEMETRY_CHAR_UUID "beb5483f-36e1-4688-b7f5-ea07361b26a8"

// =============================================================================
// BLE PARAMETERS
// =============================================================================

// Maximum attribute value length (MTU-3)
#define BLE_ATT_MAX_VALUE_LEN 512

// Connection interval (in 1.25ms units)
#define BLE_CONN_INTERVAL_MIN 6  // 7.5ms
#define BLE_CONN_INTERVAL_MAX 24 // 30ms

#endif // BLE_CONFIG_H
