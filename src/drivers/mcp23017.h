/**
 * @file mcp23017.h
 * @brief MCP23017 16-bit I2C GPIO Expander Driver
 *
 * WHY AN I2C GPIO EXPANDER?
 *   The Raspberry Pi Pico has 26 GPIO pins, but controlling 4 stepper motors
 *   (step + dir each = 8 pins), plus enable, microstepping, and spread mode
 *   (3 more pins), plus I2C itself (2 pins) would consume 13 pins — over half
 *   the Pico. The MCP23017 provides 16 additional GPIOs over just 2 I2C wires.
 *
 * MechaPico MCB Layout (custom PCB):
 *   ┌─────────────────────────────────────────────────────┐
 *   │  U6_1 (MCP23017 @ 0x20) — STEPPER CONTROL          │
 *   │    Port B: M1-M4 step/dir (8 pins)                  │
 *   │    Port A: M5 step/dir, enable, MS1, MS2, spread    │
 *   ├─────────────────────────────────────────────────────┤
 *   │  U6_2 (MCP23017 @ 0x21) — DC MOTORS & LEDs         │
 *   │    Port A: 2× DC motor H-bridge + 4× LED bar       │
 *   │    Port B: 8× general purpose 5V I/O                │
 *   └─────────────────────────────────────────────────────┘
 *
 * PERFORMANCE NOTE:
 *   Each I2C write takes ~25µs at 400kHz. A single stepperPulse() needs
 *   2 writes (high + low) = ~50µs. For 4 motors individually = ~200µs.
 *   The batch functions (stepperPulseBatchPortB) do all 4 motors in 2 writes
 *   = ~50µs total, a 4× improvement. This is why simple_stepper.cpp uses
 *   setPortB() directly instead of stepperPulse() per motor.
 */

#ifndef MCP23017_H
#define MCP23017_H

#include "hardware/i2c.h"
#include <stdint.h>

// =============================================================================
// I2C CONFIGURATION (Pico Inventor 2040 W)
// =============================================================================

#define MCP23017_I2C_PORT i2c0
#define MCP23017_I2C_SDA_PIN 4
#define MCP23017_I2C_SCL_PIN 5
#define MCP23017_I2C_FREQ 400000 // 400 kHz

// =============================================================================
// MCP23017 I2C ADDRESSES
// =============================================================================

#define MCP23017_STEPPER_ADDR 0x20 // U6_1: Stepper control
#define MCP23017_DCMOTOR_ADDR 0x21 // U6_2: DC motors & LEDs

// =============================================================================
// MCP23017 REGISTERS (IOCON.BANK = 0, sequential addressing)
// =============================================================================

// Direction registers (1 = input, 0 = output)
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01

// Input polarity (1 = inverted)
#define MCP23017_IPOLA 0x02
#define MCP23017_IPOLB 0x03

// Interrupt-on-change enable
#define MCP23017_GPINTENA 0x04
#define MCP23017_GPINTENB 0x05

// Default compare value for interrupt
#define MCP23017_DEFVALA 0x06
#define MCP23017_DEFVALB 0x07

// Interrupt control (1 = compare to DEFVAL, 0 = compare to previous)
#define MCP23017_INTCONA 0x08
#define MCP23017_INTCONB 0x09

// Configuration register
#define MCP23017_IOCON 0x0A

// Pull-up resistor (1 = enabled)
#define MCP23017_GPPUA 0x0C
#define MCP23017_GPPUB 0x0D

// Interrupt flag
#define MCP23017_INTFA 0x0E
#define MCP23017_INTFB 0x0F

// Interrupt capture (value at time of interrupt)
#define MCP23017_INTCAPA 0x10
#define MCP23017_INTCAPB 0x11

// GPIO port registers
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13

// Output latch registers
#define MCP23017_OLATA 0x14
#define MCP23017_OLATB 0x15

// =============================================================================
// U6_1 STEPPER MOTOR PIN MASKS (MCP23017 @ 0x20)
// =============================================================================

