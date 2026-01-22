/**
 * @file main.cpp
 * @brief Main Entry Point for Raspberry Pi Pico W Robot Controller
 *
 * This firmware enables BLE control of a mecanum wheel robot
 * using the RC Controller Flutter app.
 */

#include <Arduino.h>
#include <Wire.h>

// Project configuration
#include "project_config.h"

// Core modules
#include "core/ble_controller.h"
#include "core/command_parser.h"
#include "core/motor_manager.h"
#include "core/safety.h"
#include "drivers/mcp23017.h"

// Profiles
#include "profiles/profile_mecanum.h"

// =============================================================================
// GLOBAL STATE
// =============================================================================

static bool s_bleConnected = false;
static unsigned long s_lastUpdateTime = 0;

// =============================================================================
// BLE CALLBACKS
// =============================================================================

/**
 * @brief Called when a command is received over BLE
 */
void onBleCommand(const char* jsonData, uint16_t length) {
    // Feed safety watchdog
    safety_feed();
    
    // Parse command
    control_command_t cmd;
    if (!command_parse(jsonData, &cmd)) {
        return;
    }
    
    // Skip heartbeats (they just keep connection alive)
    if (command_is_heartbeat()) {
        return;
    }
    
    // Apply motion profile based on vehicle type
    if (strcmp(cmd.vehicle, "mecanum") == 0) {
#ifdef MOTION_PROFILE_MECANUM
        profile_mecanum_apply(&cmd);
#endif
    }
    // Add other profiles here as needed
}

/**
 * @brief Called when BLE connection state changes
 */
void onBleConnectionChange(bool connected) {
    s_bleConnected = connected;
    
    if (connected) {
        Serial.println(">>> Client connected!");
    } else {
        Serial.println(">>> Client disconnected - stopping motors");
        motors_stop_all();
    }
}

// =============================================================================
// ARDUINO SETUP
// =============================================================================

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    
    // Wait 10 seconds for serial monitor reconnection after flash
    // (Pico requires BOOTSEL mode for flashing, so monitor disconnects)
    delay(10000);
    while (!Serial && millis() < 13000) {
        // Wait for serial (additional 3 seconds if not ready)
    }
    
    Serial.println();
    Serial.println("===========================================");
    Serial.println("   RC Robot Controller - Pico W Edition");
    Serial.println("===========================================");
    Serial.printf("Device Name: %s\n", DEVICE_NAME);
    Serial.printf("PIN Code: %06d\n", BLE_PASSKEY);
    Serial.println();
    
    // Initialize LED for status
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    // Initialize safety watchdog
    safety_init();
    
    // =========================================================================
    // I2C BUS SCAN (with pin auto-detection)
    // Some MechaPico MCB revisions use GPIO 9/10 instead of GPIO 4/5
    // =========================================================================
    Serial.println("[I2C] Scanning for MCP23017...");
    
    // Try GPIO 4/5 first (standard)
    int devicesOnPins4_5 = 0;
    Wire.setSDA(4);
    Wire.setSCL(5);
    Wire.begin();
    for (uint8_t addr = 0x20; addr <= 0x21; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C] GPIO 4/5: Found device at 0x%02X\n", addr);
            devicesOnPins4_5++;
        }
    }
    Wire.end();
    
    // Try GPIO 9/10 (alternate MechaPico MCB revision)
    int devicesOnPins9_10 = 0;
    Wire.setSDA(8);  // GPIO 8 for SDA on I2C0
    Wire.setSCL(9);  // GPIO 9 for SCL on I2C0
    Wire.begin();
    for (uint8_t addr = 0x20; addr <= 0x21; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C] GPIO 8/9: Found device at 0x%02X\n", addr);
            devicesOnPins9_10++;
        }
    }
    Wire.end();
    
    if (devicesOnPins4_5 == 0 && devicesOnPins9_10 == 0) {
        Serial.println("[I2C] WARNING: No MCP23017 found on GPIO 4/5 OR GPIO 8/9!");
        Serial.println("[I2C] Check: 3.3V power to MCP23017, physical wiring");
    } else if (devicesOnPins9_10 > devicesOnPins4_5) {
        Serial.println("[I2C] >>> MCP23017 found on GPIO 8/9 - UPDATE mcp23017.h! <<<");
        Serial.println("[I2C] Change MCP23017_I2C_SDA_PIN to 8, MCP23017_I2C_SCL_PIN to 9");
    } else {
        Serial.printf("[I2C] MCP23017 found on GPIO 4/5 (%d devices)\n", devicesOnPins4_5);
    }
    
    // =========================================================================
    // MOTOR INITIALIZATION
    // =========================================================================
    Serial.println("[Setup] Initializing motors...");
    bool motorsOk = motors_init();
    if (!motorsOk) {
        Serial.println("ERROR: Motor initialization failed!");
        Serial.println("       MCP23017 not responding - check I2C wiring");
        // Continue anyway to allow BLE debugging
    } else {
        Serial.println("[Setup] Motors initialized OK");
    }
    
    // STEPPER HARDWARE TEST with I2C verification (only if motors initialized)
