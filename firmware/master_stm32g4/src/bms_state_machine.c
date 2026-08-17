/**
 * @file bms_state_machine.c
 * @brief Automotive Safety State Machine Implementation
 */

#include "bms_state_machine.h"
#include <string.h>

static void SetContactorOutputs(bool main_pos, bool main_neg, bool precharge) {
    // Controls TPS274C120 smart high-side drivers
    (void)main_pos;
    (void)main_neg;
    (void)precharge;
}

void BMS_StateMachine_Init(BMS_Context_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(BMS_Context_t));
    ctx->current_state = BMS_STATE_INIT;
    ctx->active_faults = FAULT_NONE;
    ctx->hvil_closed = true;
}

static void EvaluateFaults(BMS_Context_t *ctx, const LTC6813_PackData_t *pack, const IVTS_SensorData_t *sensor) {
    ctx->active_faults = FAULT_NONE;

    if (!ctx->hvil_closed) {
        ctx->active_faults |= FAULT_HVIL_LOOP_OPEN;
    }

    if (!sensor->sensor_healthy) {
        ctx->active_faults |= FAULT_IVTS_SENSOR_OFFLINE;
    }

    if (pack->max_cell_voltage_mv >= CELL_CRITICAL_HIGH_VOLTAGE_MV) {
        ctx->active_faults |= FAULT_CELL_OVERVOLTAGE;
    }

    if (pack->min_cell_voltage_mv <= CELL_CRITICAL_LOW_VOLTAGE_MV) {
        ctx->active_faults |= FAULT_CELL_UNDERVOLTAGE;
    }

    if (pack->max_temp_c >= CELL_TEMP_DISCHARGE_CRIT_C) {
        ctx->active_faults |= FAULT_CELL_OVERTEMP;
    }
}

void BMS_StateMachine_Process100Hz(BMS_Context_t *ctx, const LTC6813_PackData_t *pack, const IVTS_SensorData_t *sensor, uint32_t now_ms) {
    if (!ctx || !pack || !sensor) return;

    EvaluateFaults(ctx, pack, sensor);

    // If critical faults are active, immediately trip to FAULT state
    if (ctx->active_faults != FAULT_NONE && ctx->current_state != BMS_STATE_FAULT) {
        ctx->current_state = BMS_STATE_FAULT;
        ctx->state_entry_time_ms = now_ms;
        SetContactorOutputs(false, false, false);
        return;
    }

    switch (ctx->current_state) {
        case BMS_STATE_INIT:
            // Self-test and initial checks passed
            ctx->current_state = BMS_STATE_STANDBY;
            ctx->state_entry_time_ms = now_ms;
            SetContactorOutputs(false, false, false);
            break;

        case BMS_STATE_STANDBY:
            SetContactorOutputs(false, false, false);
            if (ctx->ignition_signal_kl15) {
                ctx->current_state = BMS_STATE_PRECHARGE;
                ctx->state_entry_time_ms = now_ms;
                // Close Main Negative and Precharge Relay
                SetContactorOutputs(false, true, true);
                ctx->system_status.relay_state_mask = 0x06; // Precharge + Main-
            } else if (ctx->charge_request_pilot) {
                ctx->current_state = BMS_STATE_CHARGE_AC;
                ctx->state_entry_time_ms = now_ms;
            }
            break;

        case BMS_STATE_PRECHARGE:
            // Check if inverter bus is charged to >= 95% of pack voltage
            if (IVTS_IsPrechargeReady(sensor)) {
                // Success: Engage Main Positive with 35% PWM hold economizer, Open Precharge
                SetContactorOutputs(true, true, false);
                ctx->system_status.relay_state_mask = 0x03; // Main+ and Main-
                ctx->current_state = BMS_STATE_DRIVE;
                ctx->state_entry_time_ms = now_ms;
            } else if ((now_ms - ctx->state_entry_time_ms) > PRECHARGE_TIMEOUT_MS) {
                // Timeout: Inverter failed to charge (short-circuit on DC bus)
                ctx->active_faults |= FAULT_PRECHARGE_TIMEOUT;
                ctx->current_state = BMS_STATE_FAULT;
                SetContactorOutputs(false, false, false);
            }
            break;

        case BMS_STATE_DRIVE:
            if (!ctx->ignition_signal_kl15) {
                // Ignition switched off -> Return to Standby safely
                SetContactorOutputs(false, false, false);
                ctx->system_status.relay_state_mask = 0x00;
                ctx->current_state = BMS_STATE_STANDBY;
            }
            break;

        case BMS_STATE_CHARGE_AC:
        case BMS_STATE_CHARGE_CCS:
            if (!ctx->charge_request_pilot) {
                SetContactorOutputs(false, false, false);
                ctx->system_status.relay_state_mask = 0x00;
                ctx->current_state = BMS_STATE_STANDBY;
            }
            break;

        case BMS_STATE_FAULT:
            // Lock out contactors until reset
            SetContactorOutputs(false, false, false);
            ctx->system_status.relay_state_mask = 0x00;
            break;
    }

    ctx->system_status.dtc_fault_mask = ctx->active_faults;
}