// Port B - Individual Motor STEP/DIR
#define STPR_M1_DIR_BIT (1 << 0)  // GPB0
#define STPR_M1_STEP_BIT (1 << 1) // GPB1
#define STPR_M2_DIR_BIT (1 << 2)  // GPB2
#define STPR_M2_STEP_BIT (1 << 3) // GPB3
#define STPR_M3_DIR_BIT (1 << 4)  // GPB4
#define STPR_M3_STEP_BIT (1 << 5) // GPB5
#define STPR_M4_DIR_BIT (1 << 6)  // GPB6
#define STPR_M4_STEP_BIT (1 << 7) // GPB7

// Port A - Motor 5 + Shared Control
#define LED_BAR_1_BIT (1 << 0)     // GPA0
#define STPR_M5_STEP_BIT (1 << 1)  // GPA1
#define STPR_M5_DIR_BIT (1 << 2)   // GPA2
#define STPR_ALL_EN_BIT (1 << 3)   // GPA3 - Active LOW
#define STPR_ALL_MS1_BIT (1 << 4)  // GPA4
#define STPR_ALL_MS2_BIT (1 << 5)  // GPA5
#define STPR_ALL_SPRD_BIT (1 << 6) // GPA6 - SpreadCycle
#define STPR_ALL_PDN_BIT                                                       \
  (1 << 7) // GPA7 - PDN_UART: HIGH=Standalone STEP/DIR, LOW=UART mode

// =============================================================================
// U6_2 DC MOTOR & LED PIN MASKS (MCP23017 @ 0x21)
// =============================================================================

// Port B - General Purpose 5V I/O
#define IO_5V_0_BIT (1 << 0) // GPB0
#define IO_5V_1_BIT (1 << 1) // GPB1
#define IO_5V_2_BIT (1 << 2) // GPB2
#define IO_5V_3_BIT (1 << 3) // GPB3
#define IO_5V_4_BIT (1 << 4) // GPB4
#define IO_5V_5_BIT (1 << 5) // GPB5
#define IO_5V_6_BIT (1 << 6) // GPB6
#define IO_5V_7_BIT (1 << 7) // GPB7

// Port A - DC Motor Control & LEDs
#define IN_MOT_4N_BIT (1 << 0) // GPA0
#define IN_MOT_4P_BIT (1 << 1) // GPA1
#define IN_MOT_3N_BIT (1 << 2) // GPA2
#define IN_MOT_3P_BIT (1 << 3) // GPA3
#define LED_BAR_2_BIT (1 << 4) // GPA4
#define LED_BAR_3_BIT (1 << 5) // GPA5
#define LED_BAR_4_BIT (1 << 6) // GPA6
#define LED_BAR_5_BIT (1 << 7) // GPA7

// =============================================================================
// STEPPER MOTOR INDEX MAPPING
// =============================================================================

typedef struct {
  uint8_t stepBit; // Bit mask for STEP pin
  uint8_t dirBit;  // Bit mask for DIR pin
  uint8_t port;    // 0 = GPIOA, 1 = GPIOB
} StepperPinConfig;

// Lookup table for stepper motor pins (index 0-4 = M1-M5)
static const StepperPinConfig STEPPER_PINS[5] = {
    {STPR_M1_STEP_BIT, STPR_M1_DIR_BIT, 1}, // M1: Port B
    {STPR_M2_STEP_BIT, STPR_M2_DIR_BIT, 1}, // M2: Port B
    {STPR_M3_STEP_BIT, STPR_M3_DIR_BIT, 1}, // M3: Port B
    {STPR_M4_STEP_BIT, STPR_M4_DIR_BIT, 1}, // M4: Port B
    {STPR_M5_STEP_BIT, STPR_M5_DIR_BIT, 0}, // M5: Port A
};

// =============================================================================
// MCP23017 DRIVER CLASS
// =============================================================================

class MCP23017 {
public:
  /**
   * @brief Construct driver for a specific I2C address
   * @param addr I2C address (0x20 for stepper, 0x21 for DC/LED)
   */
  explicit MCP23017(uint8_t addr);

  /**
   * @brief Initialize the I2C bus and configure MCP23017
   * @return true on success
   */
  bool init();

  /**
   * @brief Write a register
   * @param reg Register address
   * @param value Value to write
   * @return true on success
   */
  bool writeRegister(uint8_t reg, uint8_t value);

