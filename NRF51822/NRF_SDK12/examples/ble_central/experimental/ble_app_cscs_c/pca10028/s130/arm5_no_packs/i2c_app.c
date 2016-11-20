/*
* Copyright (c) 2016 Shifty: Automatic Gear Selection System. All Rights Reserved.
 *
 * The code developed in this file is for the 4th year project: Shifty. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with the Shifty team.
 * Created by: Ali Ba Wazir
 * Code is based on example code of nRF SDK v.12 under /examples/peripheral/twi_sensor
 * Nov 2016
 */

/**
 * @brief BLE Connection Manager application file.
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
#define NRF_LOG_MODULE_NAME "I2C APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "i2c_app.h"
#include "i2c_sim_driver.h"


/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define I2C_IN_SIMULATION_MODE   1        /*This is set to 1 only if the I2C app is running in debugging mode 
                                           *to charecterize the I2C link between nRF module and gear controller
										   */

#define I2C_BASE_REGISTER_ADDRESS   0x1234         //address of I2C base register


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
bool i2cApp_init(void){
	
	bool         ret_code         = true;
	ret_code_t   nrf_err          = NRF_SUCCESS;
	
	if (I2C_IN_SIMULATION_MODE){
		ret_code = i2cSimDriver_init();
	}
		
	return ret_code;	
}
