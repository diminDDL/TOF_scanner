#ifndef EEPROM_H
#define EEPROM_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

static const uint8_t eeprom_addr = 0x50;

static const uint8_t eeprom_serialnum = 0;					// S/N base address 0x00
static const uint8_t eeprom_user_cal = 16;					// user calibration base address 0x10
static const uint8_t eeprom_fact_cal = 32;					// factory calibration base address 0x20

static const uint8_t eeprom_magic = 0xEB;
#define EEPROM_SERIAL_SIZE 12
static const uint8_t eeprom_serial_size = EEPROM_SERIAL_SIZE;
static const uint8_t eeprom_calibration_offset = 0x24;    // total of 13 calibration registers, 0x24 - 0x30

bool EEPROM_begin(i2c_inst_t *i2c, uint8_t device_addr);
uint8_t EEPROM_PageRead(i2c_inst_t *i2c, uint8_t device_addr, uint16_t mem_addr, uint8_t* data, uint8_t size);
uint8_t EEPROM_PageWrite(i2c_inst_t *i2c, uint8_t device_addr, uint16_t mem_addr, uint8_t* data, uint8_t size);


#endif // EEPROM_H
