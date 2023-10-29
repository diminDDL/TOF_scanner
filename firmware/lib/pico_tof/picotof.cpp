#include "picotof.hpp"
#include "pico/stdlib.h"
#include "eeprom.hpp"

#include <stdio.h>
#include "math.h"
#include "eeprom.hpp"
#include "isl29501.hpp"

CALIBDATA calib;
SERIALNODATA serialNo;

i2c_inst_t *tof_i2c;
uint tof_irq_pin;
uint tof_ss_pin;


void start_calibration_meas();
void start_magnitude_calibration();
void start_crosstalk_calibration();
void start_distance_calibration(double measured_dist);
uint8_t write_to_EEPROM_raw(uint8_t start_addr, bool skip_read);
uint8_t read_EEPROM_raw(CALIBDATA *configPtr, uint8_t start_addr, bool skip_read);
uint8_t read_isl();
uint8_t write_isl();
uint8_t write_to_EEPROM_user();



unsigned char gen_checksum(unsigned char *buff, uint size);
double bytes_to_double(uint8_t EXP, uint8_t MSB, uint8_t LSB);
double bytes_to_double(uint8_t MSB, uint8_t LSB);
void double_to_bytes(double AVG, uint8_t* EXP, uint8_t* MSB, uint8_t* LSB);
void double_to_bytes(double AVG, uint8_t* MSB, uint8_t* LSB);


void tof_init(i2c_inst_t *i2c, uint irq_pin, uint ss_pin)
{
    tof_i2c = i2c;
    tof_irq_pin = irq_pin;
    tof_ss_pin = ss_pin;

	uint8_t reg0x10_data = 0x04;
	uint8_t reg0x11_data = 0x6E;
	uint8_t reg0x13_data = 0x71;
	uint8_t reg0x60_data = 0x01;
	uint8_t reg0x18_data = 0x22;
	uint8_t reg0x19_data = 0x22;
	uint8_t reg0x90_data = 0x0F;
	uint8_t reg0x91_data = 0xFF;

	ISL29501_begin(i2c, isl29501_addr);
    EEPROM_begin(i2c, eeprom_addr);

    gpio_init(irq_pin);
    gpio_set_dir(irq_pin, GPIO_IN);
    
    gpio_init(ss_pin);
    gpio_set_dir(ss_pin, GPIO_OUT);
    gpio_put(ss_pin, 1);

    bool ret = true;

    ret &= ISL29501_Write(i2c, isl29501_addr, 0x10, &reg0x10_data, 1);
    ret &= ISL29501_Write(i2c, isl29501_addr, 0x11, &reg0x11_data, 1);
    ret &= ISL29501_Write(i2c, isl29501_addr, 0x13, &reg0x13_data, 1);
    ret &= ISL29501_Write(i2c, isl29501_addr, 0x60, &reg0x60_data, 1);
    ret &= ISL29501_Write(i2c, isl29501_addr, 0x18, &reg0x18_data, 1);
    ret &= ISL29501_Write(i2c, isl29501_addr, 0x19, &reg0x19_data, 1);
    ret &= ISL29501_Write(i2c, isl29501_addr, 0x90, &reg0x90_data, 1);
    ret &= ISL29501_Write(i2c, isl29501_addr, 0x91, &reg0x91_data, 1);

    if (ret == false)
    {
        printf("Failed to write to ISL29501\n");
    }
    else
    {
        printf("ISL29501 configured\n");
    }

	ret = tof_read_calibration_user();
    if (ret == 0)
    {
        printf("Failed to read calibration from EEPROM\n");
    }
    else
    {
        printf("Calibration read from EEPROM\n");
    }
}


double tof_measure_distance()
{
    
    uint8_t reg0x13_data = 0x7D;
    uint8_t reg0x60_data = 0x01;
    
    uint8_t unused;
    uint8_t DistanceMSB;
    uint8_t DistanceLSB;


    double distance = 1;
    bool ret = true;
    ret &= ISL29501_Write(tof_i2c, isl29501_addr, 0x13, &reg0x13_data, 1);
    ret &= ISL29501_Write(tof_i2c, isl29501_addr, 0x60, &reg0x60_data, 1);
    ret &= ISL29501_Read(tof_i2c, isl29501_addr, 0x69, &unused, 1);
    start_calibration_meas();
    uint32_t start_time = time_us_32();
    // wait for IRQ
    while(gpio_get(tof_irq_pin) != 0 | (time_us_32() - start_time) > 100000);
    ret &= ISL29501_Read(tof_i2c, isl29501_addr, 0xD1, &DistanceMSB, 1);
    ret &= ISL29501_Read(tof_i2c, isl29501_addr, 0xD2, &DistanceLSB, 1);

    distance =(((double)DistanceMSB * 256 + (double)DistanceLSB)/65536) * 33.31;
    return  distance;
}

