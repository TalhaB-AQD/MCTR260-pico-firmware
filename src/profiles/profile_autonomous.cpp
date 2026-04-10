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

#define SPEED        75    // 0-100
#define MS_PER_METER 3920  // milliseconds to travel 1 meter at SPEED
#define TURN_45_MS   1000   // milliseconds to rotate 45 degrees in place
#define STOP_MS      1000  // pause between steps so motors finish decelerating

// Route: start bottom-left, rotate 45° to face top-right, drive 5.66m diagonal
// vx = strafe (+right), vy = forward (+forward), omega = rotation (+clockwise)
struct Step { float vx, vy, omega; unsigned long ms; };
static const Step STEPS[] = {
    {  0,   0, 100, TURN_45_MS          },  // Rotate 45° clockwise to face top-right
    {  0,   0,   0, STOP_MS             },  // Stop - let rotation settle
    {  0, 100,   0, (unsigned long)(5.66f * MS_PER_METER) },  // Drive 5.66m straight
};
static const int NUM_STEPS = sizeof(STEPS) / sizeof(STEPS[0]);

static bool          s_running    = false;
static bool          s_lastToggle = false;
static int           s_step       = 0;
static unsigned long s_stepStart  = 0;

static void drive(float vx, float vy, float omega) {
    WheelSpeeds w;
    mecanum_calculate(vx, vy, omega, SPEED / 100.0f, &w);

    float scale = STEPPER_MAX_SPEED / 100.0f;
    if (mutex_try_enter(&g_speedMutex, nullptr)) {
        g_targetSpeeds[0] = w.frontLeft  * scale;
        g_targetSpeeds[1] = w.frontRight * scale;
        g_targetSpeeds[2] = w.backLeft   * scale;
        g_targetSpeeds[3] = w.backRight  * scale;
        g_speedsUpdated = true;
        mutex_exit(&g_speedMutex);
    }
}

void profile_autonomous_apply(const control_command_t *cmd) {
    bool toggle = cmd->toggles[0];
    if (toggle && !s_lastToggle) {
        s_running   = true;
        s_step      = 0;
        s_stepStart = millis();
        Serial.println("[Auto] GO - rotate 45, drive 5.66m");
    }
    s_lastToggle = toggle;

    if (!s_running) return;

    if (millis() - s_stepStart >= STEPS[s_step].ms) {
        s_step++;
        s_stepStart = millis();
    }

    if (s_step < NUM_STEPS) {
        drive(STEPS[s_step].vx, STEPS[s_step].vy, STEPS[s_step].omega);
    } else {
        drive(0, 0, 0);
        s_running = false;
        Serial.println("[Auto] DONE - top-right corner reached");
    }
}

void profile_autonomous_stop() {
    drive(0, 0, 0);
    s_running = false;
}

#endif
