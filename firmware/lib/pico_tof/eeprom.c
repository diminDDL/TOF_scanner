#include "eeprom.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"

uint8_t EPROM_Read(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *data, int size);
uint8_t EPROM_Write(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *data, int size);
uint8_t EPROM_GetDevAddr(i2c_inst_t *i2c, uint8_t a8);

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

uint8_t EPROM_Read(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *data, int size) {
    // Start I2C communication
    if (!i2c_start(i2c)) {
        return ERRVAL_EPROM_READ;
    }
    
    // Write register address to read from
    if (i2c_write_timeout_us(i2c, addr, &reg, 1, true, 10000) < 1) {
        i2c_stop(i2c);
        return ERRVAL_EPROM_READ;
    }

    // Read data from EPROM
    if (i2c_read_timeout_us(i2c, addr, data, size, false, 10000) < size) {
        i2c_stop(i2c);
        return ERRVAL_EPROM_READ;
    }

    // Stop I2C communication
    i2c_stop(i2c);

    return ERRVAL_SUCCESS;
}


uint8_t EPROM_Write(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *data, int size) {
    if (size > 16) {
        size = 16;
    }

    uint8_t buffer[size + 1];
    buffer[0] = reg;
    memcpy(buffer + 1, data, size);

    int bytes_written = i2c_write_blocking(i2c, addr, buffer, size + 1, false);
    if (bytes_written != size + 1) {
        return ERRVAL_EPROM_WRITE;
    }
    return ERRVAL_SUCCESS;
}


uint8_t EPROM_Write(i2c_inst_t *i2c, uint16_t addr, uint8_t *data, uint8_t size) {
    uint8_t result = 0;
    
    // Calculate device address and memory address
    uint8_t epromDevAddr = (addr >> 8) & 0x07; // Assuming 3-bit device address
    uint8_t memAddr = addr & 0xFF;

    // Prepare data buffer with memory address + data
    uint8_t buffer[size + 1];
    buffer[0] = memAddr;
    memcpy(buffer + 1, data, size);
    
    // Write data to EEPROM
    int bytes_written = i2c_write_blocking(i2c, epromDevAddr, buffer, size + 1, false);
    if (bytes_written == size + 1) {
        result = 1; // Success
    } else {
        result = 0; // Failure
    }
    
    return result;
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
uint8_t EPROM_PageRead(EPROM* InstancePtr, u16 addr, uint8_t* data, uint8_t cntdata)
{
	uint8_t result;
	uint8_t epromDevAddr = EPROM_GetDevAddr(InstancePtr, (addr & 0x100)>>9);
	XIic_SetAddress(&InstancePtr->EPROMIic, XII_ADDR_TO_SEND_TYPE, epromDevAddr);
	result=EPROM_ReadIIC(InstancePtr, (addr & 0xFF), data, cntdata);
	return result;
}

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
uint8_t EPROM_GetDevAddr(EPROM* InstancePtr, uint8_t a8)
{
	uint8_t devAddr = InstancePtr->chipAddr|(a8 & 1);
	return devAddr;
}