uint8_t tof_read_calibration_user()
{
    return read_EEPROM_raw(&calib, adr_eprom_calib, false);

}

uint8_t read_EEPROM_raw(CALIBDATA *configPtr, uint8_t start_addr, bool skip_read)
{
    uint8_t bCrc, bCrcRead,result;
    // read calibration structure
    result = EEPROM_PageRead(tof_i2c, eeprom_addr, start_addr, (uint8_t *)configPtr, sizeof(CALIBDATA));
    
    if(result != 1)
    {
        return result;
    }
    // check CRC
    bCrcRead = configPtr->calib_crc;
    configPtr->calib_crc = 0;

    bCrc = gen_checksum((uint8_t *)configPtr, sizeof(CALIBDATA));

    configPtr->calib_crc = bCrcRead;

    if(configPtr->calib_magic != eeprom_magic)
    {
        // missing magic number
        return 0;
    }

    if(configPtr->calib_crc != bCrc)
    {
        // CRC error
        return 0;
    }
    if (skip_read == false)
    	write_isl();

    return 1;
}


uint8_t write_isl()
{
	uint8_t result = 1;
    for(int i = 0; i < 13; i++)
    {
        result = ISL29501_Write(tof_i2c, isl29501_addr, calibration_address_offset_isl29501 + i, calib.calib_conf_registers + i, 1);
        if(result != 1)
        return result;
    }
    return result;
}


void start_calibration_meas()
{
    gpio_put(tof_ss_pin, 0);
    sleep_us(5600);
    gpio_put(tof_ss_pin, 1);
    sleep_us(14400);
}

// /* ------------------------------------------------------------ */
// /** uint8_t PmodToF_start_calibration(double actual_distance)
// **  Parameters:
// **      actual_distance - the actual measuring distance (in meters) corresponding to the calibration setup. This distance must be larger than 0.05m.
// **
// **  Return Value:
// **      ERRVAL_INCORRRECT_CALIB_DISTACE 	0xED	// incorrect calibration distance; it has to be more than 5 cm(0.05 m)
// **		ERRVAL_FAILED_STARTING_CALIB		0xFC    // failed to start calibration, EEPROM or ISL29501 device is busy
// **
// **  Description:
// ** Function for performing a manual calibration of the device, for the actual distance provided as parameter. 
// ** It calls the function implementing the required  calibration sequence: CALIB_perform_magnitude_calibration, 
// ** CALIB_perform_crosstalk_calibration, CALIB_perform_distance_calibration, as described in the Firmware Routines documentation(an1724.pdf)
// ** The function returns ERRVAL_INCORRECT_CALIB_DISTACE if the provided parameter is not larger than 0.05.
// ** The function returns ERRVAL_FAILED_STARTING_CALIB if the calibration cannot be started (at least one of ISL29501 or EPROM are busy).
// **/
// uint8_t PmodToF_start_calibration(double actual_distance)
// {
// 	uint8_t bErrCode = ERRVAL_SUCCESS;
// 	if (actual_distance == 0 || actual_distance < 0.05)
// 	{
// 		bErrCode = ERRVAL_INCORRECT_CALIB_DISTACE;
// 		return bErrCode;
// 	}
// 	else
// 	{
// 		xil_printf("OK,Starting calibration...\r\n");
// 		sleep(2);
// 		//verifying that the ISL29501 and EEPROM device is not busy
// 		if(!XIic_IsIicBusy(&myToFDevice.ISL29501Iic) && !XIic_IsIicBusy(&myEPROMDevice.EPROMIic))
// 		{
// 			xil_printf("Starting magnitude calibration... You have 5 sec to prepare the device\r\n");
// 			sleep(5);
// 			CALIB_perform_magnitude_calibration();

