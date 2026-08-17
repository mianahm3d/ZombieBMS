#!/usr/bin/env python3
"""
OpenEV-BMS Automated Verification Test Suite
Tests:
1. LTC6813 PEC15 CRC Checksum calculation against official test vectors
2. Isabellenhütte IVT-S 1000A CAN frame decoding
3. Precharge safety ratio calculation (U2 / U1 >= 0.95)
4. Dynamic CCL / DCL derating algorithms for Nissan Leaf 40kWh pack
5. ZombieVerter VCU Orion CAN message framing (0x6B0 / 0x3C)
"""

import sys
import struct

# ==============================================================================
# 1. PEC15 CRC Verification (LTC6813-1)
# ==============================================================================
def calculate_pec15(data_bytes: bytes) -> int:
    remainder = 16  # Seed = 0x0010
    poly = 0x4599   # x^15 + x^14 + x^10 + x^8 + x^7 + x^4 + x^3 + 1
    
    for byte in data_bytes:
        remainder ^= (byte << 7)
        for _ in range(8):
            if remainder & 0x4000:
                remainder = ((remainder << 1) ^ poly) & 0x7FFF
            else:
                remainder = (remainder << 1) & 0x7FFF
    return remainder << 1

def test_pec15_checksum():
    print("[TEST 1] LTC6813 PEC15 CRC Checksum Vector Test...")
    # Test Vector 1: WRCFGA Command (0x00, 0x01) -> Expected PEC = 0x3D6E
    cmd1 = bytes([0x00, 0x01])
    pec1 = calculate_pec15(cmd1)
    assert pec1 == 0x3D6E, f"PEC15 Mismatch on WRCFGA! Got: 0x{pec1:04X}, Expected: 0x3D6E"
    
    # Test Vector 2: ADCV Command (0x03, 0x70) -> Expected PEC = 0xAF42
    cmd2 = bytes([0x03, 0x70])
    pec2 = calculate_pec15(cmd2)
    assert pec2 == 0xAF42, f"PEC15 Mismatch on ADCV! Got: 0x{pec2:04X}, Expected: 0xAF42"
    
    # Test Vector 3: RDCVA Command (0x00, 0x04) -> Expected PEC = 0x07C2
    cmd3 = bytes([0x00, 0x04])
    pec3 = calculate_pec15(cmd3)
    assert pec3 == 0x07C2, f"PEC15 Mismatch on RDCVA! Got: 0x{pec3:04X}, Expected: 0x07C2"
    
    print("  -> PASSED: All LTC6813 PEC15 test vectors matched perfectly.\n")

# ==============================================================================
# 2. Isabellenhütte IVT-S 1000A CAN Decoder Test
# ==============================================================================
def decode_ivts_can(can_id: int, data: bytes):
    raw_val = struct.unpack(">i", data[2:6])[0]  # Big-endian 32-bit signed integer
    if can_id == 0x521: # Current in mA
        return {"type": "CURRENT", "value_A": raw_val / 1000.0}
    elif can_id == 0x522: # Pack Voltage U1 in mV
        return {"type": "VOLTAGE_U1", "value_V": raw_val / 1000.0}
    elif can_id == 0x523: # Inverter Voltage U2 in mV
        return {"type": "VOLTAGE_U2", "value_V": raw_val / 1000.0}
    elif can_id == 0x527: # Charge in Ampere-seconds (As)
        return {"type": "COULOMB", "value_Ah": raw_val / 3600.0}
    return None

def test_ivts_can_decoding():
    print("[TEST 2] Isabellenhütte IVT-S 1000A CAN Decoder Test...")
    # Frame 0x521: Discharge Current = -150.250 A (-150250 mA)
    curr_frame = bytes([0x00, 0x00]) + struct.pack(">i", -150250) + bytes([0x00, 0x00])
    res_curr = decode_ivts_can(0x521, curr_frame)
    assert abs(res_curr["value_A"] - (-150.250)) < 0.001, f"Current decode failed: {res_curr}"
    
    # Frame 0x522: Pack Voltage U1 = 355.200 V (355200 mV = 0x00056B80)
    u1_frame = bytes([0x00, 0x00, 0x00, 0x05, 0x6B, 0x80, 0x00, 0x00])
    res_u1 = decode_ivts_can(0x522, u1_frame)
    assert abs(res_u1["value_V"] - 355.200) < 0.001, f"U1 Voltage decode failed: {res_u1}"
    
    # Frame 0x523: Inverter Voltage U2 = 340.000 V (340000 mV = 0x00053020)
    u2_frame = bytes([0x00, 0x00, 0x00, 0x05, 0x30, 0x20, 0x00, 0x00])
    res_u2 = decode_ivts_can(0x523, u2_frame)
    assert abs(res_u2["value_V"] - 340.000) < 0.001, f"U2 Voltage decode failed: {res_u2}"

    print(f"  -> Decoded Current: {res_curr['value_A']} A")
    print(f"  -> Decoded Pack Voltage U1: {res_u1['value_V']} V")
    print(f"  -> Decoded Inverter Voltage U2: {res_u2['value_V']} V")
    print("  -> PASSED: IVT-S Big-Endian CAN decoding verified.\n")

# ==============================================================================
# 3. Precharge Safety Logic Test
# ==============================================================================
def is_precharge_ready(u1_volts: float, u2_volts: float) -> bool:
    if u1_volts < 50.0:
        return False
    return (u2_volts / u1_volts) >= 0.95

