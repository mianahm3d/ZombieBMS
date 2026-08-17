# 04. Automotive EV Safety Standards & Fault Matrix

Safety is the paramount requirement for an EV Battery Management System. A high-voltage traction pack ($100\text{V} - 800\text{V}$, $20\text{kWh} - 100\text{kWh}$) stores massive energy and can deliver kilo-amperes of short-circuit current.

---

## 1. Regulatory & Engineering Standards

Our open-source BMS is designed with compliance to the following global automotive standards:

* **UN ECE R100 (Revision 2/3)**: International standard for electric vehicle powertrain electrical safety, insulation resistance ($>500\,\Omega/\text{V}$ for DC), and thermal propagation prevention.
* **ISO 26262 (ASIL C/D)**: Automotive Functional Safety. Covers hardware metric targets (Single Point Fault Metric $\ge 97\%$, Latent Fault Metric $\ge 80\%$) and fail-safe state machines.
* **IEC 60664-1**: Insulation coordination for low-voltage equipment (determines PCB creepage and clearance distances).
* **SAE J1772 / ISO 15118**: EV charging communications and pilot line interlocks.

---

## 2. Critical Safety Mechanisms

### A. Contactor Weld Detection (Before & After Every Drive Cycle)
Contactors can suffer from contact welding (contacts permanently fused together due to arcing or heavy inrush currents). If a contactor is welded closed, the vehicle high-voltage bus remains live even when the ignition is switched off.

```
       HV+ [Main Contactor +] -----[V_PackSense]----+-----[V_BusSense]-----> Inverter (+)
                                                     |
                                            (Voltage Divider)
                                                     |
                                                     v
                                            [Master BMS ADC]
```

**Diagnostic Procedure:**
1. **Before Closing Contactors**: Measure $V_{\text{BusSense}}$ relative to chassis. If $V_{\text{BusSense}} \approx V_{\text{Pack}}$, one or both contactors are **WELDED**. Abort startup and trigger critical fault.
2. **After Opening Contactors (Shutdown)**: Discharge the DC-link capacitor and verify that $V_{\text{BusSense}}$ drops to $<60\text{V}$ within $5$ seconds (as mandated by ECE R100). If voltage persists, open the secondary contactor and notify the user.

### B. High Voltage Interlock Loop (HVIL)
* A dedicated low-voltage continuous loop ($12\text{V}$ or pulsed PWM) passing through all high-voltage service disconnect plugs, inverter covers, battery enclosure lids, and heavy-gauge cable connectors.
* If any high-voltage plug is pulled or an enclosure lid is opened while the system is energized, the HVIL circuit breaks instantly ($<10\text{ms}$), causing the Master BMS to drop all main contactors before human contact can occur with live conductors.

### C. PCB Creepage and Clearance Rules (IEC 60664-1)
For an $800\text{V}$ DC pack (Overvoltage Category II, Pollution Degree 2, Material Group IIIa FR4):
* **Clearance (through air)**: Minimum $4.0\text{ mm}$ between High Voltage rails and 12V Low Voltage Ground.
* **Creepage (along PCB surface)**: Minimum $8.0\text{ mm}$ without isolation slot; or use routed PCB isolation cutouts (air slots) of $2.5\text{ mm}$ to break creepage tracking paths.

---

## 3. Comprehensive Fault Matrix & Action Response

| Fault Type | Trigger Condition | Reaction Level | Action Taken |
|---|---|---|---|
| **Cell Overvoltage (COV)** | Any cell $> 4.25\text{V}$ ($> 3.65\text{V}$ LFP) | **Critical** | Instantly set CCL = 0A, open Charge Contactor within $50\text{ms}$ |
| **Cell Undervoltage (CUV)** | Any cell $< 2.80\text{V}$ ($< 2.50\text{V}$ LFP) | **Critical** | Instantly set DCL = 0A, open Main Contactors within $100\text{ms}$ |
| **Cell Overtemperature (COT)**| Any sensor $> 55^\circ\text{C}$ ($> 60^\circ\text{C}$ critical) | **Warning -> Critical** | Derate current linearly -> Full shutdown if $>60^\circ\text{C}$ |
| **Cell Undertemperature (CUT)**| Charging attempted at $< 0^\circ\text{C}$ | **Warning** | Throttle CCL to 0A (prevent lithium dendrite plating) |
| **Pack Overcurrent (Discharge)**| $I_{\text{pack}} > I_{\text{peak\_limit}}$ for $> 200\text{ms}$ | **Critical** | Trip contactors, enable buzzer/MIL |
| **Short Circuit in Traction Bus**| $I_{\text{pack}} > 1000\text{A}$ instantaneous | **Catastrophic** | Fast hardware comparator trips gate driver in $<5\,\mu\text{s}$, Pyro-fuse triggered |
| **Insulation Fault (IMD)** | Isolation resistance $< 500\,\Omega/\text{V}$ | **Warning / Critical** | Amber warning light if $<500\,\Omega/\text{V}$, inhibit restart if $<100\,\Omega/\text{V}$ |
| **AFE Daisy Chain Loss** | Missed isoSPI/UART frame $> 100\text{ms}$ | **Critical** | Safe state transition: derate power, open contactors safely |
| **Precharge Timeout** | $V_{\text{Bus}} < 95\% \, V_{\text{Pack}}$ after $1500\text{ms}$ | **Critical** | Open Precharge relay immediately (prevent resistor burnout) |
