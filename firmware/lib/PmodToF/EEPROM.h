/******************************************************************************/
/*                                                                            */
/* EEPROM.h --  EEPROM I2C driver source                                        */
/*                                                                            */
/******************************************************************************/
/* Author: Ana-Maria-Eliza Balas                                              */
/*		   ana-maria.balas@digilent.ro										  */
/* Copyright 2019, Digilent Inc.                                              */
/*                                                                            */
/******************************************************************************/
/* Module Description:                                                        */
/*                                                                            */
/* This file contains the functions declarations of EEPROM.c for              */
/* communication over I2C with the Atmel� AT24C04D EEPROM memory chip.        */
/*                                                                            */
/******************************************************************************/
/* Revision History:                                                          */
/*                                                                            */
/*    09/23/2019(anamariabalas):   Created                                    */
/*    09/23/2019(anamariabalas): Validated for Vivado 2019.1                  */
/*                                                                            */
/******************************************************************************/

#ifndef EPROM_H
#define EPROM_H


/* -------------------------------------------------------------------------- */
/*					Include Files    						                  */
/* -------------------------------------------------------------------------- */



/* ------------------------------------------------------------ */
/*		7 bit Chip Address		                                */
/* ------------------------------------------------------------ */

#define EPROM_CHIP_ADDRESS  0x50    // The LSB bit of the EPROM_CHIP_ADDRESS is the MSB
									 // of the 9-bit memory array word address

/* ------------------------------------------------------------ */
/*		EEPROM Address		                                    */
/* ------------------------------------------------------------ */
#define ADR_EPROM_SERIALNO 	0					//Serial Number  base address : 0x00h
#define ADR_EPROM_CALIB     16					//User calibration base address : 0x10
#define ADR_EPROM_FACTCALIB	32					//Factory calibration base address : 0x20




/* ------------------------------------------------------------ */
/*				Constants  						            	*/
/* ------------------------------------------------------------ */

#define bool uint8_t
#define true 1
#define false 0

/* ------------------------------------------------------------ */
/*					Procedure Declarations						*/
/* ------------------------------------------------------------ */

typedef struct Eprom{
	XIic EPROMIic;
	uint8_t chipAddr;
}EPROM;

void EPROM_begin(EPROM* InstancePtr, u32 IIC_Address, uint8_t Chip_Address);
uint8_t EPROM_PageRead(EPROM* InstancePtr, u16 addr, uint8_t* data, uint8_t cntdata);
uint8_t EPROM_PageWrite(EPROM* InstancePtr, u16 addr, uint8_t* data, uint8_t cntdata);


#endif // EPROM_H
