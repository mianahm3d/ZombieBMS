/**
 * @file ivts_can.c
 * @brief Isabellenhütte IVT-S 1000A CAN Current Sensor & State Estimation Implementation
 */

#include "ivts_can.h"
#include <string.h>

void IVTS_Init(IVTS_SensorData_t *sensor) {
    if (!sensor) return;
    memset(sensor, 0, sizeof(IVTS_SensorData_t));
    sensor->state_of_charge_pct = 50.0f; // Default 50% until OCV sync
    sensor->discharge_current_limit_a = 300.0f; // Default 300A for Leaf pack
    sensor->charge_current_limit_a = 150.0f;    // Default 150A fast charge limit
    sensor->sensor_healthy = false;
}

static inline int32_t DecodeBigEndian32(const uint8_t *bytes) {
    return (int32_t)(((uint32_t)bytes[0] << 24) |
                     ((uint32_t)bytes[1] << 16) |
                     ((uint32_t)bytes[2] << 8)  |
                     ((uint32_t)bytes[3]));
}

bool IVTS_ProcessCANMessage(IVTS_SensorData_t *sensor, uint32_t can_id, const uint8_t *data, uint8_t dlc, uint32_t timestamp_ms) {
    if (!sensor || !data || dlc < 6) return false;

    sensor->last_msg_timestamp_ms = timestamp_ms;
    sensor->sensor_healthy = true;

    // IVT-S payload has 2 status bytes (data[0], data[1]) followed by 4-byte 32-bit big-endian value (data[2..5])
    int32_t raw_value = DecodeBigEndian32(&data[2]);

    switch (can_id) {
        case IVTS_CAN_ID_CURRENT:
            sensor->current_amps = (float)raw_value / 1000.0f; // 1 LSB = 1mA -> Amps
            break;

        case IVTS_CAN_ID_VOLTAGE_U1:
            sensor->pack_voltage_u1_volts = (float)raw_value / 1000.0f; // 1 LSB = 1mV -> Volts
            break;

        case IVTS_CAN_ID_VOLTAGE_U2:
            sensor->inverter_voltage_u2_volts = (float)raw_value / 1000.0f; // 1 LSB = 1mV -> Volts
            break;

        case IVTS_CAN_ID_VOLTAGE_U3:
            sensor->aux_voltage_u3_volts = (float)raw_value / 1000.0f;
            break;

        case IVTS_CAN_ID_TEMP:
            sensor->shunt_temp_c = (float)raw_value / 10.0f; // 1 LSB = 0.1C
            break;

        case IVTS_CAN_ID_COULOMB_AS:
            sensor->coulomb_charge_ah = (float)raw_value / 3600.0f; // 1 As = 1/3600 Ah
            break;

        default:
            return false;
    }

    return true;
}

void IVTS_CalculateDynamicLimits(IVTS_SensorData_t *sensor, uint16_t min_cell_v_mv, uint16_t max_cell_v_mv, float max_cell_temp_c) {
    if (!sensor) return;

    float max_discharge_a = 400.0f; // Leaf 40kWh peak discharge
    float max_charge_a = 150.0f;    // Leaf 40kWh peak charge / CCS

    // 1. Voltage-based Discharge Derate (Low cell voltage)
    if (min_cell_v_mv <= CELL_CRITICAL_LOW_VOLTAGE_MV) {
        max_discharge_a = 0.0f;
    } else if (min_cell_v_mv < CELL_WARN_LOW_VOLTAGE_MV) {
        float ratio = (float)(min_cell_v_mv - CELL_CRITICAL_LOW_VOLTAGE_MV) / (float)(CELL_WARN_LOW_VOLTAGE_MV - CELL_CRITICAL_LOW_VOLTAGE_MV);
        max_discharge_a *= ratio;
    }

    // 2. Voltage-based Charge Derate (High cell voltage)
    if (max_cell_v_mv >= CELL_CRITICAL_HIGH_VOLTAGE_MV) {
        max_charge_a = 0.0f;
    } else if (max_cell_v_mv > CELL_WARN_HIGH_VOLTAGE_MV) {
        float ratio = (float)(CELL_CRITICAL_HIGH_VOLTAGE_MV - max_cell_v_mv) / (float)(CELL_CRITICAL_HIGH_VOLTAGE_MV - CELL_WARN_HIGH_VOLTAGE_MV);
        max_charge_a *= ratio;
    }

    // 3. Thermal-based Derate
    if (max_cell_temp_c >= CELL_TEMP_DISCHARGE_CRIT_C) {
        max_discharge_a = 0.0f;
        max_charge_a = 0.0f;
    } else if (max_cell_temp_c > CELL_TEMP_DISCHARGE_MAX_C) {
        float t_ratio = (CELL_TEMP_DISCHARGE_CRIT_C - max_cell_temp_c) / (CELL_TEMP_DISCHARGE_CRIT_C - CELL_TEMP_DISCHARGE_MAX_C);
        max_discharge_a *= t_ratio;
        max_charge_a *= t_ratio;
    }

    sensor->discharge_current_limit_a = max_discharge_a;
    sensor->charge_current_limit_a = max_charge_a;
}

bool IVTS_IsPrechargeReady(const IVTS_SensorData_t *sensor) {
    if (!sensor || sensor->pack_voltage_u1_volts < 50.0f) return false;
    float ratio = sensor->inverter_voltage_u2_volts / sensor->pack_voltage_u1_volts;
    return (ratio >= PRECHARGE_TARGET_RATIO);
}
