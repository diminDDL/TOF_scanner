#ifndef PICO_TOF_H
#define PICO_TOF_H

#include "pico/stdlib.h"
#include "eeprom.hpp"

const uint8_t isl29501_addr = 0x57;
												
//char szMsg[400];

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
    char rgchSN[EEPROM_SERIAL_SIZE];	// 12 chars string
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