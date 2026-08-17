# 05. Engineering Roadmap & Implementation Plan

This roadmap outlines the complete path from initial research to a fully functional, verified automotive-grade BMS.

---

## 1. Development Phases

```
+---------------------------------------------------------------------------------------+
| PHASE 1: RESEARCH & SPECIFICATION (CURRENT)                                           |
|  - Define cell configurations (12s - 192s, 48V to 800V DC)                            |
|  - Select silicon: ADI LTC6813/LTC6820 vs TI BQ79616/BQ79600                          |
|  - Establish GitHub repository & open documentation architecture                      |
+---------------------------------------------------------------------------------------+
                                           |
                                           v
+---------------------------------------------------------------------------------------+
| PHASE 2: HARDWARE SCHEMATIC DESIGN (KiCad 8)                                          |
|  - Master Board: STM32G4 MCU, Dual CAN, Contactor Drivers, IMD, Isolated Current ADC |
|  - Slave AFE Board: 16s/18s LTC6813/BQ79616, Bleed FETs, NTC Matrix, isoSPI/UART      |
|  - High-Voltage Shunt & Hall Interface PCB                                            |
+---------------------------------------------------------------------------------------+
                                           |
                                           v
+---------------------------------------------------------------------------------------+
| PHASE 3: PCB LAYOUT & MANUFACTURING SPECIFICATION                                     |
|  - 4-layer impedance controlled PCB with automotive creepage/clearance slots         |
|  - Optimized for low-cost turnkey assembly at JLCPCB / PCBWay (using standard parts)  |
|  - Automated BOM & CPL (Centroid Pick & Place) generation scripts                     |
+---------------------------------------------------------------------------------------+
                                           |
                                           v
+---------------------------------------------------------------------------------------+
| PHASE 4: EMBEDDED FIRMWARE (C/C++ & FreeRTOS)                                         |
|  - Deterministic 100Hz Task Scheduler                                                 |
|  - Coulomb Counting + OCV-compensated State of Charge (SoC)                          |
|  - Dynamic DCL / CCL Calculation Engine                                               |
|  - OpenInverter / Orion CAN DBC Protocol Translation Layer                            |
+---------------------------------------------------------------------------------------+
                                           |
                                           v
+---------------------------------------------------------------------------------------+
| PHASE 5: VERIFICATION & HARDWARE-IN-THE-LOOP (HIL) TESTING                            |
|  - Resistor ladder 16s/18s cell simulator testing (EETEngineer method)                |
|  - Inrush precharge current test with high-voltage capacitor bank                     |
|  - CAN bus integration with motor inverter (OpenInverter, Leaf, Curtis, Cascadia)    |
+---------------------------------------------------------------------------------------+
```

---

## 2. Component & Bill of Materials (BOM) Strategy

To keep total cost low ($20 - $35 per 16s/18s slave board, $45 - $70 for the master board assembled):

| Function | Primary Component | Alternative | Sourcing |
|---|---|---|---|
| **AFE IC** | ADI **LTC6813HG-1#PBF** (18-channel) | TI **BQ79616-Q1** (16-channel) | LCSC / Mouser / DigiKey |
| **iso Bridge** | ADI **LTC6820IMS#PBF** | TI **BQ79600-Q1** | LCSC / JLCPCB SMT |
| **Master MCU** | ST **STM32G474RET6** (170MHz M4F) | ST **STM32F405RGT6** | LCSC / JLCPCB SMT |
| **CAN Transceiver** | TI **TCAN1042HVDQ1** / **TCAN1044** | Microchip MCP2562FD | JLCPCB Basic / Extended |
| **Isolated Current ADC** | ADI **ADuM7701** / TI **AMC1301** | TI **INA226** (I2C isolated) | LCSC / Mouser |
| **Contactor Drivers** | ST **VND5E025AK-E** (High Side) | TI **TPS274C120** | LCSC / JLCPCB |
| **Pulse Transformer** | Pulse **HM2101NL** / Bourns **SM91501AL** | WE-CST 750315 | LCSC / Mouser |
| **Bleed Resistors** | 1206 / 2512 $39\,\Omega$ 1% Thick Film | MELF 0207 High Power | JLCPCB Basic Parts |

---

## 3. Firmware Architecture & Software Stack

The firmware will be structured in modular layers:

```
[ Application Layer ]   -> State of Charge (EKF), Dynamic CCL/DCL, Safety State Machine
          ^
[ Service Layer ]       -> CAN Communication (Orion DBC / OpenInverter), Diagnostic CLI, NVM Storage
          ^
[ Driver / HAL Layer ]  -> AFE Driver (LTC6813/BQ79616 isoSPI), Shunt ADC (DFSDM), PWM Economizers
          ^
[ Hardware ]            -> STM32G474 Microcontroller
```
