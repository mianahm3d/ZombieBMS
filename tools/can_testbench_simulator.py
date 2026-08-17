#!/usr/bin/env python3
"""
OpenEV-BMS Real-Time CAN Bus & Pack Simulator
Simulates:
- Isabellenhütte IVT-S 1000A CAN Stream (0x521-0x528)
- 96s Nissan Leaf Cell Voltage & NTC Thermistor Pack Telemetry
- ZombieVerter VCU Inverter & Drive Cycling
"""

import time
import struct
import math

class BMS_Simulator:
    def __init__(self):
        self.cells = [3.700 + (math.sin(i * 0.4) * 0.006) for i in range(96)]
        self.temps = [24.0 + (math.cos(i * 0.2) * 1.5) for i in range(54)]
        self.pack_voltage = sum(self.cells)
        self.inverter_voltage = 0.0
        self.current_a = 0.0
        self.coulomb_ah = 57.5
        self.soc_pct = 50.0
        self.state = "STANDBY"
        self.main_pos_closed = False
        self.main_neg_closed = False
        self.precharge_closed = False

    def step(self, delta_t_s: float, kl15_ignition: bool):
        # 1. State Machine
        if self.state == "STANDBY" and kl15_ignition:
            self.state = "PRECHARGING"
            self.main_neg_closed = True
            self.precharge_closed = True
            self.inverter_voltage = 10.0
            print("\n[BMS STATE] -> PRECHARGING (Main- ON, Precharge ON)")

        elif self.state == "PRECHARGING":
            # Simulate RC charging of Inverter DC-link capacitor
            self.inverter_voltage += (self.pack_voltage - self.inverter_voltage) * (delta_t_s / 0.3)
            ratio = self.inverter_voltage / self.pack_voltage
            if ratio >= 0.95:
                self.state = "DRIVE"
                self.main_pos_closed = True
                self.precharge_closed = False
                print(f"[BMS STATE] -> DRIVE ENGAGED (U2={self.inverter_voltage:.1f}V / U1={self.pack_voltage:.1f}V = {ratio*100:.1f}%)")

        elif self.state == "DRIVE":
            if not kl15_ignition:
                self.state = "STANDBY"
                self.main_pos_closed = False
                self.main_neg_closed = False
                print("\n[BMS STATE] -> STANDBY (Ignition OFF, Contactors OPEN)")
            else:
                # Simulate drive current pulse
                self.current_a = -45.0 + math.sin(time.time()) * 30.0
                self.coulomb_ah += (self.current_a * delta_t_s) / 3600.0
                self.soc_pct = max(0.0, min(100.0, (self.coulomb_ah / 115.0) * 100.0))

    def generate_ivts_frames(self):
        # Frame 0x521: Current in mA
        curr_ma = int(self.current_a * 1000.0)
        f_521 = bytes([0x00, 0x00]) + struct.pack(">i", curr_ma) + bytes([0x00, 0x00])

        # Frame 0x522: Pack Voltage U1 in mV
        u1_mv = int(self.pack_voltage * 1000.0)
        f_522 = bytes([0x00, 0x00]) + struct.pack(">i", u1_mv) + bytes([0x00, 0x00])

        # Frame 0x523: Inverter Voltage U2 in mV
        u2_mv = int(self.inverter_voltage * 1000.0)
        f_523 = bytes([0x00, 0x00]) + struct.pack(">i", u2_mv) + bytes([0x00, 0x00])

        return {"0x521": f_521.hex().upper(), "0x522": f_522.hex().upper(), "0x523": f_523.hex().upper()}

    def generate_zombieverter_frame(self):
        # 0x6B0: Pack Current (0.1A), Pack V (0.1V), SoC (0.5%), DCL, CCL
        c_01a = int(self.current_a * 10.0)
        v_01v = int(self.pack_voltage * 10.0)
        soc_byte = int(self.soc_pct * 2.0)
        dcl = 350
        ccl = 150
        payload = struct.pack(">hhBHB", c_01a, v_01v, soc_byte, dcl, ccl)
        return payload.hex().upper()

def run_simulation():
    sim = BMS_Simulator()
    print("======================================================================")
    print("             OpenEV-BMS CAN & TRACTION SIMULATOR STARTING              ")
    print("======================================================================")
    
    # Run 15 simulation ticks (Ignition ON at tick 2, OFF at tick 12)
    for tick in range(15):
        kl15 = (2 <= tick < 12)
        sim.step(0.1, kl15)
        ivts = sim.generate_ivts_frames()
        zombie = sim.generate_zombieverter_frame()
        
        print(f"[Tick {tick:02d}] State: {sim.state:11s} | Pack: {sim.pack_voltage:.1f}V | Inv U2: {sim.inverter_voltage:.1f}V | Current: {sim.current_a:+.1f}A | SoC: {sim.soc_pct:.1f}% | CAN 0x6B0: {zombie}")
        time.sleep(0.05)

    print("======================================================================")
    print("                      SIMULATION RUN COMPLETE                         ")
    print("======================================================================")

if __name__ == "__main__":
    run_simulation()
