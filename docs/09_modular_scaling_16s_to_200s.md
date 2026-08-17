# 09. Modular Scaling Architecture: 16s to 200s (48V to 1000V DC)

This document specifies the communication protocol, daisy-chain timing budgets, isolation barrier ratings, and ring-redundancy loopback mechanism required to scale seamlessly from 16s up to 200s+ configurations.

---

## 1. System Scaling Matrix

```
  PACK CONFIGURATION     SERIES COUNT (s)    NOMINAL VOLTAGE     SLAVE BOARDS NEEDED (16s/18s)
  ------------------     ----------------    ---------------     -----------------------------
  Low-Voltage ESS / Kart       16s                51.2V / 59.2V             1x 16s Module
  Mid-Voltage Light EV         48s                 153V - 177V              3x 16s Modules
  Target Standard EV (OEM)     96s                 307V - 355V              6x 16s or 6x 18s (108s)
  Performance 400V EV         108s                 345V - 400V              6x 18s Modules
  800V Hypercar / CCS Fast    192s - 216s          614V - 800V              12x 18s Modules
```

---

## 2. Ring-Redundant Daisy Chain Architecture (Dual-Ended Loopback)

In standard linear daisy chains, if a single communication wire breaks or a connector pin backs out, all downstream slave modules are lost, forcing an immediate emergency contactor shutdown.

Our design implements **Dual-Ended Ring Redundancy** using two isolated transceiver channels on the Master Board:

```
                                  MASTER CONTROLLER PCB
                     +---------------------------------------------+
                     |  isoSPI / isoUART Port A (Forward Driver)   |
                     |  isoSPI / isoUART Port B (Reverse Driver)   |
                     +---------------------------------------------+
                            |                               |
                   [Channel A Bus]                 [Channel B Bus]
                            |                               |
                            v                               v
                     +--------------+                +--------------+
                     | SLAVE MOD #1 |                | SLAVE MOD #N |
                     | (16s / 18s)  |                | (16s / 18s)  |
                     +--------------+                +--------------+
                            |                               |
                            v                               v
                     +--------------+                +--------------+
                     | SLAVE MOD #2 | <============> | SLAVE MOD #3 |
                     +--------------+ (Broken Wire!) +--------------+
```

### Fault-Tolerant Operation:
1. **Normal State**: Channel A transmits forward through Slaves $1 \to 2 \to \dots \to N$ and returns back via Channel B.
2. **Severed Wire Condition**: If the harness between Slave 2 and Slave 3 is severed:
   * Channel A communicates with Slaves $1 \to 2$.
   * Channel B automatically activates and communicates in reverse with Slaves $N \to \dots \to 3$.
   * **Result**: All 96s–200s cells continue to be monitored without interruption; a CAN warning is sent to the dashboard.

---

## 3. Communication Timing Budget & Bandwidth

### Packet Size Calculation per 18s Slave Module:
* **Cell Voltages**: 18 channels $\times 16\text{-bit}$ ($2\text{ bytes}$) = $36\text{ bytes}$
* **Temperatures**: 9 auxiliary NTC channels $\times 16\text{-bit}$ = $18\text{ bytes}$
* **Diagnostics & Status**: Die temperature, open-wire flags, balance status = $6\text{ bytes}$
* **CRC-15 / PEC Checksum**: Hardware-calculated cyclic redundancy check = $2\text{ bytes}$
* **Total Payload per Module**: $62\text{ bytes}$ ($496\text{ bits}$)

### Full Pack Refresh Rate at 1 Mbps isoSPI (216s / 12 Modules):
$$\text{Total Bits} = 12 \times 496\text{ bits} = 5,952\text{ bits}$$
$$\text{Transmission Time} = \frac{5,952\text{ bits}}{1,000,000\text{ bps}} \approx 5.95\text{ ms}$$

Adding command framing and inter-frame spacing ($1.5\text{ms}$ overhead):
$$\text{Total Loop Time} \approx 7.45\text{ ms}$$

$$\mathbf{\text{Pack Update Frequency}} = \frac{1}{7.45\text{ ms}} \approx \mathbf{134\text{ Hz}}$$

> **Key Takeaway**: Even at a massive **216s (800V DC)**, the master controller reads every single cell voltage, temperature, and fault status **134 times per second**, exceeding automotive ISO 26262 response time requirements by an order of magnitude.

---

## 4. Galvanic Isolation & Creepage Compliance (IEC 60664-1)

For an $800\text{V} - 1000\text{V}$ system operating under automotive conditions (Pollution Degree 2, Material Group IIIa):

| Barrier Interface | Component / Design Feature | Isolation Rating | Creepage Distance |
|---|---|---|---|
| **Daisy Chain Inter-Module** | Pulse **HM2101NL** / Bourns **SM91501AL** | **$4300\text{V}_{\text{DC}}$ (1 min)** | $> 8.0\text{ mm}$ (with PCB air slot) |
| **Current Shunt ADC** | ADI **ADuM7701** / TI **AMC1301** | **$5000\text{V}_{\text{RMS}}$ (1 min)** | $> 8.3\text{ mm}$ wide-body SOIC |
| **Active IMD Resistor Bridge** | Vishay **VO14642** Solid-State Relays | **$5300\text{V}_{\text{RMS}}$** | $> 8.0\text{ mm}$ spacing |
| **Auxiliary 12V Power Supply** | Isolated Flyback (RECOM / Murata Automotive) | **$3000\text{V}_{\text{DC}}$** | $> 6.5\text{ mm}$ |
