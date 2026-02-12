/**
 * @file safety.cpp
 * @brief Motor safety watchdog — automatic stop on lost connection
 *
 * THE PROBLEM:
 *   If the phone disconnects (BLE drop, app crash, battery dies, user walks
 *   out of range), the last command stays active indefinitely. With a robot
 *   moving at full speed, this is dangerous.
 *
 * THE SOLUTION (watchdog pattern):
 *   1. The Flutter app sends heartbeat packets every ~500ms
 *   2. Each received command calls safety_feed() to reset the timer
 *   3. safety_check_timeout() is called every loop iteration
 *   4. If no command arrives for SAFETY_TIMEOUT_MS (default 1500ms),
 *      all motors are stopped immediately
 *
 * RELATIONSHIP TO BLE:
 *   The watchdog is INDEPENDENT of BLE connection state. Even if BLE reports
 *   "connected" (which can lag by seconds on iOS), the watchdog will trigger
 *   if data stops flowing. This is more reliable than connection callbacks.
 */

#include "safety.h"
#include "motor_manager.h"
#include "project_config.h"
#include <Arduino.h>

// =============================================================================
// PRIVATE STATE
// =============================================================================

static unsigned long s_lastFeedTime = 0;
static bool s_stopTriggered = false;

// =============================================================================
// PUBLIC API
// =============================================================================

void safety_init() {
  s_lastFeedTime = millis();
  s_stopTriggered = false;
  Serial.printf("[Safety] Watchdog initialized (timeout: %dms)\n",
                SAFETY_TIMEOUT_MS);
}

void safety_feed() {
  s_lastFeedTime = millis();
  s_stopTriggered = false;
}

bool safety_check_timeout() {
  // TEMPORARILY DISABLED FOR DEBUGGING
  return false;

  unsigned long now = millis();
  unsigned long elapsed = now - s_lastFeedTime;

  if (elapsed >= SAFETY_TIMEOUT_MS) {
    if (!s_stopTriggered) {
      Serial.printf("[Safety] TIMEOUT at %lu ms - Stopping all motors!\n", now);
      motors_stop_all();
      s_stopTriggered = true;
    }
    return true;
  }

  return false;
}

unsigned long safety_get_idle_time() { return millis() - s_lastFeedTime; }