// 			xil_printf("Starting crosstalk calibration... You have 10 sec to prepare the device \r\n");
// 			sleep(10);
// 			CALIB_perform_crosstalk_calibration();

// 			xil_printf("Starting distance calibration...  You have 10 sec to prepare the device \r\n");
// 			sleep(10);
// 			CALIB_perform_distance_calibration(actual_distance);
// 			sprintf(szMsg,"Calibration done.\r\n");
// 			return bErrCode;
// 		}
// 		else
// 		{
// 			bErrCode = ERRVAL_FAILED_STARTING_CALIB;
// 			return bErrCode;
// 		}
// 	}
// }


// /* ------------------------------------------------------------ */
// /** CALIB_perform_magnitude_calibration()
// **  Parameters:
// **      none
// **  Return Value:
// **      none
// **
// **  Description:
// **  Function for performing magnitude calibration. It implements the steps described in the Firmware Routines documentation(an1724.pdf).
// **  It is called by PmodToF_start_calibration function.
// **/
// void CALIB_perform_magnitude_calibration()
// {
//     /* WRITE REG */
//     uint8_t reg0x13_data = 0x61;
//     uint8_t reg0x60_data = 0x01;
//     /* READ REG */
//     uint8_t regs[3];
//     uint8_t unused;

//     ISL29501_WriteIIC(&myToFDevice, 0x13, &reg0x13_data, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x60, &reg0x60_data, 1);
//     ISL29501_ReadIIC(&myToFDevice, 0x69, &unused, 1);
//     CALIB_initiate_calibration_measurement();
// 	//waits for IRQ
//     while((XGpio_DiscreteRead(&gpio, GPIO_CHANNEL) & GPIO_DATA_RDY_MSK) != 0 );
//     ISL29501_ReadIIC(&myToFDevice, 0xF6, regs, 3);
//     ISL29501_WriteIIC(&myToFDevice, 0x2C, regs, 3);
//     reg0x13_data= 0x7D;
//     ISL29501_WriteIIC(&myToFDevice, 0x13, &reg0x13_data, 1);
//     reg0x60_data=0x00;
//     ISL29501_WriteIIC(&myToFDevice, 0x60, &reg0x60_data, 1);
// }

// /* ------------------------------------------------------------ */
// /** CALIB_perform_crosstalk_calibration()
// **  Parameters:
// **      none
// **  Return Value:
// **      none
// **
// **  Description:
// **  Function for performing crosstalk calibration.
// **  It implements the steps described in the Firmware Routines documentation(an1724.pdf).
// **  It is called by PmodToF_start_calibration function.
// **/
// void CALIB_perform_crosstalk_calibration()
// {
//     int N= 100;
//     /* WRITE REG */
//     uint8_t reg0x13_data = 0x7D;
//     uint8_t reg0x60_data = 0x01;
//     /* READ REG */
//     uint8_t regs[14];
//     uint8_t unused;
//     uint8_t i_exp_calib;
//     uint8_t i_msb_calib;
//     uint8_t i_lsb_calib;
//     double i_sum=0;
//     double i_avg;
//     uint8_t q_exp_calib;
//     uint8_t q_msb_calib;
//     uint8_t q_lsb_calib;
//     double q_sum=0;
//     double q_avg;
//     uint8_t gain_msb_calib;
//     uint8_t gain_lsb_calib;
//     double gain_sum=0;
//     double gain_avg;


//     ISL29501_WriteIIC(&myToFDevice, 0x13, &reg0x13_data, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x60, &reg0x60_data, 1);

//     for(int i=0; i < N;i++)
//     {
//         ISL29501_ReadIIC(&myToFDevice, 0x69, &unused, 1);
//         CALIB_initiate_calibration_measurement();
// 		//waits for IRQ
//         while((XGpio_DiscreteRead(&gpio, GPIO_CHANNEL) & GPIO_DATA_RDY_MSK) != 0 );
//         ISL29501_ReadIIC(&myToFDevice, 0xDA, regs, 14);
//         double I = _3bytes_to_double(regs[0],regs[1],regs[2]);
//         i_sum += I;
//         double Q = _3bytes_to_double(regs[3],regs[4],regs[5]);
//         q_sum += Q;
//         double gain = _2bytes_to_double(regs[12],regs[13]);
//         gain_sum += gain;

