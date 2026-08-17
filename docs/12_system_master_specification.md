# 12. Complete System Master Specification & Architecture Blueprint

This document represents the definitive engineering specification for the **OpenEV-BMS**, established through the comprehensive architectural interview and tailored for high-performance EV builds (featuring Nissan Leaf 40kWh modules, ZombieVerter VCU, Isabellenhütte IVT-S 1000A, and CCS DC Fast Charging).

---

## 1. High-Level System Parameters

| Parameter | Specification |
|---|---|
| **Target Battery Pack** | **Nissan Leaf 40kWh Modules (24 modules in 96s2p configuration)** |
| **Chemistry Engine** | Universal Configurable Profile Engine (**NMC / LFP / NCA / LTO**) |
| **Nominal Voltage** | **$355.2\text{ V}$** ($96 \times 3.7\text{V}$) |
| **Max / Min Pack Voltage** | **$403.2\text{ V}$** ($96 \times 4.2\text{V}$) / **$288.0\text{ V}$** ($96 \times 3.0\text{V}$) |
| **Physical Distribution** | **Multi-Box Modular (Front Engine Bay + Rear Trunk / Fuel Tank Area)** |
| **Current Sensor** | **Isabellenhütte IVT-S-1K (1000A CAN Bus Shunt)** |
| **Vehicle VCU** | **ZombieVerter VCU** (Orion BMS CAN DBC emulation on CAN1) |
| **Charging Standards** | **AC Onboard Charger (OBC)** + **Direct CCS DC Fast Charging (ISO 15118 / DIN 70121 via Foccci/Clara)** |
| **Configuration & Telemetry**| **Built-in WiFi / BLE Web Dashboard** (ESP32-S3 companion, phone/laptop browser UI, OTA updates) |
| **Thermal Control** | **Multi-Zone Independent 12V 4-Wire PWM Fan Drivers** (Front Box, Rear Box, Charger) |
| **Safety Interlocks** | **Continuous HVIL Loop, Active IMD Insulation Monitor ($>500\Omega/\text{V}$), Contactor Weld Check** |

---

## 2. Hardware Architecture & Block Diagram

```
+===================================================================================================================+
|                                              BATTERY BOXES & SLAVE NETWORK                                        |
|                                                                                                                   |
|   [ FRONT BATTERY BOX (e.g. 32s / 8 Modules) ]             [ REAR BATTERY BOX (e.g. 64s / 16 Modules) ]          |
|   +------------------------------------------+             +------------------------------------------+           |
|   | 2x Slave AFE Modules (16s LTC6813 each)  |             | 4x Slave AFE Modules (16s LTC6813 each)  |           |
|   | - 18x NTC Temperature Sensors            |             | - 36x NTC Temperature Sensors            |           |
|   | - 120mA Passive Bleed Resistors          |             | - 120mA Passive Bleed Resistors          |           |
|   | - Front Box 12V PWM Cooling Fan          |             | - Rear Box 12V PWM Cooling Fan           |           |
|   +------------------------------------------+             +------------------------------------------+           |
|                        |                                                                 |                        |
|                        +==================== [ ISOSPI DAISY CHAIN ] =====================+                        |
|                                         (Single 2-Wire Shielded Twisted Pair)                                     |
|                                                          |                                                        |
+==========================================================|========================================================+
                                                           | Galvanic Isolation (Pulse Transformers HM2101NL)
                                                           v
+===================================================================================================================+
|                                                MASTER CONTROLLER BOARD                                            |
|                                                                                                                   |
|   +---------------------------------------------+        +----------------------------------------------------+   |
|   |         MAIN PROCESSING CORE (STM32G474)    |        |        TELEMETRY & WEB DASHBOARD (ESP32-S3)        |   |
|   |  - 170MHz Arm Cortex-M4F + Hardware DFSDM   |<======>|  - WiFi Access Point & Web Server (HTML5/JS GUI)   |   |
|   |  - SoC / SoH Coulomb Counting & Kalman Filter| (UART) |  - Real-time Cell Bar Graphs & Parameter Tuning    |   |
|   |  - Dynamic CCL / DCL Engine & State Machine |        |  - Over-The-Air (OTA) Firmware Flashing            |   |
|   +---------------------------------------------+        +----------------------------------------------------+   |
|         |                     |                                      |                         |                  |
|         v                     v                                      v                         v                  |
|   +--------------+    +----------------------------+       +-------------------+     +--------------------+       |
|   | TRIPLE CAN   |    | SMART CONTACTOR DRIVERS    |       | MULTI-ZONE FANS   |     | ACTIVE IMD & HVIL  |       |
|   | - CAN1: VCU  |    | - Main Positive (PWM Hold) |       | - Front Box Fan   |     | - Ground Fault IMD |       |
|   | - CAN2: IVT-S|    | - Main Negative            |       | - Rear Box Fan    |     | - Contactor Weld   |       |
|   | - CAN3: CCS  |    | - Precharge Relay          |       | - Charger Fan     |     | - Hardware HVIL    |       |
|   +--------------+    +----------------------------+       +-------------------+     +--------------------+       |
+===================================================================================================================+
```

