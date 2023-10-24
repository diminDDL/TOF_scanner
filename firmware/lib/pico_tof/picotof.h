#ifndef PICO_TOF_H
#define PICO_TOF_H

#include "pico/stdlib.h"

const uint8_t eeprom_addr = 0x50;
const uint8_t isl29501_addr = 0x57;


const uint8_t eeprom_serialnum = 0;					// S/N base address 0x00
const uint8_t eeprom_user_cal = 16;					// user calibration base address 0x10
const uint8_t eeprom_fact_cal = 32;					// factory calibration base address 0x20

const uint8_t eeprom_magic = 0xEB;
const uint8_t eeprom_serial_size = 12;
const uint8_t eeprom_calibration_offset = 0x24;    // total of 13 calibration registers, 0x24 - 0x30
												
char szMsg[400];

/* ------------------------------------------------------------ */
/*				ERROR Definitions							    */
/* ------------------------------------------------------------ */

#define ERRVAL_SUCCESS                  0       // success

#define ERRVAL_EPROM_WRITE              0xFA    // failed to write EPROM over I2C communication
#define ERRVAL_EPROM_READ               0xF9    // failed to read EPROM over I2C communication
#define ERRVAL_ToF_WRITE              	0xF8    // failed to write ISL29501 registers over I2C communication
#define ERRVAL_ToF_READ              	0xF6    // failed to read ISL29501 registers over I2C communication

#define ERRVAL_EPROM_CRC                0xFE    // wrong CRC when reading data from EPROM
#define ERRVAL_EPROM_MAGICNO            0xFD    // wrong Magic No. when reading data from EPROM
#define ERRVAL_FAILED_STARTING_CALIB    0xFC    // failed to start calibration, EEPROM or ISL29501 device is busy
#define ERRVAL_INCORRECT_CALIB_DISTACE  0xED	// incorrect calibration distance; it has to be more than 5 cm(0.05 m)
#define ERRVAL_FAILED_STARTING_MEASURE	0xEC	// failed to start measurement


/* ------------------------------------------------------------ */
/*		 Register addresses Definitions	for calibration			*/
/* ------------------------------------------------------------ */
#define CROSSTALK_I_EXPONENT 0x24
#define CROSSTALK_I_MSB 0x25
#define CROSSTALK_I_LSB 0x26
#define CROSSTALK_Q_EXPONENT 0x27
#define CROSSTALK_Q_MSB 0x28
#define CROSSTALK_Q_LSB 0x29
#define CROSSTALK_GAIN_MSB 0x2A
#define CROSSTALK_GAIN_LSB 0x2B
#define MAGNITUDE_REFERENCE_EXP 0x2C
#define MAGNITUDE_REFERENCE_MSB 0x2D
#define MAGNITUDE_REFERENCE_LSB 0x2E
#define PHASE_OFFSET_MSB 0x2F
#define PHASE_OFFSET_LSB 0x30

/* ------------------------------------------------------------ */
/*		 Register addresses Definitions	for readout			    */
/* ------------------------------------------------------------ */
#define STATUS_REGISTERS 0x02
#define DISTANCE_READOUT_MSB_REG 0xD1
#define DISTANCE_READOUT_LSB_REG 0xD2

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

typedef struct _CALIBDATA{    //
    uint8_t magic;
    uint8_t  regs[13];    // 13 registers
    uint8_t crc;
    uint8_t dummy;	// align to 16 bytes
}  __attribute__((__packed__)) CALIBDATA;


typedef struct _SERIALNODATA{    //
    uint8_t magic;
    char rgchSN[eeprom_serial_size];	// 12 chars string
    uint8_t crc;
    uint8_t dummy[2];	// align to 16 bytes
}  __attribute__((__packed__)) SERIALNODATA;


void PmodToF_Initialize();
double PmodToF_perform_distance_measurement();
uint8_t PmodToF_start_calibration(double actual_distance);
uint8_t PmodToF_WriteCalibsToEPROM_User();
uint8_t PmodToF_ReadCalibsFromEPROM_User();
uint8_t PmodToF_RestoreAllCalibsFromEPROM_Factory();
uint8_t PmodToF_ReadSerialNoFromEPROM(char *pSzSerialNo);


#endif  // PICO_TOF_H