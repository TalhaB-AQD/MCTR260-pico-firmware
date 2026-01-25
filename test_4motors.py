"""
Simple 4-motor simultaneous step test for MechaPico MCB
Flash MicroPython to Pico, then run this script via Thonny
"""
from time import sleep
import machine

# I2C setup - same as C++ firmware
i2c = machine.I2C(0, scl=machine.Pin(5), sda=machine.Pin(4), freq=400000)

MCP_ADDR = 0x20  # Stepper MCP23017

def write_reg(reg, val):
    i2c.writeto_mem(MCP_ADDR, reg, bytes([val]))

def read_reg(reg):
    return i2c.readfrom_mem(MCP_ADDR, reg, 1)[0]

# Port A control bits
EN_BIT   = 0x08  # GPA3 - Enable (active low)
MS1_BIT  = 0x10  # GPA4
MS2_BIT  = 0x20  # GPA5
SPRD_BIT = 0x40  # GPA6
PDN_BIT  = 0x80  # GPA7 - Must be HIGH for standalone mode

# Port B step/dir bits for all 4 motors
M1_DIR = 0x01; M1_STEP = 0x02
M2_DIR = 0x04; M2_STEP = 0x08
M3_DIR = 0x10; M3_STEP = 0x20
M4_DIR = 0x40; M4_STEP = 0x80

ALL_STEPS = M1_STEP | M2_STEP | M3_STEP | M4_STEP
ALL_DIRS  = M1_DIR | M2_DIR | M3_DIR | M4_DIR

def setup():
    print("Setting up MCP23017...")
    write_reg(0x00, 0x00)  # IODIRA - all outputs
    write_reg(0x01, 0x00)  # IODIRB - all outputs
    
    # Port A: EN=0 (enabled), PDN=1 (standalone mode), MS2=1 MS1=0 (1/4 step)
    port_a = PDN_BIT | MS2_BIT  # EN=0 means enabled
    write_reg(0x14, port_a)  # OLATA
    print(f"Port A = 0x{port_a:02X}")
    
    # Port B: All directions forward, steps low
    write_reg(0x15, 0x00)  # OLATB
    print("Setup complete!")

def test_all_motors(duration_sec=10, step_period_us=500):
    """Run all 4 motors simultaneously for duration_sec"""
    print(f"\n=== Running ALL 4 MOTORS for {duration_sec} seconds ===")
    print(f"Step period: {step_period_us}us ({1000000/step_period_us:.0f} steps/sec)")
    print("Watch for power supply brownout or motor stalls...\n")
    
    # Calculate iterations
    iterations = int(duration_sec * 1_000_000 / step_period_us)
    delay_sec = step_period_us / 2_000_000  # Half period for high/low
    
    # Set all directions forward
    write_reg(0x15, ALL_DIRS)  # All dirs = 1
    
    print("Starting...")
    for i in range(iterations):
        # All steps HIGH
        write_reg(0x15, ALL_DIRS | ALL_STEPS)
        sleep(delay_sec)
        
        # All steps LOW  
        write_reg(0x15, ALL_DIRS)
        sleep(delay_sec)
        
        # Progress indicator every 1000 steps
        if i % 1000 == 0:
            print(f"  Step {i}/{iterations}")
    
    print("Done!")

def test_one_by_one(steps=500, step_period_us=500):
    """Test each motor individually"""
    print("\n=== Testing each motor individually ===")
    delay_sec = step_period_us / 2_000_000
    
    motors = [
        ("M1", M1_DIR, M1_STEP),
        ("M2", M2_DIR, M2_STEP),
        ("M3", M3_DIR, M3_STEP),
        ("M4", M4_DIR, M4_STEP),
    ]
    
    for name, dir_bit, step_bit in motors:
        print(f"Testing {name}...")
        write_reg(0x15, dir_bit)  # Set direction
        
        for _ in range(steps):
            write_reg(0x15, dir_bit | step_bit)
            sleep(delay_sec)
            write_reg(0x15, dir_bit)
            sleep(delay_sec)
        
        print(f"  {name} done")
        sleep(1)
    
    print("All individual tests complete!")

# === MAIN ===
print("\n" + "="*50)
print("  MechaPico 4-Motor Power Supply Test")
print("="*50)

setup()

# First test individually to confirm all work
test_one_by_one(steps=400, step_period_us=500)

# Then test all simultaneously - THIS is the power stress test
test_all_motors(duration_sec=10, step_period_us=500)

print("\n=== TEST COMPLETE ===")
print("If motors stopped during simultaneous test but worked individually,")
print("you likely have a power supply issue (voltage drop under load).")