//     }
//     i_avg= i_sum / N;
//     q_avg= q_sum / N;
//     gain_avg= gain_sum / N;

//     double_to_3bytes(i_avg, &i_exp_calib, &i_msb_calib, &i_lsb_calib);
//     double_to_3bytes(q_avg, &q_exp_calib, &q_msb_calib, &q_lsb_calib);
//     double_to_2bytes(gain_avg, &gain_msb_calib, &gain_lsb_calib);
//     ISL29501_WriteIIC(&myToFDevice, 0x24, &i_exp_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x25, &i_msb_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x26, &i_lsb_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x27, &q_exp_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x28, &q_msb_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x29, &q_lsb_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x2A, &gain_msb_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x2B, &gain_lsb_calib, 1);

// }


// /* ------------------------------------------------------------ */
// /** void CALIB_perform_distance_calibration(double actual_dist)
// **  Parameters:
// **      actual_distance - the actual measuring distance (in meters) corresponding to the calibration setup.
// **  Return Value:
// **      none
// **
// **  Description:
// **  Function for performing distance calibration.
// **  It implements the steps described in the Firmware Routines documentation(an1724.pdf).
// **  It is called by PmodToF_start_calibration function.
// **/
// void CALIB_perform_distance_calibration(double actual_dist)
// {
//     int N= 100;
//     /* WRITE REG */
//     uint8_t reg0x13_data = 0x7D;
//     uint8_t reg0x60_data = 0x01;


//     /* READ REG */
//     uint8_t regs[2];
//     uint8_t unused;
//     double phase_sum=0;
//     double phase_avg;
//     double dist_calib;
//     uint8_t dist_msb_calib;
//     uint8_t dist_lsb_calib;


//     ISL29501_WriteIIC(&myToFDevice, 0x13, &reg0x13_data, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x60, &reg0x60_data, 1);
//     for(int i=0; i < N;i++)
//     {
//         ISL29501_ReadIIC(&myToFDevice, 0x69, &unused, 1);
//         CALIB_initiate_calibration_measurement();
// 		//waits for IRQ
//         while((XGpio_DiscreteRead(&gpio, GPIO_CHANNEL) & GPIO_DATA_RDY_MSK) != 0 );
//         ISL29501_ReadIIC(&myToFDevice, 0xD8, regs, 2);
//         double phase = _2bytes_to_double(regs[0],regs[1]);
//         phase_sum += phase;

//     }
//     phase_avg= phase_sum / N;
//     dist_calib = phase_avg - (actual_dist/33.31*65536);
//     double_to_2bytes(dist_calib, &dist_msb_calib, &dist_lsb_calib);
//     ISL29501_WriteIIC(&myToFDevice, 0x2F, &dist_msb_calib, 1);
//     ISL29501_WriteIIC(&myToFDevice, 0x30, &dist_lsb_calib, 1);

// }


// /***    CALIB_WriteCalibsToEPROM_User(uint8_t msg_flag)
// **
// **  Parameters:
// **      msg_flag - flag that enables display over UART the notification message
// **
// **  Return Value:
// **      uint8_t ERRVAL_SUCCESS          0
// **				ERRVAL_EPROM_WRITE		0xFA
// **				ERRVAL_ToF_READ			0xF6
// **
// **  Description:
// **      This local function writes calibration data in the user calibration area of EPROM. It calls CALIB_WriteCalibsToEPROM_Raw providing
// **      user calibration area EPROM address as parameter. 
// **		It returns ERRVAL_SUCCESS for success or the error codes provided by CALIB_WriteCalibsToEPROM_Raw function.
// */
// uint8_t CALIB_WriteCalibsToEPROM_User(uint8_t msg_flag)
// {
//     uint8_t bErrCode;
//     bErrCode = CALIB_WriteCalibsToEPROM_Raw((uint8_t)ADR_EPROM_CALIB, 0);  // write calibration to EPROM
//     if(bErrCode == ERRVAL_SUCCESS)
//     {
//     	if(msg_flag == 1)
//     	{
// 			sprintf(szMsg,"Calibration stored to EEPROM user space.");

//     	}
//     	return bErrCode;
//     }
//     else
//     {

// 		return bErrCode;
//     }
// }