---

## 3. Detailed Subsystem Specifications

### Subsystem A: 18-Channel AFE Slave Modules (`Slave_AFE_18s`)
* **Quantity**: 6 Slave boards for the 96s pack (16s configured on each 18s board).
* **Core Silicon**: Analog Devices **LTC6813-1** (AEC-Q100, ASIL-D ready).
* **Measurement Performance**: $\pm 1.2\text{ mV}$ typical accuracy, $<290\,\mu\text{s}$ simultaneous conversion.
* **Passive Balancing**: External AO3400A N-channel MOSFETs switching $33\,\Omega$ 2512 1W power resistors ($121\text{mA}$ at $4.0\text{V}$), with software temperature throttling.
* **Thermal Monitoring**: 9x 10k NTC thermistors per board (Total: 54 temperature probes across the 96s pack for complete thermal visibility).

### Subsystem B: Master Controller Core (`Master_Controller`)
* **Core MCU**: **STM32G474RET6** (170MHz, 512KB Flash, 128KB RAM).
* **Wireless Companion**: **ESP32-S3-WROOM-1** running an embedded async web server hosting a zero-install, responsive HTML5 dashboard accessible on mobile and desktop.
* **Ring-Redundant isoSPI Transceivers**: Dual **LTC6820** ICs (Port A forward, Port B reverse loopback).
* **Triple Isolated CAN Bus**:
  1. `CAN 1 (500 kbps)`: ZombieVerter VCU / Inverter (Broadcasting Orion-compatible `0x3B`, `0x3C`, `0x6B0`).
  2. `CAN 2 (500 kbps)`: Dedicated Isabellenhütte IVT-S-1K Current Sensor (`0x521` to `0x528`).
  3. `CAN 3 (500 kbps / 250 kbps)`: CCS DC Fast Charging Controller (Foccci / Clara ISO 15118 PLC) & AC OBC.
* **Smart Contactor Drivers**: TI `TPS274C120` high-side smart switches with 25kHz PWM economizer hold for external TE EV200 / Gigavac contactors.
* **Thermal Management**: 3x independent 12V 4-wire PWM fan channels (Front Box, Rear Box, Charger).
* **Active IMD**: High-voltage optical solid-state bridge (Vishay `VO14642`) verifying $>500\,\Omega/\text{V}$ chassis isolation.

---

## 4. CCS DC Fast Charging & Safety State Machine

```
              [ CCS Plug Connected (CP / PP Detected) ]
                                 |
                                 v
              [ Pre-Charge Insulation Test (Active IMD) ]
              (Verify Chassis Isolation > 500 ohms/V)
                                 |
                                 v
              [ Fast Charge CAN Handshake (ISO 15118 / Clara) ]
              (Transmit: Target Voltage = 403.2V, Max Current = 150A)
                                 |
                                 v
              [ Inverter / DC Bus Precharge Sequence ]
              (Verify IVT-S U2 / U1 >= 0.95)
                                 |
                                 v
              [ Engage Fast Charge Contactors (DC+ / DC-) ]
                                 |
                                 v
              [ Active Multi-Zone Thermal Loop Active ]
              (PWM Fans at 100% in Front & Rear Boxes)
                                 |
                                 v
     +---------------------------+---------------------------+
     |                                                       |
[ Normal Fast Charge ]                               [ Safety Anomaly Detected ]
- IVT-S monitors Coulombs                             - Any Cell Temp > 50C
- Dynamic CCL throttles as                             - Any Cell Voltage > 4.22V
  cells reach 4.15V                                    - Isolation Drops < 500 ohms/V
- Taper to 0A at 4.20V                                - Loss of CAN / HVIL Break
                                                             |
                                                             v
                                                      [ EMERGENCY SHUTDOWN ]
                                                      - Demand 0A from CCS Station
                                                      - Open DC Contactors in < 50ms
```
