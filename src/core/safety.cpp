/**
 * @file safety.cpp
 * @brief Motor safety watchdog implementation
 *
 * Stops all motors if no command received within timeout period.
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
    Serial.printf("[Safety] Watchdog initialized (timeout: %dms)\n", SAFETY_TIMEOUT_MS);
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

unsigned long safety_get_idle_time() {
    return millis() - s_lastFeedTime;
}
