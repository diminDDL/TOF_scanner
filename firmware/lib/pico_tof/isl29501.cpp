#include "isl29501.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"


bool ISL29501_begin(i2c_inst_t *i2c, uint8_t device_addr){
    i2c_init(i2c, 100000);
    // try reading to check if it's correct
    uint8_t rxdata = 0;
    int ret = i2c_read_blocking(i2c, device_addr, &rxdata, 1, false);
    if (ret < 0)
        return false;
    else
        return true;
}


uint8_t ISL29501_Read(i2c_inst_t *i2c, uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint8_t size){
    // Send register address we want to read from
    if (i2c_write_timeout_us(i2c, device_addr, &reg_addr, 1, true, 10000) < 1) {
        return 0;
    }

    // Read 'nData' bytes of data starting from the specified register
    if (i2c_read_timeout_us(i2c, device_addr, data, size, false, 10000) < size) {
        return 0;
    }

    return 1;
}

uint8_t ISL29501_Write(i2c_inst_t *i2c, uint8_t device_addr, uint8_t reg_addr, uint8_t* data, uint8_t size) {
    uint8_t buffer[256];
    buffer[0] = reg_addr;
    
    memcpy(buffer + 1, data, size);
    
    // Start I2C communication and send data
    int bytes_written = i2c_write_blocking(i2c, device_addr, buffer, size + 1, false);
    
    // Check if all bytes were written
    if (bytes_written == size + 1) {
        return 0;
    }
    return 1;
}
