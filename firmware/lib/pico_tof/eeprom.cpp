#include "eeprom.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"

uint8_t EEPROM_Read(i2c_inst_t *i2c, uint8_t device_addr, uint8_t mem_addr, uint8_t *data, uint size);
uint8_t EEPROM_Write(i2c_inst_t *i2c, uint8_t device_addr, uint8_t mem_addr, uint8_t *data, uint size);
uint8_t EEPROM_get_device_address(uint8_t device_addr, uint16_t mem_addr);

uint8_t data_buff[8] = {0};

bool EEPROM_begin(i2c_inst_t *i2c, uint8_t addr)
{
	i2c_init(i2c, 100000);
    // try reading to check if it's correct
    uint8_t rxdata = 0;
    int ret = i2c_read_blocking(i2c, addr, &rxdata, 1, false);
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
// TODO
}

/* ------------------------------------------------------------ */
/***	EPROM_PageRead
**
**	Parameters:
**		InstancePtr - EPROM object to initialize
***		addr		- Address to send to
**		Data		- Pointer to data buffer to send
**		cntdata		- Number of data values to send
**
**	Return Value:
** uint8_t result :
**				ERRVAL_EPROM_READ	- failed to read EEPROM over I2C communication
**				ERRVAL_SUCCESS		- success
**	Description:
**		Gets the EEPROM device address and reads cntdata data bytes from the consecutive registers 
**		starting at the specified register address.
*/
// uint8_t EPROM_PageRead(EPROM* InstancePtr, u16 addr, uint8_t* data, uint8_t cntdata)
// {
// 	uint8_t result;
// 	uint8_t epromDevAddr = EPROM_GetDevAddr(InstancePtr, (addr & 0x100)>>9);
// 	XIic_SetAddress(&InstancePtr->EPROMIic, XII_ADDR_TO_SEND_TYPE, epromDevAddr);
// 	result=EPROM_ReadIIC(InstancePtr, (addr & 0xFF), data, cntdata);
// 	return result;
// }

/* ------------------------------------------------------------ */
/***	EPROM_GetDevAddr
**
**	Parameters:
**		InstancePtr - EPROM object to initialize
***		a8			- the 9th-bit of the memory array word address
**
**	Return Value:
**	uint8_t	devAddr     - EEPROM device address
**
**	Description:
**		Returns the EEPROM device address.
**
*/

uint8_t EEPROM_get_device_address(uint8_t device_addr, uint16_t mem_addr) {
    uint8_t wide_address = device_addr | (mem_addr & 0x01);
    return wide_address;
}

