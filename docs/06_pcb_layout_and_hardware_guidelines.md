# 06. BMS PCB Layout, High-Current & Thermal Design Guidelines

Battery Management System PCBs present a unique engineering challenge: they must route high-current switching paths (charging, discharging, contactor coils) while simultaneously taking microvolt/millivolt precision measurements from sensitive battery cells without noise corruption.

---

## 1. Layer Stack-Up Strategy (4-Layer Standard)

A 4-layer stackup with a solid, unbroken ground plane on Layer 2 is the minimum requirement for automotive-grade signal integrity and EMC:

```
[Layer 1: TOP]    -> Low-noise analog cell sense traces, AFE ICs, discrete passive components
----------------- -> Dielectric (0.2mm FR4)
[Layer 2: GND]    -> Solid, unbroken Low-Voltage GND Plane (shielding and return paths)
----------------- -> Core (1.0mm FR4)
[Layer 3: PWR/HV] -> Isolated power planes (3.3V, 5V, 12V, isolated DC-DC supplies)
----------------- -> Dielectric (0.2mm FR4)
[Layer 4: BOTTOM] -> High-current traces, balancing bleed resistors, thermal copper pours
```

---

## 2. High-Current Traces & Kelvin Sensing

### A. IPC-2152 Trace Width Calculations
For internal/external traces carrying balancing currents or contactor economizer currents:
* **Balancing Paths ($200\text{mA} - 500\text{mA}$)**: Minimum $0.5\text{ mm} - 1.0\text{ mm}$ trace width on $1\text{ oz}$ ($35\,\mu\text{m}$) copper.
* **Master High-Current Bus (Current Shunt interface)**: Use dedicated 4-terminal Kelvin sense connections.

### B. Kelvin Current Shunt Layout
Never connect measurement ADC traces directly to the high-current power lugs. Use dedicated Kelvin sense pads located directly across the active resistance element:

```
        High Current IN (e.g. 500A)                         High Current OUT
   ========================[ Manganin Shunt Element ]========================
                              |                 |
                              | (Kelvin Pad +)  | (Kelvin Pad -)
                              +--------+ +------+
                                       | |  <-- Symmetrical Differential Pair (100 ohm)
                                       v v
                              [ Anti-Aliasing RC Filter ] (10 ohm + 100nF C0G)
                                       | |
                                       v v
                              [ Isolated ADC Modulator ] (ADuM7701 / AMC1301)
```

---

## 3. Cell Tap Filtering & Hot-Plug ESD Protection

When plugging in a multi-pin battery harness, pins never make contact simultaneously. High-voltage transients (hot-plug surges) can easily destroy input multiplexer FETs inside the AFE IC.

### Protection Circuit Topology per Cell Tap:
1. **Differential Low-Pass Filter**: $100\,\Omega$ series resistor + $100\text{nF}$ X7R ceramic capacitor directly across adjacent cell inputs (cutoff frequency $f_c \approx 8\text{kHz}$).
2. **Bidirectional TVS Diodes / Zener Clamps**: Clamp transient spikes to $<6\text{V}$ per cell tap channel.
3. **Ferrite Beads**: Place on harness connector pins to reject inverter PWM switching EMI ($10\text{kHz} - 20\text{kHz}$ common mode noise).

---

## 4. Thermal Management of Passive Balancing Bleeders

Passive balancing dissipates battery energy as heat ($P = I^2 R$).
* For a 16s board balancing at $150\text{mA}$ per cell ($4\text{V} \times 0.15\text{A} = 0.6\text{W}$ per cell):
  $$\text{Total Heat} = 16 \times 0.6\text{W} \approx 9.6\text{W}$$
* **Thermal Placement Rules**:
  1. Distribute bleed resistors across the bottom PCB perimeter with large copper thermal pours ($2\text{ oz}$ copper preferred).
  2. Place an array of stitching vias ($0.3\text{ mm}$ hole, $0.6\text{ mm}$ pad, $1.0\text{ mm}$ grid) under every power resistor to conduct heat to the enclosure/heatsink.
  3. **Crucial**: Keep bleed resistors far away ($>15\text{ mm}$) from the AFE IC and onboard temperature reference sensors to prevent false thermal trips.

---

## 5. Galvanic Isolation & Creepage Slots

* Maintain a physical **Isolation Moat** (minimum $6.0\text{ mm} - 8.0\text{ mm}$ clearance) separating High Voltage battery domains from the 12V chassis domain.
* Mill physical **PCB isolation slots (air cutouts $\ge 2.0\text{ mm}$ wide)** beneath isolation transformers (e.g. Pulse HM2101NL), optocouplers, and isolated DC-DC converters to prevent surface carbon tracking and dust contamination.
