/**
 * @file main.c
 * @brief OpenEV-BMS Master Controller Main Execution Loop (100Hz Task Scheduler)
 * @target STM32G474RET6 @ 170MHz
 */

#include "bms_config.h"
#include "ltc6813.h"
#include "ivts_can.h"
#include "zombie_can.h"
#include "bms_state_machine.h"

// Global System State Data
static LTC6813_PackData_t g_pack_data;
static IVTS_SensorData_t  g_ivts_sensor;
static BMS_Context_t      g_bms_ctx;

void System_Init(void) {
    LTC6813_Init();
    IVTS_Init(&g_ivts_sensor);
    BMS_StateMachine_Init(&g_bms_ctx);
}

/**
 * @brief Main 100Hz Deterministic Execution Loop (10ms cycle)
 */
void BMS_Task_100Hz(uint32_t current_time_ms) {
    // 1. Start simultaneous ADC conversion on all 6x LTC6813 slave modules
    LTC6813_StartSimultaneousConversion();

    // 2. Read cell voltages & temperatures from isoSPI ring-redundant bus
    LTC6813_ReadAllVoltages(&g_pack_data);
    LTC6813_ReadAllTemperatures(&g_pack_data);

    // 3. Update dynamic Charge & Discharge Current Limits (CCL / DCL)
    IVTS_CalculateDynamicLimits(&g_ivts_sensor, 
                                g_pack_data.min_cell_voltage_mv, 
                                g_pack_data.max_cell_voltage_mv, 
                                g_pack_data.max_temp_c);

    // 4. Process safety state machine, precharge sequencing, and contactors
    BMS_StateMachine_Process100Hz(&g_bms_ctx, &g_pack_data, &g_ivts_sensor, current_time_ms);

    // 5. Broadcast Orion-compatible telemetry to ZombieVerter VCU on CAN1 (50Hz / 100Hz)
    ZombieCAN_TransmitPackSummary(&g_ivts_sensor);
    ZombieCAN_TransmitCellExtremes(&g_pack_data, &g_bms_ctx.system_status);
}

int main(void) {
    System_Init();

    uint32_t simulated_time_ms = 0;
    while (1) {
        BMS_Task_100Hz(simulated_time_ms);
        simulated_time_ms += 10;
        // In real MCU: SysTick / FreeRTOS vTaskDelayUntil
        break; // Guard for compilation/testing
    }

    return 0;
}
