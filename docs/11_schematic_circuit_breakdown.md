# 11. Complete Schematic Circuit Breakdown & Netlist Specification

This document details the exact electrical connections, component parameters, filtering time constants, and signal nets for the **18-Channel AFE Slave Module** and the **Master Controller Board**.

---

## 1. 18-Channel AFE Slave Module (`Slave_AFE_18s`)

### A. Cell Tap Input Filtering & Protection (Per Channel $k = 1 \dots 18$)
Each differential cell tap connects from the battery harness connector $J_1$ to the LTC6813 AFE through a matched low-pass RC filter and transient clamp:

```
  Harness Pin C[k] -----[ R_series = 100R ]-----+-----> LTC6813 Pin C[k]
                                                |
                                          [ C_diff = 100nF ] (Across C[k] and C[k-1])
                                                |
  Harness Pin C[k-1] ---[ R_series = 100R ]-----+-----> LTC6813 Pin C[k-1]
                                                |
                                        [ TVS Clamp 5.6V ]
```

* **Cutoff Frequency**:
  $$f_c = \frac{1}{2\pi \cdot (2 \cdot R_{\text{series}}) \cdot C_{\text{diff}}} = \frac{1}{2\pi \cdot 200\,\Omega \cdot 100\text{ nF}} \approx \mathbf{7.96\text{ kHz}}$$
  *Effect*: Rejects high-frequency inverter PWM switching noise ($10\text{kHz} - 20\text{kHz}$) while providing sub-millisecond response to real cell step changes.

### B. Passive Bleed Balancing Circuit (Per Channel $k = 1 \dots 18$)
Instead of relying on the LTC6813 internal micro-switches (which heat up the silicon die and induce measurement drift), we drive external **AO3400A N-Channel MOSFETs**:

```
  LTC6813 S[k] Gate Pin -----[ 1k0 Resistor ]-----> Gate of AO3400A (Q[k])
                                                     |
  Cell Tap [k] -----[ 33R 2512 1W Resistor ]-----> Drain of AO3400A
                                                     |
  Cell Tap [k-1] --------------------------------> Source of AO3400A
```

* **Bleed Current at Nominal $4.0\text{V}$**:
  $$I_{\text{bleed}} = \frac{4.0\text{V}}{33\,\Omega + R_{\text{ds(on)}}} \approx \frac{4.0\text{V}}{33.03\,\Omega} \approx \mathbf{121\text{ mA}}$$
* **Power Dissipated per Resistor**:
  $$P = I^2 R = (0.121\text{A})^2 \times 33\,\Omega \approx \mathbf{0.483\text{ W}} \quad (\text{Rated for } 1.0\text{W } 2512 \text{ package})$$

### C. NTC Thermistor Matrix (Channels $1 \dots 9$)
* 9 channels connected to the LTC6813 `GPIO1` through `GPIO9` pins.
* Biased from the internal precision $3.0\text{V} / 3.3\text{V}$ `VREF2` output through $10.0\text{k}\Omega$ 0.1% 25ppm pull-up resistors.
* Filtered with $10\text{nF}$ ceramic capacitors to prevent RF rectification.

### D. Dual isoSPI Daisy-Chain Pulse Interface
* **Inbound (Port A)**: LTC6813 `IPB` / `IMB` pins $\leftrightarrow$ Pulse `HM2101NL` Transformer Primary $\leftrightarrow$ 4-pin Micro-Fit harness.
* **Outbound (Port B)**: LTC6813 `IPA` / `IMA` pins $\leftrightarrow$ Pulse `HM2101NL` Transformer Secondary $\leftrightarrow$ Next Slave Board.
* **Common-Mode Termination**: Center taps biased with $10\text{nF}$ $3000\text{V}$ high-voltage capacitors for robust common-mode surge immunity.

---

## 2. Master Controller Board (`Master_Controller`)

### A. Processing Core: STM32G474RET6
* **Clock**: $8.000\text{MHz}$ automotive external crystal multiplied via internal PLL to **$170\text{MHz}$**.
* **SWD Header**: 6-pin JST-SH connector exposing `SWCLK`, `SWDIO`, `NRST`, `3.3V`, `GND`, and `SWO` for ST-Link / J-Link flashing.

### B. Dual LTC6820 isoSPI Ring-Redundancy Transceivers
* **Transceiver 1 (Port A - Forward Driver)**: Connected to STM32 `SPI1` (`SCK`, `MISO`, `MOSI`, `NSS`).
* **Transceiver 2 (Port B - Reverse Driver)**: Connected to STM32 `SPI2` (`SCK`, `MISO`, `MOSI`, `NSS`).
* If a packet error or broken wire is detected on SPI1, SPI2 initiates reverse polling in $<50\,\mu\text{s}$.

### C. Triple Automotive Isolated CAN-FD Subsystem
1. **CAN 1 (Drivetrain / VCU / Inverter)**: STM32 `FDCAN1` $\to$ `TCAN1042HVDQ1` transceiver $\to$ 120-Ohm termination $\to$ Vehicle bus.
2. **CAN 2 (Isabellenhütte IVT-S 1000A Current Sensor)**: STM32 `FDCAN2` $\to$ `TCAN1042HVDQ1` transceiver $\to$ 120-Ohm termination $\to$ IVT-S high-voltage shunt.
3. **CAN 3 (Onboard Charger / J1772)**: STM32 `FDCAN3` $\to$ `TCAN1042HVDQ1` transceiver $\to$ Isolated charging port.

### D. Smart Contactor & Thermal Fan Drivers (TI `TPS274C120`)
4 independent automotive high-side switches capable of delivering up to $4.0\text{A}$ continuous:
1. **Main Positive Contactor Driver**:
   * Initial Pull-in: 100% duty cycle ($12\text{V}$) for $100\text{ms}$.
   * Hold-in Economizer: PWM throttled to 35% duty cycle ($25\text{kHz}$) to eliminate coil heating.
2. **Main Negative Contactor Driver**: Controls low-side return contactor.
3. **Precharge Relay Driver**: Energizes precharge circuit; monitored against IVT-S $U_2 / U_1 \ge 0.95$.
4. **12V 4-Wire PWM Thermal Fan Driver**: Proportional fan speed control ($0-100\%$) based on maximum pack thermistor temperature.

### E. Active Insulation Monitoring (IMD / Ground Fault Bridge)
* High-voltage sensing nodes connected to HV+ and HV- via strings of $1.0\text{M}\Omega$ 500V high-voltage resistors.
* Switched to chassis 12V ground via two Vishay `VO14642` $5300\text{V}_{\text{RMS}}$ optical solid-state relays.
* The STM32 12-bit ADC measures the bridge voltage drop to compute the exact isolation resistance in $\text{k}\Omega/\text{V}$, raising an alarm if isolation drops below $500\,\Omega/\text{V}$.
