# OpenEV-BMS: Automotive-Grade Open Source Battery Management System

An open-source, automotive-grade Battery Management System (BMS) designed for electric vehicle (EV) conversions, racing (Formula SAE / Student), and custom high-voltage energy storage.

Inspired by industry benchmarks like the **Orion BMS 2** and modern EV architectures (Tesla / VW MEB), this project aims to provide an uncompromised, safety-certified, modular Master-Slave and Centralized hardware & firmware design.

---

## ⚡ Key Highlights & Target Architecture

| Feature | Target Specification |
|---|---|
| **Topology** | Modular Master-Slave (IsoSPI / Isolated Daisy Chain) + Centralized Options |
| **Voltage Range** | 24V up to 800V DC (12s to 192s+) |
| **AFE Silicon** | Analog Devices LTC6813 / ADBMS6830 or Texas Instruments BQ79616 |
| **Cell Accuracy** | $\pm 1.0\text{ mV}$ typical with 16-bit to 18-bit Delta-Sigma ADCs |
| **Cell Balancing** | High-efficiency Passive Bleeding ($100\text{mA} - 250\text{mA}$ per cell with thermal foldback) |
| **Current Sensing** | Galvanically isolated Shunt ($0.5\%$ accuracy) with Coulomb Counting & Kalman Filter |
| **HV Isolation (IMD)**| Onboard active Insulation Resistance Monitoring (Chassis-to-HV leakage detection) |
| **Contactor Drivers** | Integrated High/Low Side Drivers with PWM Coil Economizers (Precharge, Main+, Main-) |
| **Communications** | Dual CAN 2.0B / CAN-FD (Vehicle VCU/Inverter CAN + Isolated Charger CAN) + USB/BLE |
| **Safety Compliance** | Designed according to ISO 26262 (ASIL-C/D ready), UN ECE R100, and UL 2580 |

---

## 📁 Repository Structure

```
diy-ev-bms/
├── docs/
│   ├── 01_bms_fundamentals.md          # Physics of Li-ion, BMS state machines, and core functions
│   ├── 02_architecture_comparison.md   # Orion vs Flying ADC vs Dedicated AFE vs Commercial Slaves
│   ├── 03_system_block_diagram.md      # Hardware subsystems, signal flow, and isolation barriers
│   ├── 04_safety_and_standards.md      # ISO 26262, UN ECE R100, contactor weld check, HVIL, IMD
│   └── 05_roadmap_and_design_spec.md   # Hardware, schematic, PCB layout rules, and firmware roadmap
├── hardware/                           # KiCad schematics, PCB layouts, and BOMs (upcoming)
│   ├── master_controller/
│   ├── slave_afe_16s_18s/
│   └── current_shunt_imd/
├── firmware/                           # Embedded C/C++ firmware, RTOS, state machines (upcoming)
└── tools/                              # PC GUI, CAN logs, battery simulation & calibration tools
```

---

## 🚀 Getting Started & Contributing
This repository is open for collaborative engineering. Review the architectural documentation in `/docs` to understand the physics, safety state machines, and component selection criteria.
