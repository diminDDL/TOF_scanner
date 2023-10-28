#ifndef PICO_TOF_H
#define PICO_TOF_H

#include "pico/stdlib.h"
#include "eeprom.hpp"
#include "isl29501.hpp"
#include <stdint.h>

static const uint8_t adr_eprom_serialno = 0;        // Serial Number base address: 0x00h
static const uint8_t adr_eprom_calib = 16;          // User calibration base address: 0x10
static const uint8_t eeprom_factcalib = 32;          // Factory calibration base address: 0x20

static const uint8_t serial_num_size = 12;
static const uint8_t calibration_address_offset_isl29501 = 0x24;    // total of 13 calibration registers, 0x24 - 0x30

typedef struct _CALIBDATA {
    uint8_t calib_magic;
    uint8_t calib_conf_registers[13];
    uint8_t calib_crc;
    uint8_t dummy; // To align to 16 bytes
} __attribute__((__packed__)) CALIBDATA;

typedef struct _SERIALNODATA {
    uint8_t serial_magic;
    char serial_num[12];
    uint8_t serial_crc;
    uint8_t dummy[2]; // To align to 16 bytes
} __attribute__((__packed__)) SERIALNODATA;

void tof_init(i2c_inst_t *i2c, uint irq_pin, uint ss_pin);
double tof_measure_distance();
uint8_t tof_calibrate(double actual_distance);
uint8_t tof_save_calibration_user();
uint8_t tof_read_calibration_user();
uint8_t tof_read_calibration_factory();
uint8_t tof_read_serial(char *serial_str);


#endif  // PICO_TOF_H