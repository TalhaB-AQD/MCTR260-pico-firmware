/**
 * @file ble_controller.h
 * @brief BLE GATT server public API — init, callbacks, and telemetry
 *
 * THREAD SAFETY:
 *   All functions here run on Core 0. BLE callbacks fire from BTstack's
 *   event loop (also Core 0), so no cross-core synchronization needed.
 *
 * INIT ORDER: ble_init() must be called LAST in setup() because BTstack
 * takes over Core 0's scheduling. Anything after ble_init() won't run
 * until the first BTstack idle callback.
 */

#ifndef BLE_CONTROLLER_H
#define BLE_CONTROLLER_H

#include <stdint.h>

// =============================================================================
// CALLBACK TYPES
// =============================================================================

// Called when a control command is received
typedef void (*CommandReceivedCallback)(const char *jsonData, uint16_t length);

// Called when connection state changes
typedef void (*ConnectionStateCallback)(bool connected);

// =============================================================================
// PUBLIC API
// =============================================================================

/**
 * @brief Initialize BLE GATT server
 * @param onCommand Callback for received commands
 * @param onConnection Callback for connection state changes
 */
void ble_init(CommandReceivedCallback onCommand,
              ConnectionStateCallback onConnection);

/**
 * @brief Process BLE events (call from main loop)
 */
void ble_update(void);

/**
 * @brief Check if a client is connected
 */
bool ble_is_connected(void);

/**
 * @brief Check if first write received (connection handshake complete)
 */
bool ble_first_write_received(void);

/**
 * @brief Send telemetry notification to connected client
 * @param jsonData JSON string to send
 * @return true if sent successfully
 */
bool ble_send_telemetry(const char *jsonData);

/**
 * @brief Get time of last received command (microseconds)
 */
unsigned long ble_get_last_command_time(void);

#endif // BLE_CONTROLLER_H