// /***    PmodToF_WriteCalibsToEPROM_User
// **
// **  Parameters:
// **     none
// **
// **  Return Value:
// **      uint8_t ERRVAL_SUCCESS          0
// **				ERRVAL_EPROM_WRITE		0xFA
// **				ERRVAL_ToF_READ			0xF6
// **
// **  Description:
// **      This function writes calibration data in the user calibration area of EEPROM.
// **      It calls the local function CALIB_WriteCalibsToEPROM_User.
// **      This function should be called after changes were made in calibration data(after a manual calibration),
// **      in order to save them in the non-volatile memory.
// **		It returns ERRVAL_SUCCESS for success or the error codes provided by CALIB_WriteCalibsToEPROM_Raw function.
// **
// */
// uint8_t PmodToF_WriteCalibsToEPROM_User()
// {
//     return CALIB_WriteCalibsToEPROM_User(1);
// }

// /***    PmodToF_RestoreAllCalibsFromEPROM_Factory
// **
// **  Parameters:
// **      none
// **
// **  Return Value:
// **      uint8_t
// **          ERRVAL_SUCCESS                  0       // success
// **          ERRVAL_EPROM_MAGICNO            0xFD    // wrong magic number when reading data from EEPROM.
// **          ERRVAL_EPROM_CRC                0xFE    // wrong checksum when reading data from EPROM
// **          ERRVAL_EPROM_WRITE				0xFA    // failed to write EPROM over I2C communication
// **          ERRVAL_ToF_READ					0xF6    // failed to read ISL29501 registers over I2C communication
// **
// **  Description:
// **      This function restores the factory calibration data from EPROM.
// **      This function reads factory calibration data from EPROM using CALIB_ReadCalibsFromEPROM_Raw local function 
// **      and writes the calibration data into the user calibration area of EPROM using CALIB_WriteCalibsToEPROM_User local function.
// **      As the "skip_write_regs" parameter from CALIB_ReadCalibsFromEPROM_Raw function is 0, calibration data will be written
// **      into ToF registers. If "skip_write_regs"= 1, calibration data will not be written into ToF registers.
// **      The function returns ERRVAL_SUCCESS for success or the error codes provided by  
// **		CALIB_ReadCalibsFromEPROM_Raw and CALIB_WriteCalibsToEPROM_Raw.
// **
// */
// uint8_t PmodToF_RestoreAllCalibsFromEPROM_Factory()
// {
//     uint8_t bErrCode;
//     bErrCode = CALIB_ReadCalibsFromEPROM_Raw(&calib, (uint8_t)ADR_EPROM_FACTCALIB, 0);
//     if(bErrCode == ERRVAL_SUCCESS)
//     {
//     	CALIB_WriteCalibsToEPROM_User(0);
//         sprintf(szMsg, "Factory calibration restored.");
//         return bErrCode;
//     }
//     else
//     {
// 		return bErrCode;
//     }
// }

// /***    PmodToF_ReadSerialNoFromEPROM
// **
// **  Parameters:
// **      char *pSzSerialNo - the pointer used to store the retrieved serial number.
// **
// **  Return Value:
// **      uint8_t
// **          ERRVAL_SUCCESS              0       // success
// **          ERRVAL_EPROM_CRC            0xFE    // wrong CRC when reading data from EPROM
// **          ERRVAL_EPROM_MAGICNO        0xFD    // wrong Magic No. when reading data from EPROM
// **
// **  Description:
// **      This function reads the serial number data from EPROM and stores the data in the serialNo global data structure.
// **		It copies the 12 serial number characters into the pSzSerialNo parameter.
// **      The function returns ERRVAL_SUCCESS for success.
// **      The function returns ERRVAL_EPROM_MAGICNO when a wrong magic number was detected in the data read from EPROM.
// **      The function returns ERRVAL_EPROM_CRC when the checksum is wrong for the data read from EEPROM.
// **
// */
// uint8_t PmodToF_ReadSerialNoFromEPROM(char *pSzSerialNo)
// {
//     uint8_t bCrc, bCrcRead=0;
//     // read calibration structure
//     EPROM_PageRead(&myEPROMDevice, ADR_EPROM_SERIALNO, (uint8_t *)&serialNo, sizeof(SERIALNODATA));


//     // check CRC
//     bCrcRead = serialNo.crc;

//     serialNo.crc = 0;

//     bCrc = GetBufferChecksum((uint8_t *)&serialNo, sizeof(SERIALNODATA));

