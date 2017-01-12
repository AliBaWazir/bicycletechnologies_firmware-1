/*
* Copyright (c) 2016 Shifty: Automatic Gear Selection System. All Rights Reserved.
 *
 * The code developed in this file is for the 4th year project: Shifty. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with the Shifty team.
 * Created by: Ali Ba Wazir
 * Nov 2016
 */

/**
 * @brief I2C simulation driver file.
 *
 */
 
/**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "boards.h"
#include "app_util_platform.h"
#include "nrf_delay.h"
#include "app_error.h"
#include "app_timer.h"
#define NRF_LOG_MODULE_NAME "I2C SIM_DRIVER"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "i2c_sim_driver.h"


/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/

#define BNO055_chip_id_reg    0x00

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/


/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/


/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
bool i2cSimDriver_get_BNO055_id(){
	
	return false;
	
}

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/
/**
 * @brief Function for I2C scan.
 */
static bool i2cSimDriver_scan_avail_slave_addr(const nrf_drv_twi_t* twi, bool* xfer_done, uint8_t reg_addr){
	
	bool ret= false;
	ret_code_t err_code;
	uint8_t i2c_address = 0x00;
	uint8_t working_i2c_addresses[256];
	uint8_t array_index=0;
	
	memset(working_i2c_addresses, 0x00, sizeof(working_i2c_addresses));
	
	for(i2c_address=0; i2c_address <=255; i2c_address++){
		
		*xfer_done = false;
		
		//writing to pointer byte
		err_code = nrf_drv_twi_tx(twi, (i2c_address), &reg_addr, 1, false);
		for(int i=0; i<1000;i++){
			;
		}
		
		if(*xfer_done){
			working_i2c_addresses[array_index]= i2c_address;
			array_index++;
		}
		
	}
	
	if(array_index==0){
		return false;
	} else{
		return true;
	}
}

/**
 * @brief Connection Manager App initialization.
 */
bool i2cSimDriver_init(const nrf_drv_twi_t* twi, bool* xfer_done_p){
	
	bool         ret_code         = true;
	ret_code_t   nrf_err          = NRF_SUCCESS;
	
	i2cSimDriver_scan_avail_slave_addr(twi, xfer_done_p, 0x00);
		
	return ret_code;	
}
