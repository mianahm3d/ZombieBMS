/**
 * @file ivts_can.h
 * @brief Isabellenhütte IVT-S 1000A CAN Current Sensor & State Estimation Engine
 */

#ifndef IVTS_CAN_H
#define IVTS_CAN_H

#include "bms_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// IVT-S Standard CAN IDs
#define IVTS_CAN_ID_CURRENT         0x521   // Current in mA (signed 32-bit big-endian)
#define IVTS_CAN_ID_VOLTAGE_U1      0x522   // Pack Voltage U1 in mV
#define IVTS_CAN_ID_VOLTAGE_U2      0x523   // Inverter Bus Voltage U2 in mV
#define IVTS_CAN_ID_VOLTAGE_U3      0x524   // Auxiliary Voltage U3 in mV
#define IVTS_CAN_ID_TEMP            0x525   // Shunt Temperature in 0.1C
#define IVTS_CAN_ID_ENERGY          0x526   // Cumulative Energy in Wh
#define IVTS_CAN_ID_COULOMB_AS      0x527   // Charge in Ampere-seconds (As)
#define IVTS_CAN_ID_STATUS          0x528   // System Status & Error Flags

typedef struct {
    float    current_amps;              // Instantaneous current (+ = charge, - = discharge)
    float    pack_voltage_u1_volts;     // Total pack terminal voltage
    float    inverter_voltage_u2_volts; // Inverter DC-link capacitor voltage
    float    aux_voltage_u3_volts;      // Chassis / Aux voltage
    float    shunt_temp_c;              // Shunt temperature
    float    coulomb_charge_ah;         // Integrated Ampere-hours
    float    state_of_charge_pct;       // Estimated SoC (0.0% to 100.0%)
    float    discharge_current_limit_a; // Dynamic DCL (A)
    float    charge_current_limit_a;    // Dynamic CCL (A)
    uint32_t last_msg_timestamp_ms;     // Watchdog timestamp
    bool     sensor_healthy;            // True if sensor is communicating
} IVTS_SensorData_t;

void IVTS_Init(IVTS_SensorData_t *sensor);
bool IVTS_ProcessCANMessage(IVTS_SensorData_t *sensor, uint32_t can_id, const uint8_t *data, uint8_t dlc, uint32_t timestamp_ms);
void IVTS_CalculateDynamicLimits(IVTS_SensorData_t *sensor, uint16_t min_cell_v_mv, uint16_t max_cell_v_mv, float max_cell_temp_c);
bool IVTS_IsPrechargeReady(const IVTS_SensorData_t *sensor);

#ifdef __cplusplus
}
#endif

#endif // IVTS_CAN_H
