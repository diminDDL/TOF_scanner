#ifndef PICO_TOF_H
#define PICO_TOF_H

#include "pico/stdlib.h"
#include "eeprom.hpp"
#include "isl29501.hpp"

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

typedef struct EEPROM_DATA{
    uint8_t calib_magic;
    uint8_t calib_conf_registers[13];
    uint8_t calib_crc;
    uint8_t serial_magic;
    char serial_num[12];
    uint8_t serial_crc;
}  __attribute__((__packed__)) EEPROM_DATA;

void PmodToF_Initialize(i2c_inst_t *i2c, uint irq_pin, uint ss_pin);
double PmodToF_perform_distance_measurement();
uint8_t PmodToF_start_calibration(double actual_distance);
uint8_t PmodToF_WriteCalibsToEPROM_User();
uint8_t PmodToF_ReadCalibsFromEPROM_User();
uint8_t PmodToF_RestoreAllCalibsFromEPROM_Factory();
uint8_t PmodToF_ReadSerialNoFromEPROM(char *pSzSerialNo);


#endif  // PICO_TOF_H