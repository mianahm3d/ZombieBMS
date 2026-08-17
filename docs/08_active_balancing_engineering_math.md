# 08. Active Balancing: Topologies, Mathematical Models & Circuit Design

Passive bleed balancing works by turning excess cell electrical energy into waste heat. In high-capacity EV packs ($50\text{Ah} - 200\text{Ah}$ cells), passive balancing is painfully slow and generates dangerous thermal loads. 

This document details the engineering mathematics, topology selection, and hardware schematic implementation for **Active Balancing** across 16s to 200s strings.

---

## 1. Mathematical Analysis: Active vs Passive Balancing

### A. Balancing Time Calculation
Let $\Delta Q$ be the capacity delta between the highest cell and the lowest cell in ampere-hours ($\text{Ah}$).

$$t_{\text{balance}} = \frac{\Delta Q}{I_{\text{balance}} \cdot D}$$

Where:
* $I_{\text{balance}}$ is the effective balancing current.
* $D$ is the balancing duty cycle ($0 < D \le 1.0$).

#### Real-World Case Study:
Consider a **96s pack (100Ah prismatic cells)** with a **$4\text{Ah}$ capacity mismatch** (common in used EV modules or after deep storage).

| Balancing Method | Balancing Current ($I_{\text{bal}}$) | Time to Balance $4\text{Ah}$ ($t_{\text{bal}}$) | Heat Generated ($P_{\text{loss}}$ per 10 active cells) | Energy Preserved ($\eta$) |
|---|---|---|---|---|
| **Orion / AEM (Passive Bleed)** | $150\text{mA}$ ($0.15\text{A}$) | **$26.7\text{ Hours}$** | $10 \times (0.15\text{A} \times 3.7\text{V}) = \mathbf{5.55\text{ W}}$ (100% heat) | **$0\%$** (Wasted) |
| **Tesla OEM (Passive Bleed)** | $100\text{mA}$ ($0.10\text{A}$) | **$40.0\text{ Hours}$** | $10 \times (0.10\text{A} \times 3.7\text{V}) = \mathbf{3.70\text{ W}}$ (100% heat) | **$0\%$** (Wasted) |
| **OpenEV-BMS Active Engine** | **$2.5\text{A}$ Continuous** | **$1.6\text{ Hours}$** | $10 \times [2.5\text{A} \times 3.7\text{V} \times (1 - 0.90)] = \mathbf{9.25\text{ W}}$ for entire 25A transfer! | **$\mathbf{\approx 90\%}$ Transferred** |
| **OpenEV-BMS Boost Mode** | **$4.0\text{A}$ Peak** | **$1.0\text{ Hour}$** | Dynamically throttled via NTC closed-loop | **$\mathbf{\approx 88\%}$ Transferred** |

---

## 2. Active Balancing Topology Trade-Offs

```
========================================================================================================
TOPOLOGY 1: ADJACENT CELL-TO-CELL (BUCK-BOOST)
  [Cell 1] <---> (Inductor L1) <---> [Cell 2] <---> (Inductor L2) <---> [Cell 3] ...
  - Transfer Efficiency across N cells: \eta_{total} = (\eta_{single})^{N-1}
  - For a transfer from Cell 1 to Cell 18: 0.90^{17} = 16.6% efficiency!
  - VERDICT: Inefficient for large series strings; only good for immediately adjacent cells.
========================================================================================================
TOPOLOGY 2: BIDIRECTIONAL ISOLATED FLYBACK (CELL-TO-MODULE BUS)
  [Cell K] <---> [Bidirectional Flyback Transformer] <---> [16s / 18s Module Rail (48V-75V)]
  - Single-stage conversion: Direct transfer from ANY cell to the module bus (or vice-versa).
  - Constant efficiency: \eta \approx 88% - 92% regardless of cell position in the string!
  - VERDICT: THE OPTIMAL AUTOMOTIVE CHOICE.
========================================================================================================
TOPOLOGY 3: RESONANT SWITCHED-CAPACITOR (LC ZCS MATRIX)
  [Cell K] <---> [MOSFET Matrix] <---> [Resonant LC Tank] <---> [Lowest Cell M]
  - Zero Current Switching (ZCS) eliminates switching losses.
  - Automatically equalizes highest to lowest potential without complex feedback loops.
  - VERDICT: EXCELLENT HIGH-SPEED AUTONOMOUS SECONDARY OPTION.
========================================================================================================
```

