# 03. Hardware System Block Diagram & Subsystems

This document specifies the hardware architecture, galvanic isolation boundaries, and electrical interface requirements for our automotive-grade EV BMS.

---

## 1. Top-Level Hardware Architecture

```
+========================================================================================================+
|                                        HIGH VOLTAGE DOMAIN (HV)                                        |
|                                                                                                        |
|   +---------------------------------------+         +---------------------------------------+          |
|   |         SLAVE MODULE #1 (AFE)         |         |         SLAVE MODULE #N (AFE)         |          |
|   |  - 16s / 18s Cell Taps (0-75V)        |         |  - 16s / 18s Cell Taps (0-75V)        |          |
|   |  - LTC6813 / BQ79616 AFE IC           |         |  - LTC6813 / BQ79616 AFE IC           |          |
|   |  - 8-9x NTC Thermistors               |         |  - 8-9x NTC Thermistors               |          |
|   |  - Passive Bleed Balancing FETs/Res   |         |  - Passive Bleed Balancing FETs/Res   |          |
|   +---------------------------------------+         +---------------------------------------+          |
|                       |                                                 |                              |
|                       +============ [ ISOLATED DAISY CHAIN ] ===========+                              |
|                                     (isoSPI / isoUART - 2 Wires)                                       |
|                                                 |                                                      |
+=================================================|======================================================+
                                                  | Galvanic Isolation Barrier (Pulse Transformer / Caps)
                                                  v
+========================================================================================================+
|                                    LOW VOLTAGE DOMAIN (12V Chassis GND)                                |
|                                                                                                        |
|   +------------------------------------------------------------------------------------------------+   |
|   |                                     MASTER CONTROLLER PCB                                      |   |
|   |                                                                                                |   |
|   |   +------------------------------------+      +--------------------------------------------+   |   |
|   |   |        MCU SUBSYSTEM (STM32)       |      |             CURRENT SHUNT & ADC            |   |   |
|   |   |  - STM32G474 (Cortex-M4F @ 170MHz) |<---->|  - Isolated Delta-Sigma Modulator (ADuM7701|   |   |
|   |   |  - Hardware Floating Point Unit    |      |    or TI AMC1301 / INA226)                 |   |   |
|   |   |  - Coulomb Counting, EKF, DCL/CCL  |      |  - 50uOhm - 100uOhm Manganin Shunt         |   |   |
|   |   +------------------------------------+      +--------------------------------------------+   |   |
|   |             |                     |                                 |                          |   |
|   |             v                     v                                 v                          |   |
|   |   +-------------------+ +--------------------+            +--------------------+               |   |
|   |   | DUAL CAN BUS      | | CONTACTOR DRIVERS  |            | INSULATION (IMD)   |               |   |
|   |   | - CAN1: Inverter  | | - Precharge Relay  |            | - Active AC/DC     |               |   |
|   |   |   & Vehicle VCU   | | - Main (+) Contactor|           |   Ground Fault     |               |   |
|   |   | - CAN2: Charger   | | - Main (-) Contactor|           |   Resistance Test  |               |   |
|   |   | - Isolated TCAN   | | - PWM Economizer   |            | - Optical Switch   |               |   |
|   |   |   transceivers    | | - Current Monitor  |            |   Resistor Bridge  |               |   |
|   |   +-------------------+ +--------------------+            +--------------------+               |   |
|   +------------------------------------------------------------------------------------------------+   |
|                                                 |                                                      |
|                                                 v                                                      |
|                                  [ 12V Automotive Power / KL30 / KL15 ]                                |
+========================================================================================================+
```

---

## 2. Subsystem Breakdown

### Subsystem 1: AFE Slave Boards (Cell Tap & Balancing)
* **Function**: Placed directly on or near battery modules (e.g. 12s, 14s, 16s, or 18s segments).
* **Components**:
  * **AFE IC**: ADI `LTC6813-1` (18-channel) or TI `BQ79616-Q1` (16-channel).
  * **Input RC Filters**: Low-pass differential filter on every cell tap ($100\Omega + 100\text{nF}$) for anti-aliasing and surge absorption.
  * **Balancing Shunts**: $33\Omega - 47\Omega$ 1206/2512 power resistors with N-channel MOSFETs providing $80\text{mA} - 150\text{mA}$ passive bleed per channel.
  * **Thermistor Channels**: 8 to 9 channels connected to 10k NTC beads glued between cells.
  * **Isolated Daisy Chain Interface**: Pulse transformers (e.g., Pulse Electronics HM2101NL) rated for $>1500\text{V}_{\text{RMS}}$ isolation.

### Subsystem 2: Master Controller & Processing Engine
* **Microcontroller**: `STM32G474RET6` (Arm Cortex-M4F with DSP/FPU, 512KB Flash, dual CAN-FD controllers, high-resolution timers) or `ESP32-S3` (for built-in WiFi/BLE telemetry companion).
* **Isolated isoSPI / isoUART Transceiver Bridge**:
  * For LTC6813: Analog Devices `LTC6820` (SPI to isoSPI converter).
  * For BQ79616: TI `BQ79600-Q1` (SPI to isolated UART bridge).
* **Power Supply (Wide Input)**: Automotive buck regulator accepting $8\text{V}$ to $36\text{V}$ (load dump tolerant up to $45\text{V}$, reverse polarity protected via ideal diode controller).

### Subsystem 3: High-Precision Isolated Current Shunt
* **Shunt**: Isabellenhütte or Bourns $50\,\mu\Omega$ precision 4-terminal Kelvin shunt (handles up to $\pm 800\text{A}$ continuous, $1200\text{A}$ peak).
* **Isolation ADC**: `ADuM7701` or `AMC1301` isolated delta-sigma modulator with $>5\text{kV}$ galvanic barrier, feeding the STM32 hardware DFSDM (Digital Filter for Sigma-Delta Modulators).
* **Measurement Performance**: $<0.1\text{A}$ resolution, $<0.5\%$ full-scale error, immune to motor inverter PWM switching noise.

### Subsystem 4: Contactor & Precharge Drivers
* **Drivers**: Automotive-grade smart high-side / low-side drivers (e.g., TI `TPS274C` or ST `VND5E` series) with integrated:
  * Overcurrent / Short-circuit shutdown.
  * Open-load detection.
  * **PWM Economizer**: Automatically switches coil voltage from $100\%$ ($12\text{V}$ for $100\text{ms}$ pull-in) down to $30\% - 40\%$ duty cycle ($4\text{V}-5\text{V}$ hold-in) to eliminate coil heat and save battery power.

### Subsystem 5: Active Insulation Monitoring Device (IMD)
* In an ungrounded high-voltage system, if the positive or negative traction bus contacts the metal vehicle chassis, a shock hazard exists.
* **Working Principle**: The IMD connects a high-impedance resistor divider ($>1\text{M}\Omega$) from HV+ and HV- to chassis GND via optical solid-state relays (e.g. Vishay VO14642), measuring the differential voltage drop to calculate chassis isolation resistance in $\text{k}\Omega/\text{V}$.
* Complies with **UN ECE R100** threshold ($>500\,\Omega/\text{V}$ requirement for DC systems).