def test_precharge_logic():
    print("[TEST 3] Precharge Inrush Safety Check Test...")
    # Scenario A: U1 = 355.2V, U2 = 300.0V (Ratio = 84.4% < 95%) -> MUST BE FALSE
    assert not is_precharge_ready(355.2, 300.0), "Precharge incorrectly marked ready at 84%!"
    
    # Scenario B: U1 = 355.2V, U2 = 345.0V (Ratio = 97.1% >= 95%) -> MUST BE TRUE
    assert is_precharge_ready(355.2, 345.0), "Precharge failed to recognize 97.1% ready state!"
    
    print("  -> PASSED: Precharge voltage ratio (U2 / U1 >= 0.95) verified.\n")

# ==============================================================================
# 4. Dynamic CCL / DCL Derating Test (Nissan Leaf 40kWh)
# ==============================================================================
def calculate_dynamic_limits(min_cell_mv: int, max_cell_mv: int, max_temp_c: float):
    max_dcl = 400.0
    max_ccl = 150.0
    
    # Low cell voltage discharge derate (3150mV warn, 2800mV critical)
    if min_cell_mv <= 2800:
        max_dcl = 0.0
    elif min_cell_mv < 3150:
        max_dcl *= (min_cell_mv - 2800) / (3150 - 2800)
        
    # High cell voltage charge derate (4180mV warn, 4250mV critical)
    if max_cell_mv >= 4250:
        max_ccl = 0.0
    elif max_cell_mv > 4180:
        max_ccl *= (4250 - max_cell_mv) / (4250 - 4180)
        
    # High temp derate (55C warn, 60C critical)
    if max_temp_c >= 60.0:
        max_dcl = 0.0
        max_ccl = 0.0
    elif max_temp_c > 55.0:
        t_ratio = (60.0 - max_temp_c) / (60.0 - 55.0)
        max_dcl *= t_ratio
        max_ccl *= t_ratio
        
    return max_dcl, max_ccl

def test_dynamic_limits():
    print("[TEST 4] Dynamic CCL / DCL Limits Calculation Test...")
    # Nominal pack state (3700mV, 25C) -> Max power
    dcl1, ccl1 = calculate_dynamic_limits(3690, 3710, 25.0)
    assert dcl1 == 400.0 and ccl1 == 150.0, f"Nominal limits failed: DCL={dcl1}, CCL={ccl1}"
    
    # Near empty state (Min cell = 2975mV -> halfway between 2800 and 3150) -> DCL = 50% = 200A
    dcl2, ccl2 = calculate_dynamic_limits(2975, 3050, 25.0)
    assert abs(dcl2 - 200.0) < 0.1, f"Discharge derate failed: DCL={dcl2}"
    
    # Near full state (Max cell = 4215mV -> halfway between 4180 and 4250) -> CCL = 50% = 75A
    dcl3, ccl3 = calculate_dynamic_limits(4100, 4215, 25.0)
    assert abs(ccl3 - 75.0) < 0.1, f"Charge derate failed: CCL={ccl3}"
    
    # Over-temp state (57.5C -> halfway between 55C and 60C) -> 50% derate
    dcl4, ccl4 = calculate_dynamic_limits(3700, 3700, 57.5)
    assert abs(dcl4 - 200.0) < 0.1 and abs(ccl4 - 75.0) < 0.1, f"Thermal derate failed: DCL={dcl4}, CCL={ccl4}"

    print("  -> PASSED: Real-time dynamic CCL/DCL derating curves verified.\n")

# ==============================================================================
# 5. ZombieVerter VCU / Orion CAN Framing Test
# ==============================================================================
def frame_zombie_pack_summary(curr_a: float, pack_v: float, soc_pct: float, dcl_a: float, ccl_a: float) -> bytes:
    curr_01a = int(curr_a * 10.0)
    volt_01v = int(pack_v * 10.0)
    soc_byte = int(soc_pct * 2.0)
    dcl_val = int(dcl_a)
    ccl_val = int(ccl_a)
    return struct.pack(">hhBHB", curr_01a, volt_01v, soc_byte, dcl_val, ccl_val)

def test_zombie_can_framing():
    print("[TEST 5] ZombieVerter VCU / Orion CAN Broadcast Framing Test (0x6B0)...")
    # Current = -50.5A, Voltage = 355.2V, SoC = 50.0%, DCL = 300A, CCL = 150A
    frame = frame_zombie_pack_summary(-50.5, 355.2, 50.0, 300.0, 150.0)
    assert len(frame) == 8, f"CAN Frame length incorrect: {len(frame)}"
    
    # Unpack and verify
    c_raw, v_raw, soc_raw, dcl_raw, ccl_raw = struct.unpack(">hhBHB", frame)
    assert c_raw == -505, f"Current mismatch: {c_raw}"
    assert v_raw == 3552, f"Voltage mismatch: {v_raw}"
    assert soc_raw == 100, f"SoC mismatch: {soc_raw}"
    assert dcl_raw == 300, f"DCL mismatch: {dcl_raw}"
    assert ccl_raw == 150, f"CCL mismatch: {ccl_raw}"
    
    print(f"  -> Generated CAN 0x6B0 Payload: {frame.hex().upper()}")
    print("  -> PASSED: ZombieVerter VCU CAN framing validated.\n")

if __name__ == "__main__":
    print("======================================================================")
    print("        OpenEV-BMS AUTOMATED VERIFICATION & TESTBENCH RUNNER          ")
    print("======================================================================\n")
    test_pec15_checksum()
    test_ivts_can_decoding()
    test_precharge_logic()
    test_dynamic_limits()
    test_zombie_can_framing()
    print("======================================================================")
    print("          ALL 5 TEST SUITES PASSED WITH 100% SUCCESS!                 ")
    print("======================================================================")