---

## 3. Circuit Design: 16s/18s Bidirectional Flyback Active Balancing Engine

In our architecture, each 16s/18s Slave Module incorporates an isolated bidirectional DC-DC engine:

```
                  +-----------------------------------------------------------+
                  |               16s / 18s MODULE BUS (48V - 75V)            |
                  +-----------------------------------------------------------+
                                               |
                                      [ High-Voltage FET Q_HV ]
                                               |
                               +---------------+---------------+
                               |                               |
                     +-------------------+           +-------------------+
                     | Primary Winding   |           | Secondary Winding |
                     |   (L_p = 100uH)   | ))     (( |   (L_s = 2.5uH)   |
                     +-------------------+           +-------------------+
                               |                               |
                     [ Isolated Core T1 ]            [ Low-Voltage Sync FET Q_LV ]
                                                               |
                                            +------------------+------------------+
                                            |                  |                  |
                                         [MUX K1]           [MUX K2]           [MUX K_N]
                                            |                  |                  |
                                         [Cell 1]           [Cell 2]          [Cell N]
```

### Operational Modes:
1. **Discharge High Cell (Cell $\to$ Module Bus)**:
   * MUX connects the secondary winding across the highest cell ($V_{\text{high}} \approx 4.15\text{V}$).
   * $Q_{\text{LV}}$ turns ON, storing energy in the transformer magnetic core ($E = \frac{1}{2} L_s I_{\text{peak}}^2$).
   * $Q_{\text{LV}}$ turns OFF, $Q_{\text{HV}}$ turns ON (synchronous rectification), dumping the energy into the entire 48V–75V module bus.
   * *Result*: 90% of the high cell's excess energy charges all other 15–17 cells equally.

2. **Charge Low Cell (Module Bus $\to$ Cell)**:
   * $Q_{\text{HV}}$ energizes the primary from the 48V–75V module rail.
   * Energy is discharged through $Q_{\text{LV}}$ directly into the single weakest cell.

### Key Component Calculations:
* **Switching Frequency**: $f_{\text{sw}} = 150\text{ kHz}$
* **Transformer Turn Ratio**:
  $$n = \frac{N_p}{N_s} = \frac{V_{\text{module\_nom}}}{V_{\text{cell\_nom}}} = \frac{60\text{V}}{3.7\text{V}} \approx 16 : 1$$
* **Secondary Peak Current**:
  $$I_{s,\text{peak}} = \frac{2 \cdot I_{\text{bal\_avg}}}{1 - D} = \frac{2 \cdot 2.5\text{A}}{0.5} = 10\text{A}$$
* **Primary Peak Current**:
  $$I_{p,\text{peak}} = \frac{I_{s,\text{peak}}}{n} = \frac{10\text{A}}{16} = 0.625\text{A}$$
* **Secondary Inductance**:
  $$L_s = \frac{V_{\text{cell}} \cdot D}{I_{s,\text{peak}} \cdot f_{\text{sw}}} = \frac{3.7\text{V} \cdot 0.5}{10\text{A} \cdot 150\text{kHz}} \approx 1.23\,\mu\text{H}$$
* **Primary Inductance**:
  $$L_p = n^2 \cdot L_s = 16^2 \cdot 1.23\,\mu\text{H} \approx 315\,\mu\text{H}$$

---

## 4. Autonomous Balancing Control Algorithm

The Master MCU runs an active balancing optimization routine every $1.0\text{ second}$:

```
                    [ Sample All 96s-200s Voltages Simultaneously ]
                                          |
                                          v
                    [ Calculate Mean Voltage (V_avg) & Standard Dev (sigma) ]
                                          |
                     +--------------------+--------------------+
                     |                                         |
            (V_max - V_min < 10mV)                    (V_max - V_min >= 10mV)
                     |                                         |
                     v                                         v
              [ BALANCING IDLE ]                     [ Check Temperature Window ]
                                                               |
                                            +------------------+------------------+
                                            |                                     |
                                  (Temp < 5C or > 45C)                   (5C <= Temp <= 45C)
                                            |                                     |
                                            v                                     v
                                    [ Inhibit Balancing ]               [ Select Highest Cell ]
                                                                                  |
                                                                        [ Engage Active Engine ]
                                                                        (I_bal = 2.5A Flyback)
```
