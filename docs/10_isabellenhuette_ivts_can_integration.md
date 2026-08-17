# 10. Isabellenhütte IVT-S CAN Current Sensor & ZombieVerter VCU Integration

The **Isabellenhütte IVT-S-1K** (1000A version) is the automotive gold standard for high-voltage current measurement, Coulomb counting, and multi-channel voltage sensing in electric vehicles. It communicates digitally over CAN bus (typically 500kbps) and is natively supported by the **ZombieVerter VCU / OpenInverter** ecosystem.

---

## 1. System Interconnection Architecture

```
                                  HIGH VOLTAGE TRACTION PACK
                      ===================================================
                                               |
                                     [ IVT-S-1K Shunt Bar ]
                                     (1000A Continuous / 3000A Peak)
                                               |
                          +--------------------+--------------------+
                          | (U1 Sense: Pack +) | (U2 Sense: Inv +)  |
                          v                    v                    v
                   +-------------------------------------------------------+
                   |           ISABELLENHÜTTE IVT-S-1K SENSOR              |
                   |  - Current I: \pm 1000A (0.1% accuracy)               |
                   |  - Voltage U1: 0 - 1000V DC (Pack Voltage)            |
                   |  - Voltage U2: 0 - 1000V DC (Inverter Bus / Precharge)|
                   |  - Voltage U3: 0 - 1000V DC (Aux / Ground Sense)      |
                   |  - Coulomb Counter: Ampere-seconds (As) / Ah          |
                   |  - Energy Counter: Watt-hours (Wh)                    |
                   +-------------------------------------------------------+
                                               |
                                               | Isolated CAN Bus (500 kbps)
                                               v
     +===================================================================================+
     |                               MASTER CONTROLLER (BMS)                             |
     |                                                                                   |
     |   +------------------------------------+      +-------------------------------+   |
     |   | CAN Port 2 (Sensor Bus)            |      | CAN Port 1 (Drivetrain Bus)   |   |
     |   | Reads: 0x521 (Current I)           |      | Broadcasts to ZombieVerter:   |   |
     |   | Reads: 0x522 (Pack Voltage U1)     |      | - 0x3B / 0x3C (Orion Emulation|   |
     |   | Reads: 0x523 (Precharge Bus U2)    |      | - DCL / CCL Limits (Amps)     |   |
     |   | Reads: 0x527 (Coulomb As / Ah)     |      | - Min/Max Cell Voltage & Temp |   |
     |   +------------------------------------+      +-------------------------------+   |
     +===================================================================================+
                                                               |
                                                               v
                                                   [ ZOMBIEVERTER VCU ]
                                                   (Inverter & Drive Logic)
```

---

## 2. Isabellenhütte IVT-S Standard CAN Protocol Specification

* **Baud Rate**: $500\text{ kbps}$ (Standard 11-bit CAN 2.0B)
* **Byte Order**: Big-Endian (Motorola format)
* **Message Cycle Time**: $20\text{ms}$ (Current), $100\text{ms}$ (Voltages, Temperature, Coulomb Counter)

| CAN ID | Signal Name | Data Length | Byte Index | Format | Scaling / Unit | Description |
|---|---|---|---|---|---|---|
| **`0x521`** | **Current ($I$)** | 8 Bytes | Byte 2–5 | 32-bit Signed Int | $1\text{ LSB} = 1\text{ mA}$ | Instantaneous traction current ($+ = \text{Charge}, - = \text{Discharge}$) |
| **`0x522`** | **Voltage $U_1$** | 8 Bytes | Byte 2–5 | 32-bit Signed Int | $1\text{ LSB} = 1\text{ mV}$ | Pack Total Voltage across terminals |
| **`0x523`** | **Voltage $U_2$** | 8 Bytes | Byte 2–5 | 32-bit Signed Int | $1\text{ LSB} = 1\text{ mV}$ | Inverter DC-Link Bus Voltage (Used for Precharge Check) |
| **`0x524`** | **Voltage $U_3$** | 8 Bytes | Byte 2–5 | 32-bit Signed Int | $1\text{ LSB} = 1\text{ mV}$ | Auxiliary Voltage / Chassis reference |
| **`0x525`** | **Temperature ($T$)** | 8 Bytes | Byte 2–5 | 32-bit Signed Int | $1\text{ LSB} = 0.1^\circ\text{C}$ | Internal Shunt Temperature |
| **`0x526`** | **Energy ($W$)** | 8 Bytes | Byte 2–5 | 32-bit Signed Int | $1\text{ LSB} = 1\text{ Wh}$ | Cumulative energy consumed / regenerated |
| **`0x527`** | **Charge ($Q_{\text{As}}$)**| 8 Bytes | Byte 2–5 | 32-bit Signed Int | $1\text{ LSB} = 1\text{ As}$ | Coulomb counter ($\text{Ah} = Q_{\text{As}} / 3600$) |
| **`0x528`** | **Status / Flags** | 8 Bytes | Byte 0–7 | Bitmask | Flags | Error states, sensor health, measurement validity |

---

## 3. Precharge Automation via IVT-S $U_1$ and $U_2$

With the IVT-S sensing both $U_1$ (Pack Voltage) and $U_2$ (Inverter Bus Voltage), the Master BMS automates precharge sequencing with $100\%$ precision:

$$\Delta V = |U_1 - U_2|$$

1. **Start Precharge**: Close Main Negative Contactor and Precharge Relay.
2. **Dynamic Inverter Verification**:
   $$\text{Precharge Completion Condition: } \frac{U_2}{U_1} \ge 0.95 \quad (\Delta V \le 5\% \, U_1)$$
3. **Engage Main Positive**: When the condition is met, close Main Positive Contactor with zero contact arcing.
4. **Open Precharge**: Open Precharge Relay within $50\text{ms}$.
5. **Timeout Safety**: If $U_2 < 0.95 \, U_1$ after $1500\text{ms}$ (e.g. short in inverter DC bus), instantly abort and open all contactors.

---

## 4. ZombieVerter VCU & OpenInverter CAN Broadcast (Orion Emulation)

The Master Controller converts the IVT-S high-precision data + LTC6813 cell data into standard **Orion BMS CAN frames** on CAN Port 1, enabling plug-and-play operation with ZombieVerter VCU:

### Message 1: Pack Summary & Current Limits (`0x6B0` / `0x3B`)
* **Byte 0–1**: Pack Current ($0.1\text{A}$ / bit, signed) $\leftarrow$ derived directly from IVT-S `0x521`.
* **Byte 2–3**: Pack Total Voltage ($0.1\text{V}$ / bit) $\leftarrow$ derived from IVT-S `0x522`.
* **Byte 4**: State of Charge ($\text{SoC} \times 2$, $0.5\%$ / bit).
* **Byte 5–6**: Discharge Current Limit (DCL in Amps).
* **Byte 7**: Charge Current Limit (CCL in Amps).

### Message 2: Cell Extrema & Temperatures (`0x3C`)
* **Byte 0–1**: High Cell Voltage ($1\text{mV}$ / bit) $\leftarrow$ from LTC6813 slave network.
* **Byte 2–3**: Low Cell Voltage ($1\text{mV}$ / bit).
* **Byte 4**: High Cell Temperature ($1^\circ\text{C}$ offset by $40^\circ\text{C}$).
* **Byte 5**: Low Cell Temperature.
* **Byte 6–7**: Safety Fault Bitmask (COV, CUV, COT, CUT, Isolation Fault, Interlock Broken).
