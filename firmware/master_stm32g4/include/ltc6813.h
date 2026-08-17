/**
 * @file ltc6813.h
 * @brief Driver for Analog Devices LTC6813-1 18-Channel AFE over isoSPI
 */

#ifndef LTC6813_H
#define LTC6813_H

#include "bms_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// LTC6813 Command Codes
#define LTC6813_CMD_WRCFGA      0x0001  // Write Configuration Register Group A
#define LTC6813_CMD_RDCFGA      0x0002  // Read Configuration Register Group A
#define LTC6813_CMD_WRCFGB      0x0024  // Write Configuration Register Group B
#define LTC6813_CMD_RDCFGB      0x0026  // Read Configuration Register Group B
#define LTC6813_CMD_RDCVA       0x0004  // Read Cell Voltage Register Group A (Cells 1-3)
#define LTC6813_CMD_RDCVB       0x0006  // Read Cell Voltage Register Group B (Cells 4-6)
#define LTC6813_CMD_RDCVC       0x0008  // Read Cell Voltage Register Group C (Cells 7-9)
#define LTC6813_CMD_RDCVD       0x000A  // Read Cell Voltage Register Group D (Cells 10-12)
#define LTC6813_CMD_RDCVE       0x0009  // Read Cell Voltage Register Group E (Cells 13-15)
#define LTC6813_CMD_RDCVF       0x000B  // Read Cell Voltage Register Group F (Cells 16-18)
#define LTC6813_CMD_RDAUXA      0x000C  // Read Auxiliary Register Group A (GPIO 1-3)
#define LTC6813_CMD_RDAUXB      0x000E  // Read Auxiliary Register Group B (GPIO 4-6)
#define LTC6813_CMD_RDAUXC      0x000D  // Read Auxiliary Register Group C (GPIO 7-9)
#define LTC6813_CMD_ADCV        0x0370  // Start Cell Voltage ADC (7kHz mode, all channels)
#define LTC6813_CMD_ADAX        0x0570  // Start Auxiliary ADC (7kHz mode, all GPIOs)
#define LTC6813_CMD_ADOW        0x0328  // Start Open-Wire Detection ADC
#define LTC6813_CMD_CLRCELL     0x0711  // Clear Cell Voltage Registers
#define LTC6813_CMD_CLRAUX      0x0712  // Clear Auxiliary Registers

typedef struct {
    uint16_t cell_voltages_mv[BMS_TOTAL_SERIES_CELLS];
    float    temperatures_c[BMS_TOTAL_NTC_SENSORS];
    bool     discharge_mask[BMS_TOTAL_SERIES_CELLS];
    bool     open_wire_detected[BMS_TOTAL_SERIES_CELLS];
    uint16_t min_cell_voltage_mv;
    uint16_t max_cell_voltage_mv;
    uint16_t avg_cell_voltage_mv;
    uint16_t delta_cell_voltage_mv;
    float    min_temp_c;
    float    max_temp_c;
    float    avg_temp_c;
} LTC6813_PackData_t;

// Function Prototypes
void LTC6813_Init(void);
uint16_t LTC6813_CalculatePEC15(const uint8_t *data, uint8_t len);
bool LTC6813_StartSimultaneousConversion(void);
bool LTC6813_ReadAllVoltages(LTC6813_PackData_t *pack);
bool LTC6813_ReadAllTemperatures(LTC6813_PackData_t *pack);
bool LTC6813_SetBalancingDischarge(const bool *discharge_mask);
bool LTC6813_RunOpenWireTest(LTC6813_PackData_t *pack);

#ifdef __cplusplus
}
#endif

#endif // LTC6813_H