//     serialNo.crc = bCrcRead;

//     if(serialNo.magic != EPROM_MAGIC_NO)
//     {
//         // missing magic number
//         return ERRVAL_EPROM_MAGICNO;
//     }

//     if(serialNo.crc != bCrc)
//     {
//         // CRC error
//         return ERRVAL_EPROM_CRC;
//     }
//     strncpy(pSzSerialNo, serialNo.rgchSN, SERIALNO_SIZE);   // copy 12 chars of serial number from serialNo to the destination string
//     pSzSerialNo[SERIALNO_SIZE] = 0; // terminate string
//     return ERRVAL_SUCCESS;
// }

// /***    CALIB_WriteAllCalibsToEPROM_Raw
// **
// **  Parameters:
// **      uint8_t baseAddr        - the address where the calibration data  will be written in EPROM
// **		uint8_t skip_read_regs - Enables reading current data from 13 ToF calibration registers into the calib global structure
// **
// **  Return Value:
// **      uint8_t
// **          ERRVAL_SUCCESS                  
// **			ERRVAL_EPROM_WRITE
// **			ERRVAL_ToF_READ
// **
// **
// **  Description:
// **      This local function writes calibration data to a specific location in EPROM. 
// **      Depending on skip_read_regs it reads current data from 13 ToF calibration registers into the calib global structure using ISL29501_ReadIIC function.
// **      The 16 bytes payload to be written to EPROM also stores a signature byte called magic number (0xEB), one dummy byte 
// **      and the checksum of the active payload (previous 15 bytes).
// **      The function calls EPROM_PageWrite in order to write the 16 bytes to EPROM address specified in baseAddr parameter.
// **      This function is called by PmodToF_WriteAllCalibsToEPROM_User, which provides user calibration area EPROM address as parameter.
// **      This function shouldn't be called by user, instead, the user should call PmodToF_WriteAllCalibsToEPROM_User.
// **      The function returns ERRVAL_SUCCESS for success or the error codes provided by ISL29501_ReadIIC and EPROM_PageWrite functions.
// **
// */
// uint8_t CALIB_WriteCalibsToEPROM_Raw(uint8_t baseAddr, uint8_t skip_read_regs)
// {
//     uint8_t bResult;
//     // fill the calib register values
//     if (skip_read_regs == 0 )
//     	bResult=CALIB_Read_ISL29501_Regs();
//     if(bResult!= ERRVAL_SUCCESS)
//     	return bResult;

//     calib.magic = EPROM_MAGIC_NO;
//     calib.dummy = 0;
//     calib.crc = 0;  // neutral value for the checksum
//     calib.crc = GetBufferChecksum((uint8_t *)&calib, sizeof(calib));
//     bResult = EPROM_PageWrite(&myEPROMDevice, baseAddr, (uint8_t *)&calib, sizeof(calib));
//     usleep(10000);   // 10 ms
//     return bResult;
// }

// /***    CALIB_Read_ISL29501_Regs
// **
// **  Parameters:
// **			none
// **
// **  Return Value:
// **	uint8_t		ERRVAL_SUCCESS      0       // success 
// **	uint8_t		ERRVAL_ToF_READ		0xF6    // failed to read ISL29501 registers over I2C communication
// **  Description:
// **      This local function reads calibration data from the 13 ISL29501 calibration registers starting at 0x24 address 
// **		into the calib global structure, using the ISL29501_ReadIIC function.
// **      The function returns ERRVAL_SUCCESS for success or the error codes provided by ISL29501_ReadIIC function.
// **
// */
// uint8_t CALIB_Read_ISL29501_Regs()
// {
// 	 uint8_t result;
//      result = ISL29501_ReadIIC(&myToFDevice, ADR_OFFSET_CALIB_REG_ISL29501, calib.regs, 13);
//      return result;
// }

// /***
// ** _3bytes_to_double(uint8_t exp, uint8_t msb, uint8_t lsb)
// **
// **  Parameters:
// **      exp - the exponent for Double-precision binary floating-point format
// **      msb - most significant byte for Double-precision binary floating-point format
// **		lsb - least significant byte for Double-precision binary floating-point format
// **  Return Values:
// **      Double number
// **  Description:
// **      This function converts 3 bytes into double format number.
// **      Returns a double format number.
// **
// */
// double _3bytes_to_double(uint8_t exp, uint8_t msb, uint8_t lsb)
// {
//     double result = 0;
//     int iMantissa = 0;
//     // flag for negative numbers
//     uint8_t negative = 0u;

