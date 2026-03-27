// Mode 2: Autonomous diagonal drive from (0,0) to (4,4)
// Toggle CH1 in the app to start. Calibrate MS_PER_METER_DIAGONAL on your surface.

#include "project_config.h"
#ifdef MOTION_PROFILE_AUTONOMOUS

#include "profile_autonomous.h"
#include "core/command_packet.h"
#include "core/motor_manager.h"
#include "drivers/mecanum_kinematics.h"
#include "pico/mutex.h"
#include <Arduino.h>

extern mutex_t g_speedMutex;
extern volatile float g_targetSpeeds[4];
extern volatile bool g_speedsUpdated;

// --- CALIBRATE THESE ---
#define SPEED                   75
#define MS_PER_METER_DIAGONAL   3000  // Measure this on your surface!
#define DRIVE_TIME_MS           (unsigned long)(5.66f * MS_PER_METER_DIAGONAL)

static bool s_running = false;
static bool s_lastToggle = false;
static unsigned long s_startTime = 0;

static void setMotors(float vx, float vy, float omega, uint8_t speed) {
    WheelSpeeds w;
    mecanum_calculate(vx, vy, omega, speed / 100.0f, &w);

    if (motors_has_steppers()) {
        float scale = STEPPER_MAX_SPEED / 100.0f;
        if (mutex_try_enter(&g_speedMutex, nullptr)) {
            g_targetSpeeds[0] = w.frontLeft * scale;
            g_targetSpeeds[1] = w.frontRight * scale;
            g_targetSpeeds[2] = w.backLeft * scale;
            g_targetSpeeds[3] = w.backRight * scale;
            g_speedsUpdated = true;
            mutex_exit(&g_speedMutex);
        }
    } else if (motors_has_dc()) {
        motor_set_pwm(0, (int16_t)(w.frontLeft * 2.55f));
        motor_set_pwm(1, (int16_t)(w.frontRight * 2.55f));
        motor_set_pwm(2, (int16_t)(w.backLeft * 2.55f));
        motor_set_pwm(3, (int16_t)(w.backRight * 2.55f));
    }
}

void profile_autonomous_apply(const control_command_t *cmd) {
    // Toggle CH1 rising edge starts the route
    bool toggle = cmd->toggles[0];
    if (toggle && !s_lastToggle) {
        s_running = true;
        s_startTime = millis();
        Serial.println("[Auto] GO");
    }
    s_lastToggle = toggle;

    if (!s_running) return;

    if (millis() - s_startTime < DRIVE_TIME_MS) {
        // Drive 45-degree diagonal (forward + right)
        setMotors(100, 100, 0, SPEED);
    } else {
        // Done
        setMotors(0, 0, 0, 0);
        s_running = false;
        Serial.println("[Auto] DONE");
    }
}

void profile_autonomous_stop() {
    setMotors(0, 0, 0, 0);
    s_running = false;
}

#endif
