
#ifndef ISL29501_HPP
#define ISL29501_HPP

#include "pico/stdlib.h"
#include "hardware/i2c.h"

static const uint8_t isl29501_addr = 0x57;


static const uint8_t isl29501_crosstalk_i_exponent = 0x24;
static const uint8_t isl29501_crosstalk_i_msb = 0x25;
static const uint8_t isl29501_crosstalk_i_lsb = 0x26;
static const uint8_t isl29501_crosstalk_q_exponent = 0x27;
static const uint8_t isl29501_crosstalk_q_msb = 0x28;
static const uint8_t isl29501_crosstalk_q_lsb = 0x29;
static const uint8_t isl29501_crosstalk_gain_msb = 0x2A;
static const uint8_t isl29501_crosstalk_gain_lsb = 0x2B;
static const uint8_t isl29501_magnitude_reference_exp = 0x2C;
static const uint8_t isl29501_magnitude_reference_msb = 0x2D;
static const uint8_t isl29501_magnitude_reference_lsb = 0x2E;
static const uint8_t isl29501_phase_offset_msb = 0x2F;
static const uint8_t isl29501_phase_offset_lsb = 0x30;


static const uint8_t isl29501_device_id = 0x00;
static const uint8_t isl29501_status_registers = 0x02;
static const uint8_t isl29501_distance_readout_msb_reg = 0xD1;
static const uint8_t isl29501_distance_readout_lsb_reg = 0xD2;




bool ISL29501_begin(i2c_inst_t *i2c, uint8_t device_addr);
uint8_t ISL29501_Read(i2c_inst_t *i2c, uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint8_t size);
uint8_t ISL29501_Write(i2c_inst_t *i2c, uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint8_t size);


#endif // ISL29501_H
