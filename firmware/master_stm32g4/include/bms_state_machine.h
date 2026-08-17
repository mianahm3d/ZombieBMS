/**
 * @file bms_state_machine.h
 * @brief Deterministic Automotive Safety State Machine for OpenEV-BMS
 */

#ifndef BMS_STATE_MACHINE_H
#define BMS_STATE_MACHINE_H

#include "bms_config.h"
#include "ltc6813.h"
#include "ivts_can.h"
#include "zombie_can.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BMS_STATE_INIT = 0,
    BMS_STATE_STANDBY,
    BMS_STATE_PRECHARGE,
    BMS_STATE_DRIVE,
    BMS_STATE_CHARGE_AC,
    BMS_STATE_CHARGE_CCS,
    BMS_STATE_FAULT
} BMS_State_t;

typedef enum {
    FAULT_NONE                  = 0x0000,
    FAULT_CELL_OVERVOLTAGE      = 0x0001,
    FAULT_CELL_UNDERVOLTAGE     = 0x0002,
    FAULT_CELL_OVERTEMP         = 0x0004,
    FAULT_CELL_UNDERTEMP        = 0x0008,
    FAULT_ISOLATION_IMD         = 0x0010,
    FAULT_PRECHARGE_TIMEOUT     = 0x0020,
    FAULT_CONTACTOR_WELDED      = 0x0040,
    FAULT_IVTS_SENSOR_OFFLINE   = 0x0080,
    FAULT_SLAVE_AFE_OFFLINE     = 0x0100,
    FAULT_HVIL_LOOP_OPEN        = 0x0200
} BMS_Fault_t;

typedef struct {
    BMS_State_t         current_state;
    BMS_Fault_t         active_faults;
    uint32_t            state_entry_time_ms;
    BMS_SystemStatus_t  system_status;
    bool                ignition_signal_kl15;
    bool                charge_request_pilot;
    bool                hvil_closed;
} BMS_Context_t;

void BMS_StateMachine_Init(BMS_Context_t *ctx);
void BMS_StateMachine_Process100Hz(BMS_Context_t *ctx, const LTC6813_PackData_t *pack, const IVTS_SensorData_t *sensor, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // BMS_STATE_MACHINE_H