//     // negative number
//     if (msb > 127)
//         negative = 1u;

//     iMantissa = msb << 8;
//     iMantissa |= lsb;
//     if (negative) {
//         // convert from 2's complement
//         iMantissa = ((iMantissa - 1) ^ 0xFFFF);
//         // combine mantissa and exponent
//         result = -iMantissa * pow(2, exp);
//     } else {
//         result = iMantissa * pow(2, exp);
//     }

//     return result;
// }
// /***
// ** _2bytes_to_double(uint8_t msb, uint8_t lsb)
// **
// **  Parameters:
// **      msb - most significant byte
// **		lsb - least significant byte
// **  Return Values:
// **      Double number
// **
// **  Description:
// **      This function converts 2 bytes into double format number.
// **      Returns a double format number.
// **
// */
// double _2bytes_to_double(uint8_t msb, uint8_t lsb)
// {
//     return (double)(unsigned short)(((int)(msb) << 8) | (int)(lsb));
// }
// /***
// ** double_to_3bytes(double dNum, uint8_t* exp, uint8_t* msb, uint8_t* lsb)
// **
// **  Parameters:
// **  	dNum - double number which will be converted to 3 bytes
// **      exp - the exponent for Double-precision binary floating-point format
// **      msb - most significant byte for Double-precision binary floating-point format
// **		lsb - least significant byte for Double-precision binary floating-point format
// **  Return Values:
// **		none
// **  Description:
// **      This function converts double number to 3 bytes.
// **
// */
// void double_to_3bytes(double dNum, uint8_t* exp, uint8_t* msb, uint8_t* lsb)
// {
//     double dNumLog = 0;
//     double dMantissa = 0;
//     int iMantissa = 0;
//     uint8_t negative = 0u;
//     uint8_t first_exp, new_exp = 0;
//     uint8_t a;

//     // handle negative numbers
//     if (dNum < 0) {
//         // set negative flag
//         negative = 1u;
//         // convert to positive
//         dNum = fabs(dNum);
//     }

//     // log base 2 of input
//     dNumLog = log2(dNum);
//     // exponent of the double
//     first_exp = (uint8_t) dNumLog;
//     // log of mantissa
//     dMantissa = (dNumLog - (double) first_exp);
//     // convert mantissa to double
//     dMantissa = pow(2, dMantissa);
//     // start new exponent as the original exponent
//     new_exp = first_exp;

//     // it might seem like 15 shifts is the correct number but it's 14.
//     // Doing 15 shifts into the sign bit making it a negative number.
//     // convert mantissa to whole number
//     for (a = 1; a <= first_exp && a < 15; ++a) {
//         // double the mantissa
//         dMantissa = dMantissa * 2;
//         // decrement the exponent
//         --new_exp;
//     }

//     if (negative) {
//         // take 2's complement, convert to short
//         iMantissa = (int) (-dMantissa);
//     } else {
//         // convert to short
//         iMantissa = (int) (dMantissa);
//     }

//     *exp = new_exp;
//     *msb = (uint8_t) ((iMantissa & 0xFF00) >> 8);
//     *lsb = (uint8_t) (iMantissa & 0x00FF);
// }
// /***
// ** double_to_2bytes(double dNum, uint8_t* msb, uint8_t* lsb)
// **
// **  Parameters:
// **  	dNum - double number which will be converted to 3 bytes
// **      msb - most significant byte for Double-precision binary floating-point format
// **		lsb - least significant byte for Double-precision binary floating-point format
// **  Return Values:
// **		none
// **
// **  Description:
// **      This function converts double number to 2 bytes.
// **
// */
// void double_to_2bytes(double dNum, uint8_t* msb, uint8_t* lsb)
// {
//     int iNum = (int) dNum;
//     *msb = (uint8_t) ((iNum & 0x0000FF00) >> 8);
//     *lsb = (uint8_t) (iNum & 0x000000FF);
// }


unsigned char gen_checksum(unsigned char *buff, uint size){
    int i;
    unsigned char checksum = 0;
    for(i= 0; i < size; i++)
    {
        checksum += buff[i];
    }
    return checksum;
}