#if defined(STEPPER_DRIVER_TMC2209) || defined(STEPPER_DRIVER_A4988) || defined(STEPPER_DRIVER_DRV8825)
    if (motorsOk) {
        Serial.println("[TEST] I2C Verification test...");
        
        // Read current Port B value
        uint8_t readBack = mcpStepper.readRegister(MCP23017_OLATB);
        Serial.printf("[TEST] Port B initial: 0x%02X\n", readBack);
        
        // Write 0xAA to Port B
        mcpStepper.setPortB(0xAA);
        readBack = mcpStepper.readRegister(MCP23017_OLATB);
        Serial.printf("[TEST] Port B after 0xAA write: 0x%02X %s\n", readBack, (readBack == 0xAA) ? "OK" : "FAIL!");
        
        // Write 0x55 to Port B
        mcpStepper.setPortB(0x55);
        readBack = mcpStepper.readRegister(MCP23017_OLATB);
        Serial.printf("[TEST] Port B after 0x55 write: 0x%02X %s\n", readBack, (readBack == 0x55) ? "OK" : "FAIL!");
        
        // Reset to 0
        mcpStepper.setPortB(0x00);
        
        // Light up LED_BAR_1 to indicate I2C is working
        mcpStepper.setBitA(LED_BAR_1_BIT, true);
        Serial.println("[TEST] LED_BAR_1 should be ON");
        
        // Now test stepping - 200 steps on Motor 0 only (GPB1 = STEP)
        Serial.println("[TEST] Sending 200 step pulses to Motor 0 (GPB1)...");
        stepperEnableAll();
        stepperSetDirection(0, true);
        
        for (int step = 0; step < 200; step++) {
            stepperPulse(0);
            delayMicroseconds(2000);  // Slower: 500 steps/sec
        }
        Serial.println("[TEST] Done - Motor 0 should have moved ~1 revolution");
    }
#endif
    
    // =========================================================================
    // BLE INITIALIZATION (after motors to avoid I2C conflicts)
    // =========================================================================
    ble_init(onBleCommand, onBleConnectionChange);
    
    Serial.println();
    Serial.println("Setup complete - waiting for BLE connection...");
    Serial.println();
    
    s_lastUpdateTime = millis();
}

// =============================================================================
// ARDUINO LOOP
// =============================================================================

void loop() {
    // Process BLE events
    ble_update();
    
    // Motor update at 50Hz
    unsigned long now = millis();
    if (now - s_lastUpdateTime >= MOTOR_UPDATE_INTERVAL_MS) {
        float dtSec = (now - s_lastUpdateTime) / 1000.0f;
        s_lastUpdateTime = now;
        
        // Check safety timeout
        if (s_bleConnected && safety_check_timeout()) {
            // Timeout triggered - motors already stopped by safety module
        }
        
        // Update motor control loops
        motors_update(dtSec);
    }
    
    // Status LED: blink when disconnected, solid when connected
    if (!s_bleConnected) {
        static unsigned long lastBlink = 0;
        if (now - lastBlink >= 500) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            lastBlink = now;
        }
    }
}
