"""
Wheel Mapping Test - runs each motor one at a time for 30 seconds
"""
from time import sleep
import machine

i2c = machine.I2C(0, scl=machine.Pin(5), sda=machine.Pin(4), freq=400000)
MCP_ADDR = 0x20

def write_reg(reg, val):
    i2c.writeto_mem(MCP_ADDR, reg, bytes([val]))

# Port A control bits
PDN_BIT  = 0x80
MS2_BIT  = 0x20

# Port B step/dir bits
M1_DIR = 0x01; M1_STEP = 0x02
M2_DIR = 0x04; M2_STEP = 0x08
M3_DIR = 0x10; M3_STEP = 0x20
M4_DIR = 0x40; M4_STEP = 0x80

def setup():
    write_reg(0x00, 0x00)  # IODIRA
    write_reg(0x01, 0x00)  # IODIRB
    write_reg(0x14, PDN_BIT | MS2_BIT)  # OLATA
    write_reg(0x15, 0x00)  # OLATB

def run_motor(name, dir_bit, step_bit, seconds=30):
    print(f"\n{'='*40}")
    print(f"  NOW RUNNING: {name}")
    print(f"  Watch which wheel spins!")
    print(f"{'='*40}")
    
    iterations = int(seconds * 1000000 / 500)
    delay = 0.00025
    
    for i in range(iterations):
        write_reg(0x15, dir_bit | step_bit)
        sleep(delay)
        write_reg(0x15, dir_bit)
        sleep(delay)
        if i % 10000 == 0:
            print(f"  {int((iterations-i)*0.0005)}s remaining...")
    
    write_reg(0x15, 0)
    print(f"  {name} DONE")
    sleep(2)

# MAIN
print("\n" + "="*50)
print("  WHEEL MAPPING TEST")
print("  Each motor runs for 30 seconds")
print("="*50)

setup()

run_motor("Motor 1 (C++ FL - Front Left)", M1_DIR, M1_STEP, 30)
run_motor("Motor 2 (C++ FR - Front Right)", M2_DIR, M2_STEP, 30)
run_motor("Motor 3 (C++ RL - Rear Left)", M3_DIR, M3_STEP, 30)
run_motor("Motor 4 (C++ RR - Rear Right)", M4_DIR, M4_STEP, 30)

print("\n=== TEST COMPLETE ===")
print("Note which physical wheel moved for each motor.")
