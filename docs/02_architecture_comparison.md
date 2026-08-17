# 02. Comprehensive Architecture Comparison

To design an automotive-grade BMS that is both cost-effective and reliable, we must analyze the existing paradigms and why certain approaches succeed or fail in the real world.

---

## 1. Architecture Paradigm Comparison

```
+-----------------------------------------------------------------------------------------------+
| ARCHITECTURE             | PROS                                    | CONS                     |
+--------------------------+-----------------------------------------+--------------------------+
| 1. Centralized           | - Single PCB to manufacture             | - High-voltage harness   |
|    (e.g., Orion BMS 2)   | - Lowest overall cost for compact packs |   spaghetti (dozens of   |
|                          | - Fast unified MCU communication        |   wires routed to 1 box) |
|                          |                                         | - Limited modularity     |
+--------------------------+-----------------------------------------+--------------------------+
| 2. Modular Master-Slave  | - Clean physical wiring (daisy chain)   | - Multiple PCBs to make  |
|    (e.g., Tesla, FoxBMS, | - Scales from 24V to 1000V              | - Requires isolated bus  |
|     Modern OEM standard) | - True module-level thermal/voltage isolation| (isoSPI or isoUART) |
+--------------------------+-----------------------------------------+--------------------------+
| 3. Flying ADC / Cap MUX  | - Low silicon cost (single ADC)         | - Slow sequential read   |
|    (Johannes Hübner)     | - Can implement active balance tricks   | - Solid-state relay wear |
|                          |                                         | - Bad in dynamic pulses  |
+--------------------------+-----------------------------------------+--------------------------+
| 4. Commercial Chinese    | - Ready to buy out of the box           | - Closed-source firmware |
|    Master-Slave ($500)   | - Pre-assembled with housing            | - Hard to customize CAN  |
|                          |                                         | - Questionable warranty  |
+--------------------------+-----------------------------------------+--------------------------+
```

---

## 2. Deep-Dive: Orion BMS (The Industry Benchmark)

The **Orion BMS 2** (by Ewert Energy Systems) is the undisputed gold standard in DIY EV conversions, custom racing, and industrial energy storage.

### How Orion BMS Works Internally:
1. **Centralized AFE Array**: The standard Orion 2 uses multiple multi-channel Analog Front End (AFE) chips mounted directly on one large master board.
2. **Heavy-Duty Amphenol Connectors**: Up to 108 cell tap wires (plus thermistor wires) enter via heavy-duty automotive locking harnesses.
3. **Dedicated Microcontroller Subsystem**:
   * Measures all cell voltages, currents, and pack isolation.
   * Runs advanced SoC / SoH Kalman filters.
   * Calculates continuous Charge Current Limit (CCL) and Discharge Current Limit (DCL).
4. **Contactor Control**: Low-side and high-side drivers with PWM hold (economizer) to keep contactors cool.
5. **Dual CAN 2.0B**: CAN1 communicates with the vehicle drivetrain (inverter, VCU), while CAN2 handles the onboard charger (Elcon, TC Charger, Lear, etc.).

**Why people love it:** Extremely robust, highly configurable PC utility, comprehensive safety interlocks.  
**Why people seek alternatives:** Expensive ($1,200 – $2,500 USD), proprietary closed-source firmware, fixed form factor that requires routing dozens of high-voltage tap wires across the vehicle chassis.

---

## 3. Deep-Dive: Johannes Hübner's "Flying ADC" BMS

Johannes Hübner (creator of the open-source OpenInverter platform) created an open-source "Flying ADC" BMS to lower component costs.

### The Working Principle:
* Instead of placing a dedicated multi-channel ADC on every cell tap, a single high-precision 18-bit Delta-Sigma ADC (e.g. MCP3421 or ADS1115) is placed on a **floating, galvanically isolated power rail**.
* Solid-State Relays (Opto-MOS / PhotoMOS switches like Panasonic AQY/AQW series) sequentially switch the differential inputs of the single ADC across Cell 1, then Cell 2, then Cell 3, etc.
* Charge can also be pumped into individual low cells via an isolated DC-DC converter for active balancing.

