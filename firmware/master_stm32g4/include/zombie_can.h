/**
 * @file zombie_can.h
 * @brief ZombieVerter VCU & Orion BMS CAN Broadcast Interface
 */

#ifndef ZOMBIE_CAN_H
#define ZOMBIE_CAN_H

#include "bms_config.h"
#include "ltc6813.h"
#include "ivts_can.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZOMBIE_CAN_ID_PACK_SUMMARY     0x6B0   // Orion Pack Summary (Current, Pack V, SoC, DCL, CCL)
#define ZOMBIE_CAN_ID_CELL_EXTREMES    0x03C   // Orion Cell Extremes (High/Low V, High/Low Temp)

typedef struct {
    uint8_t  relay_state_mask; // Bit 0: Main+, Bit 1: Main-, Bit 2: Precharge, Bit 3: Fan
    uint16_t dtc_fault_mask;   // Bitmask of active Diagnostic Trouble Codes
} BMS_SystemStatus_t;

void ZombieCAN_TransmitPackSummary(const IVTS_SensorData_t *sensor);
void ZombieCAN_TransmitCellExtremes(const LTC6813_PackData_t *pack, const BMS_SystemStatus_t *status);

#ifdef __cplusplus
}
#endif

#endif // ZOMBIE_CAN_H
