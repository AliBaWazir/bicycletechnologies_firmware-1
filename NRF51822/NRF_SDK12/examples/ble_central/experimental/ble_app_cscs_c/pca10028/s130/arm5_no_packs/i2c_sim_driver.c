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
#include "nrf_drv_twi.h"
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


/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/


/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/


/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/

/**
 * @brief Connection Manager App initialization.
 */
bool i2cSimDriver_init(void){
	
	bool         ret_code         = true;
	ret_code_t   nrf_err          = NRF_SUCCESS;
	
		
	return ret_code;	
}