### Why the Flying ADC Approach Has Problems in Real EV Applications:
1. **Sampling Latency & Dynamic Skew**:
   * PhotoMOS relays have turn-on/turn-off and settling delays ($5\text{ms} - 20\text{ms}$ per channel).
   * Reading 16 cells takes $200\text{ms} - 1\text{s}$.
   * During hard EV acceleration ($500\text{A}$ launch) or heavy regenerative braking, cell voltages fluctuate dramatically in milliseconds. By the time the Flying ADC reads Cell 16, the pack conditions are completely different from when it read Cell 1. This causes **false overvoltage/undervoltage trips** and distorted balance algorithms.
2. **Opto-MOS Degradation and Leakage**:
   * Solid-state relays suffer from junction leakage at elevated automotive temperatures ($>50^\circ\text{C}$).
   * If any switch leaks or fails shorted, high voltage from an upper cell is dumped into a lower cell, destroying the ADC and risking a pack short.
3. **Lack of Hardware Watchdog / Broken Wire Protection**:
   * Dedicated automotive AFEs perform instantaneous, simultaneous sampling with internal open-wire diagnostics and hardware fault comparators. The Flying ADC relies entirely on software loops.

---

## 4. Modern Standard: Dedicated Automotive AFEs (TI BQ79616 vs ADI LTC6813)

Modern OEM EVs (Tesla Model 3/Y, VW ID.4, Porsche Taycan) and open architectures (like EETEngineer's designs and FoxBMS) use dedicated **AEC-Q100 ASIL-D qualified Analog Front End (AFE) ICs**.

### Top Contenders for Open-Source EV BMS:

| Specification | Analog Devices **LTC6813-1** / **ADBMS6830** | Texas Instruments **BQ79616-Q1** | NXP **MC33771C** |
|---|---|---|---|
| **Max Cells per IC** | 18 Cells (LTC6813) / 16 Cells (ADBMS6830) | 16 Cells | 14 Cells |
| **Measurement Error** | $\pm 1.2\text{ mV}$ | $\pm 1.0\text{ mV}$ | $\pm 2.0\text{ mV}$ |
| **Simultaneous ADC Conversion**| $< 290\,\mu\text{s}$ for all 18 cells | $< 120\,\mu\text{s}$ for all 16 cells | $< 250\,\mu\text{s}$ for all 14 cells |
| **Daisy Chain Protocol** | **isoSPI** (Isolated 2-wire differential, pulse transformer up to $1000\text{V}$) | **isoUART** (Capacitive or Transformer isolated differential UART) | **TPL** (Transformer Physical Layer) |
| **Passive Balancing** | Internal FETs (up to $200\text{mA}$) or external FET gate drivers | Internal FETs (up to $150\text{mA}$) with thermal foldback | Internal FETs with PWM control |
| **GPIOs for NTCs** | 9 Aux inputs (can read 9 thermistors directly) | 8 GPIOs / ADC inputs | 7 Aux inputs |
| **Automotive Safety** | ASIL-D ready, open-wire test, self-test | ASIL-D qualified, built-in redundancy | ASIL-D qualified |
| **Availability & Cost** | High availability, JLCPCB / LCSC stocked | High availability, JLCPCB / Mouser | Available, popular in EU OEMs |

### Why Dedicated AFE ICs are the Optimal Path:
1. **Simultaneous sub-millisecond conversion**: Measures the entire 400V/800V pack at the exact same microsecond, eliminating pulse-load measurement skew.
2. **Galvanic Daisy Chain**: A single twisted-pair wire carries high-speed communications from battery module to battery module without complex optocouplers.
3. **Hardware Redundancy**: Built-in analog comparators trip alarm lines instantly if a cell exceeds safety limits, even if the main microcontroller hangs.
