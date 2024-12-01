#pragma once

#include <Arduino.h>
#include <config.h>
#include <ModbusMaster.h>

//extern ModbusMaster node;

void preTransmission();

void postTransmission();

void setupModbus();

uint8_t readInputRegister(uint16_t addr);
uint8_t readInputRegisters(uint16_t addr, uint8_t qty);

uint8_t readHoldingRegister(uint16_t addr);
uint8_t readHoldingRegisters(uint16_t addr, uint8_t qty);

uint8_t writeMultipleRegisters(uint16_t u16WriteAddress, uint16_t u16WriteQty);

uint16_t getResponseBuffer(uint8_t u8Index);
void clearTransmitBuffer();
uint8_t setTransmitBuffer(uint8_t u8Index, uint16_t u16Value);

void readLiveData(uint8_t* payload);
void readStatsData(uint8_t* payload);
void readRTC(uint8_t* payload);
void writeRTC(uint8_t* payload);

// ModBus Register Locations
//
#define LIVE_DATA       0x3100     // start of live-data 
#define LIVE_DATA_CNT   16         // 16 regs

// just for reference, not used in code
#define PANEL_VOLTS     0x00
#define PANEL_AMPS      0x01
#define PANEL_POWER_L   0x02
#define PANEL_POWER_H   0x03

#define BATT_VOLTS      0x04
#define BATT_AMPS       0x05
#define BATT_POWER_L    0x06
#define BATT_POWER_H    0x07

// UNUSED 0x08-0x0B

#define LOAD_VOLTS      0x0C
#define LOAD_AMPS       0x0D
#define LOAD_POWER_L    0x0E
#define LOAD_POWER_H    0x0F
/*
#define BATTERY_TEMP    0x10*/
#define CASE_TEMP       0x11
#define HEATSINK_TEMP   0x12
#define BATTERY_SOC     0x1A
/*#define BATTERY_TEMP_REMOTE  0x1B
#define BATTERY_RATED   0x1D

#define CCMODEL_           0x2b
*/
#define RTC_CLOCK           0x9013  // D7-0 Sec, D15-8 Min  :   D7-0 Hour, D15-8 Day  :  D7-0 Month, D15-8 Year
#define RTC_CLOCK_CNT       3       // 3 regs

#define BATTERY_CURRENT_L   0x331B  // Battery current L
#define BATTERY_CURRENT_H   0x331C  // Battery current H


#define STATISTICS      0x3300 // start of statistical data
#define STATISTICS_CNT  22     // 22 regs

// just for reference, not used in code
#define PV_MAX     0x00 // Maximum input volt (PV) today
#define PV_MIN     0x01 // Minimum input volt (PV) today
#define BATT_MAX   0x02 // Maximum battery volt today
#define BATT_MIN   0x03 // Minimum battery volt today

#define CONS_ENERGY_DAY_L   0x04 // Consumed energy today L
#define CONS_ENGERY_DAY_H   0x05 // Consumed energy today H
#define CONS_ENGERY_MON_L   0x06 // Consumed energy this month L
#define CONS_ENGERY_MON_H   0x07 // Consumed energy this month H
#define CONS_ENGERY_YEAR_L  0x08 // Consumed energy this year L
#define CONS_ENGERY_YEAR_H  0x09 // Consumed energy this year H
#define CONS_ENGERY_TOT_L   0x0A // Total consumed energy L
#define CONS_ENGERY_TOT_H   0x0B // Total consumed energy  H

#define GEN_ENERGY_DAY_L   0x0C // Generated energy today L
#define GEN_ENERGY_DAY_H   0x0D // Generated energy today H
#define GEN_ENERGY_MON_L   0x0E // Generated energy this month L
#define GEN_ENERGY_MON_H   0x0F // Generated energy this month H
#define GEN_ENERGY_YEAR_L  0x10 // Generated energy this year L
#define GEN_ENERGY_YEAR_H  0x11 // Generated energy this year H
#define GEN_ENERGY_TOT_L   0x12 // Total generated energy L
#define GEN_ENERGY_TOT_H   0x13 // Total Generated energy  H
/*
#define CO2_REDUCTION_L    0x14 // Carbon dioxide reduction L
#define CO2_REDUCTION_H    0x15 // Carbon dioxide reduction H

#define BAT_CUR_L 0x1B
#define BAT_CUR_H 0x1C
#define BAT_TEMP 0x1D
#define AMBIENT_TEMP 0x1E

#define LOAD_STATE         0x02 // r/w load switch state
*/
#define STATUS_FLAGS    0x3200
#define STATUS_BATTERY    0x00  // Battery status register
#define STATUS_CHARGER    0x01  // Charging equipment status register
#define STATUS_DISCHARGER 0x02  // Discharging equipment status register