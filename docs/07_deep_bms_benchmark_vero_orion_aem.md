# 07. Deep Benchmark: Orion BMS 2 vs AEM EV BMS-18 vs Vero BMS vs OpenEV-BMS

This document provides a rigorous, component-level engineering and mathematical comparison of the leading EV BMS solutions on the market against our proposed open-source architecture.

---

## 1. High-Level Engineering Comparison Matrix

| Technical Metric | **Orion BMS 2** (Ewert Energy) | **AEM EV BMS-18** (AEM Performance) | **Vero BMS V2** (Vero Electric) | **OpenEV-BMS (Our Proposed System)** |
|---|---|---|---|---|
| **Architecture Topology** | Centralized (up to 108s) or Master-Satellite | Pure Modular Master + Satellite (18s per module) | Modular Master + Tesla OEM BMB Harness | **Modular Master + Universal Slaves (16s/18s) with Ring Redundancy** |
| **Series Scaling Range** | 24s to 108s (Centralized) / 250s (Modular) | 18s to 240s+ (up to 1200V DC) | 6s to 96s (Tesla Model S/X modules) | **16s to 216s+ (48V to 1000V DC)** |
| **Silicon / AFE Chipset** | Proprietary Centralized Multiplexed AFE Array | Analog Devices **LTC6818 / LTC6813** | Reads Tesla OEM BMBs (TI BQ76PL536 / LTC6804) | **ADI LTC6813-1** or **TI BQ79616-Q1** |
| **Balancing Architecture** | **Passive Bleed Only** ($150\text{mA} - 200\text{mA}$) | **Passive Bleed Only** ($100\text{mA} - 150\text{mA}$) | **Passive Bleed Only** (Tesla OEM ~100mA) | **Hybrid: 250mA Passive Bleed + Autonomous 2.0A–4.0A Active Balancer Engine** |
| **Balancing Energy Fate** | Dissipated 100% as heat into PCB | Dissipated 100% as heat into PCB | Dissipated 100% as heat into OEM BMB | **Transferred back into pack/module bus at $\ge 88\%$ efficiency** |
| **Sampling Speed (Pack)**| $\approx 10\text{ms} - 50\text{ms}$ | $< 300\,\mu\text{s}$ per 18s module | Variable (UART poll across Tesla modules) | **$< 290\,\mu\text{s}$ simultaneously across all 96s–200s** |
| **Temperature Inputs** | 12 to 80+ NTCs (via harness) | 3 NTCs per 18s module | Tesla Module OEM thermistors | **8 to 9 NTCs per 16s/18s module (High-density mapping)** |
| **Current Measurement** | Shunt or Hall Sensor ($0.5\%$ error) | Integrated with AEM VCU | 400A Hall Sensor | **$50\,\mu\Omega$ Manganin Kelvin Shunt + Isolated $\Delta\Sigma$ ADC (ADuM7701/AMC1301)** |
| **Insulation Monitor (IMD)**| Onboard active isolation measurement | External or VCU-managed | None (External required) | **Onboard Active Ground Fault Monitor ($>500\,\Omega/\text{V}$ ECE R100 compliant)** |
| **Contactor Drivers** | High/Low side with PWM economizer | Handled by external AEM VCU | 12V contactor trigger outputs | **Integrated Dual High-Side Drivers (TPS274C) with programmable PWM hold** |
| **Vehicle CAN Protocols** | Dual CAN 2.0B (Configurable DBC) | Proprietary AEM VCU CAN | CAN bus (Tesla / custom) | **Dual CAN 2.0B / CAN-FD (OpenInverter, Orion DBC emulation, J1939, VCU)** |
| **Firmware Openness** | 100% Closed / Proprietary binary | 100% Closed (requires AEMCal software)| Closed Source | **100% Open Source (GPL v3 / MIT) C/C++ FreeRTOS** |
| **Approximate Cost** | **$1,500 – $2,800 USD** | **$1,200 – $2,200 USD** (+ $1,500 VCU) | **$800 – $1,400 USD** | **~$180 – $320 USD total (Complete 96s BOM)** |

---

## 2. Deep Teardowns of Benchmarks

### 1. Orion BMS 2 (The Industry Standard)
* **Design Philosophy**: Built for turnkey reliability in conversions and industrial fleets.
* **Limitations**:
  1. *Harness Complexity*: In a 96s or 108s centralized setup, **97 to 109 heavy-gauge high-voltage wires** must travel through the vehicle to a central enclosure. If a harness shorts or is pinched, it poses a severe fire hazard.
  2. *Passive Balancing Heat*: Bleeding 96 cells at $150\text{mA}$ generates $\approx 50\text{W}-60\text{W}$ of heat inside the sealed BMS casing. This requires aggressive thermal throttling.
  3. *No Native Active Balancing*: Cannot dynamically equalize degraded or capacity-mismatched cells during drive cycles.

### 2. AEM EV BMS-18 (The Motorsport Modular Standard)
* **Design Philosophy**: Distributed 18-channel satellite modules communicating with an AEM Vehicle Control Unit (VCU200/VCU300).
* **Silicon**: Built around Linear Tech / Analog Devices LTC681x family using **isoSPI** pulse transformers.
* **Limitations**:
  1. *Tightly Coupled to AEM Ecosystem*: The BMS-18 satellites have no standalone intelligence—they are dumb AFEs. If you do not buy a $1,500-$3,000 AEM VCU, the system cannot function.
  2. *Minimal Temperature Sensing*: Only 3 thermistor inputs per 18 cells (1 per 6 cells), which falls short of OEM standards (1 probe per 2 cells for high-performance pouch/prismatic cells).

### 3. Vero BMS V2 (The Tesla Recycler Solution)
* **Design Philosophy**: Reverse-engineered master controller that taps directly into OEM Tesla Model S/X Battery Monitoring Boards (BMBs).
* **Limitations**:
  1. *Tied to Tesla Modules*: Only works with 6s Tesla modules. Cannot be used with modern CATL/CALB prismatic cells, pouch cells, BMW i3 modules, or custom 18650/21700/46800 packs.
  2. *Passive-Only OEM Balancing*: Limited to Tesla's fixed ~100mA bleed rate.

---

## 3. The OpenEV-BMS Synthesis: Why Our Architecture Wins

By learning from these three industry leaders, OpenEV-BMS combines:
1. **The Modular Scalability of AEM BMS-18** (isoSPI/isoUART daisy-chain scaling from 16s to 216s+ up to 1000V).
2. **The Standalone Compute & Safety Intelligence of Orion BMS 2** (Onboard SoC/SoH Kalman filtering, contactor economizers, weld detection, IMD insulation monitoring, and native CAN broadcast).
3. **High-Performance Active Balancing (2.0A–4.0A)**: Solves the thermal and speed bottleneck of passive bleeding, allowing salvaged or slightly mismatched cells to deliver 100% of usable pack capacity.
4. **Ring-Redundant Daisy Chain**: A dual-ended loopback bus ensures that even if one daisy-chain cable is severed, communication immediately reverses and zero cell data is lost.
