#include "eeprom.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"

uint8_t EEPROM_Read(i2c_inst_t *i2c, uint8_t device_addr, uint8_t mem_addr, uint8_t *data, uint size);
uint8_t EEPROM_Write(i2c_inst_t *i2c, uint8_t device_addr, uint8_t mem_addr, uint8_t *data, uint size);
uint8_t EEPROM_get_device_address(uint8_t device_addr, uint16_t mem_addr);

uint8_t data_buff[8] = {0};

bool EEPROM_begin(i2c_inst_t *i2c, uint8_t device_addr)
{
	i2c_init(i2c, 100000);
    // try reading to check if it's correct
    uint8_t rxdata = 0;
    int ret = i2c_read_blocking(i2c, device_addr, &rxdata, 1, false);
    if (ret < 0)
        return false;
    else
        return true;
}

uint8_t EEPROM_Read(i2c_inst_t *i2c, uint8_t device_addr, uint8_t mem_addr, uint8_t *data, uint size) {
    
    // Write register address to read from
    if (i2c_write_timeout_us(i2c, device_addr, &mem_addr, 1, true, 10000) < 1) {
        return 0;
    }

    // Read data from EPROM
    if (i2c_read_timeout_us(i2c, device_addr, data, size, false, 10000) < size) {
        return 0;
    }

    return 1;
}


// TODO test this, maybe later
uint8_t EEPROM_Write(i2c_inst_t *i2c, uint8_t device_addr, uint8_t mem_addr, uint8_t *data, uint size) {
    if (size > 16) {
        size = 16;
    }

    uint8_t buffer[17];
    buffer[0] = mem_addr;
    memcpy(buffer + 1, data, size);

    int bytes_written = i2c_write_blocking(i2c, device_addr, buffer, size + 1, false);
    if (bytes_written != size + 1) {
        return 0;
    }
    return 1;
}


uint8_t EEPROM_PageWrite(i2c_inst_t *i2c, uint8_t device_addr, uint16_t mem_addr, uint8_t* data, uint8_t size) {
    // Get the EEPROM device address based on memory address
    uint8_t new_device_address = EEPROM_get_device_address(device_addr, mem_addr);

    // Write data to EEPROM
    uint8_t result = EEPROM_Write(i2c, new_device_address, mem_addr & 0xFF, data, size);
    return result;
}

uint8_t EEPROM_PageRead(i2c_inst_t *i2c, uint8_t device_addr, uint16_t mem_addr, uint8_t* data, uint8_t size) {
    // Get the EEPROM device address based on memory address
    uint8_t new_device_address = EEPROM_get_device_address(device_addr, mem_addr);

    // Read data from EEPROM
    uint8_t result = EEPROM_Read(i2c, new_device_address, mem_addr & 0xFF, data, size);

    return result;
}


uint8_t EEPROM_get_device_address(uint8_t device_addr, uint16_t mem_addr) {
    uint8_t wide_address = (device_addr & 0xFE) | ((mem_addr >> 7) & 0x01);
    return wide_address;
}
