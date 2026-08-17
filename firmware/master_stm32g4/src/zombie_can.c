/**
 * @file zombie_can.c
 * @brief ZombieVerter VCU & Orion CAN Broadcast Implementation
 */

#include "zombie_can.h"
#include <string.h>

// Low-level hardware CAN1 transmit stub
static void CAN1_Transmit(uint32_t can_id, const uint8_t *data, uint8_t dlc) {
    (void)can_id;
    (void)data;
    (void)dlc;
}

void ZombieCAN_TransmitPackSummary(const IVTS_SensorData_t *sensor) {
    if (!sensor) return;

    uint8_t msg[8];
    memset(msg, 0, sizeof(msg));

    // Byte 0-1: Pack Current in 0.1A (signed big-endian)
    int16_t curr_01a = (int16_t)(sensor->current_amps * 10.0f);
    msg[0] = (curr_01a >> 8) & 0xFF;
    msg[1] = curr_01a & 0xFF;

    // Byte 2-3: Pack Total Voltage in 0.1V
    uint16_t volt_01v = (uint16_t)(sensor->pack_voltage_u1_volts * 10.0f);
    msg[2] = (volt_01v >> 8) & 0xFF;
    msg[3] = volt_01v & 0xFF;

    // Byte 4: State of Charge (0.5% / bit) -> 0..200
    msg[4] = (uint8_t)(sensor->state_of_charge_pct * 2.0f);

    // Byte 5-6: Discharge Current Limit (DCL in Amps)
    uint16_t dcl_a = (uint16_t)sensor->discharge_current_limit_a;
    msg[5] = (dcl_a >> 8) & 0xFF;
    msg[6] = dcl_a & 0xFF;

    // Byte 7: Charge Current Limit (CCL in Amps)
    msg[7] = (uint8_t)sensor->charge_current_limit_a;

    CAN1_Transmit(ZOMBIE_CAN_ID_PACK_SUMMARY, msg, 8);
}

void ZombieCAN_TransmitCellExtremes(const LTC6813_PackData_t *pack, const BMS_SystemStatus_t *status) {
    if (!pack || !status) return;

    uint8_t msg[8];
    memset(msg, 0, sizeof(msg));

    // Byte 0-1: Highest Cell Voltage in mV
    msg[0] = (pack->max_cell_voltage_mv >> 8) & 0xFF;
    msg[1] = pack->max_cell_voltage_mv & 0xFF;

    // Byte 2-3: Lowest Cell Voltage in mV
    msg[2] = (pack->min_cell_voltage_mv >> 8) & 0xFF;
    msg[3] = pack->min_cell_voltage_mv & 0xFF;

    // Byte 4: Highest Cell Temp (offset +40C)
    msg[4] = (uint8_t)(pack->max_temp_c + 40.0f);

    // Byte 5: Lowest Cell Temp (offset +40C)
    msg[5] = (uint8_t)(pack->min_temp_c + 40.0f);

    // Byte 6: Relay State Bitmask
    msg[6] = status->relay_state_mask;

    // Byte 7: Fault Code
    msg[7] = (uint8_t)(status->dtc_fault_mask & 0xFF);

    CAN1_Transmit(ZOMBIE_CAN_ID_CELL_EXTREMES, msg, 8);
}
