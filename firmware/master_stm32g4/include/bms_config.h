/**
 * @file bms_config.h
 * @brief Global Hardware and Battery Pack Configuration for OpenEV-BMS
 * @target STM32G474RET6 Master Controller
 */

#ifndef BMS_CONFIG_H
#define BMS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                       PACK TOPOLOGY & HARDWARE MATRIX                      */
/* ========================================================================== */
#define BMS_TOTAL_SERIES_CELLS          96      // Nissan Leaf 40kWh Pack (96s2p)
#define BMS_TOTAL_SLAVE_MODULES         6       // 6x 16s Slave Boards (LTC6813-1)
#define BMS_CELLS_PER_SLAVE             16      // Cells monitored per slave board
#define BMS_NTCS_PER_SLAVE              9       // NTC thermistors per slave board
#define BMS_TOTAL_NTC_SENSORS           (BMS_TOTAL_SLAVE_MODULES * BMS_NTCS_PER_SLAVE) // 54 Sensors

/* ========================================================================== */
/*                     BATTERY CHEMISTRY: NISSAN LEAF NMC 40kWh               */
/* ========================================================================== */
#define CELL_CHEMISTRY_NAME             "NMC_Nissan_Leaf_40kWh"
#define CELL_NOMINAL_CAPACITY_AH        115.0f  // 115Ah parallel cell pair (40kWh / 355V)
#define CELL_NOMINAL_VOLTAGE_MV         3700    // 3.70V
#define CELL_MAX_VOLTAGE_MV             4200    // 4.20V Normal Full Charge
#define CELL_WARN_HIGH_VOLTAGE_MV       4180    // 4.18V Charge Taper Start
#define CELL_CRITICAL_HIGH_VOLTAGE_MV   4250    // 4.25V Emergency Trip
#define CELL_MIN_VOLTAGE_MV             3000    // 3.00V Normal Empty Cutoff
#define CELL_WARN_LOW_VOLTAGE_MV        3150    // 3.15V Power Derate Start
#define CELL_CRITICAL_LOW_VOLTAGE_MV    2800    // 2.80V Emergency Trip

/* ========================================================================== */
/*                           TEMPERATURE THRESHOLDS                           */
/* ========================================================================== */
#define CELL_TEMP_DISCHARGE_MAX_C       55.0f   // Discharge Thermal Warning
#define CELL_TEMP_DISCHARGE_CRIT_C      60.0f   // Emergency Contactor Trip
#define CELL_TEMP_CHARGE_MAX_C          45.0f   // Charge Thermal Warning
#define CELL_TEMP_CHARGE_MIN_C          0.0f    // Charge Inhibit below 0C (prevent plating)
#define CELL_TEMP_DELTA_MAX_C           8.0f    // Max allowable temperature gradient across pack

/* ========================================================================== */
/*                           PASSIVE BALANCING MATRIX                         */
/* ========================================================================== */
#define BALANCING_MIN_CELL_VOLT_MV      3800    // Only balance above 3.80V (Top-of-Charge)
#define BALANCING_VOLTAGE_DELTA_MV      10      // Balance if (V_cell - V_min) >= 10mV
#define BALANCING_MAX_CELL_TEMP_C       45.0f   // Inhibit balancing if cell exceeds 45C

/* ========================================================================== */
/*                   PRECHARGE & CONTACTOR TIMING & PWM ECONOMIZER            */
/* ========================================================================== */
#define PRECHARGE_TARGET_RATIO          0.95f   // Close Main+ when (U2 / U1) >= 95%
#define PRECHARGE_TIMEOUT_MS            1500    // Abort if precharge takes longer than 1.5s
#define CONTACTOR_PULLIN_TIME_MS        100     // 100% duty cycle for 100ms
#define CONTACTOR_HOLD_PWM_DUTY_PCT     35      // 35% duty cycle hold (25kHz PWM)

/* ========================================================================== */
/*                      MULTI-ZONE 12V 4-WIRE PWM FAN CURVES                  */
/* ========================================================================== */
#define FAN_ZONE_FRONT_BOX              0
#define FAN_ZONE_REAR_BOX               1
#define FAN_ZONE_CHARGER                2
#define FAN_TEMP_TURN_ON_C              30.0f   // 20% PWM minimum speed
#define FAN_TEMP_FULL_SPEED_C           45.0f   // 100% PWM maximum speed

/* ========================================================================== */
/*                              CAN BUS BAUD RATES                            */
/* ========================================================================== */
#define CAN1_DRIVETRAIN_BAUD            500000  // 500 kbps (ZombieVerter VCU / Inverter)
#define CAN2_IVTS_SENSOR_BAUD           500000  // 500 kbps (Isabellenhütte IVT-S 1000A)
#define CAN3_CHARGER_CCS_BAUD           500000  // 500 kbps (CCS Fast Charger / OBC)

#ifdef __cplusplus
}
#endif

#endif // BMS_CONFIG_H