  /**
   * @brief Read a register
   * @param reg Register address
   * @return Register value (0 on error)
   */
  uint8_t readRegister(uint8_t reg);

  /**
   * @brief Set all pins on Port A
   * @param value 8-bit value for pins
   */
  void setPortA(uint8_t value);

  /**
   * @brief Set all pins on Port B
   * @param value 8-bit value for pins
   */
  void setPortB(uint8_t value);

  /**
   * @brief Get current Port A output latch
   */
  uint8_t getPortA() const { return portA_; }

  /**
   * @brief Get current Port B output latch
   */
  uint8_t getPortB() const { return portB_; }

  /**
   * @brief Set a single bit on Port A
   * @param bit Bit mask to set
   * @param state true = high, false = low
   */
  void setBitA(uint8_t bit, bool state);

  /**
   * @brief Set a single bit on Port B
   * @param bit Bit mask to set
   * @param state true = high, false = low
   */
  void setBitB(uint8_t bit, bool state);

  /**
   * @brief Toggle a bit on Port B (for step pulses)
   * @param bit Bit mask to toggle
   */
  void toggleBitB(uint8_t bit);

  /**
   * @brief Toggle a bit on Port A
   * @param bit Bit mask to toggle
   */
  void toggleBitA(uint8_t bit);

private:
  uint8_t addr_;
  uint8_t portA_; // Cached output latch A
  uint8_t portB_; // Cached output latch B
  bool initialized_;

  static bool i2cInitialized_;
  static void initI2C();
};

// =============================================================================
// GLOBAL INSTANCES
// =============================================================================

// These are declared in mcp23017.cpp
extern MCP23017 mcpStepper; // U6_1 @ 0x20
extern MCP23017 mcpDCMotor; // U6_2 @ 0x21

// =============================================================================
// CONVENIENCE FUNCTIONS FOR STEPPER CONTROL
// =============================================================================

/**
 * @brief Enable all stepper motors (set STPR_ALL_EN LOW)
 */
void stepperEnableAll();

/**
 * @brief Disable all stepper motors (set STPR_ALL_EN HIGH)
 */
void stepperDisableAll();

/**
 * @brief Set microstepping mode for all steppers
 * @param ms1 MS1 pin state
 * @param ms2 MS2 pin state
 *
 * MS1=0, MS2=0: Full step
 * MS1=1, MS2=0: 1/2 step
 * MS1=0, MS2=1: 1/4 step
 * MS1=1, MS2=1: 1/8 step
 */
void stepperSetMicrostepping(bool ms1, bool ms2);

/**
 * @brief Set direction for a specific motor
 * @param motorIndex Motor index (0-4 for M1-M5)
 * @param forward true for forward, false for reverse
 */
void stepperSetDirection(uint8_t motorIndex, bool forward);

/**
 * @brief Generate a step pulse toggle for a specific motor
 * @param motorIndex Motor index (0-4 for M1-M5)
 */
void stepperToggleStep(uint8_t motorIndex);

/**
 * @brief Generate a complete step pulse (high then low)
 * @param motorIndex Motor index (0-4 for M1-M5)
 */
void stepperPulse(uint8_t motorIndex);

/**
 * @brief Generate step pulses for multiple motors in a single I2C transaction
 * @param stepMask Bitmask of STEP bits to pulse (e.g., STPR_M1_STEP_BIT |
 * STPR_M3_STEP_BIT)
 *
 * This is much more efficient than calling stepperPulse() for each motor,
 * as it uses only 2 I2C writes total instead of 2 per motor.
 * Only works for motors M1-M4 which are on Port B.
 */
void stepperPulseBatchPortB(uint8_t stepMask);

/**
 * @brief Set direction bits for multiple motors in a single I2C transaction
 * @param dirMask Bitmask of direction bits to set HIGH
 * @param clearMask Bitmask of direction bits to set LOW
 *
 * Efficiently sets multiple motor directions with one I2C write.
 */
void stepperSetDirectionBatch(uint8_t setHighMask, uint8_t setLowMask);

#endif // MCP23017_H
