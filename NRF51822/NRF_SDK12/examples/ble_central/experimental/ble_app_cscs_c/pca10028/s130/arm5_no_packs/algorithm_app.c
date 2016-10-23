/*
* Copyright (c) 2016 Shifty: Automatic Gear Selection System. All Rights Reserved.
 *
 * The code developed in this file is for the 4th year project: Shifty. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with the Shifty team.
 * Aportion of the source code here has been adopted from the Nordic Semiconductor' RSCS main application
 * Created by: Ali Ba Wazir
 * Oct 2016
 *
 */

/**
 * @brief Gear Selection Algoirthm.
 *
 */
 
 /**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include "sdk_config.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#define NRF_LOG_MODULE_NAME "ALGORITHM APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "drivers_commons.h"
#include "algorithm_app.h"


/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/


/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/
typedef enum{
	USER_DEFINED_PROPERTIES_UPDATE
} algorithmApp_event_e;

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static user_defined_properties_t user_defined_properties;  //will contain user defined data

/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
static void algorithmApp_fire_event (algorithmApp_event_e event){
	switch (event){
		case USER_DEFINED_PROPERTIES_UPDATE:
			//update user defined properties stored in ROM
			/*TODO: implement encoding function to encode user_defined_properties serially*/
		break;
		
		default:
			NRF_LOG_ERROR("algorithmApp_fire_event called with invalid event= %d\r\n",event);
		break;
	}
	
}

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/
void algorithmApp_set_cadence_setpoint(uint8_t new_cadence_setpoint){
	user_defined_properties.cadence_setpoint = new_cadence_setpoint;
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(USER_DEFINED_PROPERTIES_UPDATE);
	
	NRF_LOG_INFO("cadence_setpoint is changed to= %d .\r\n",new_cadence_setpoint);
}

void algorithmApp_set_wheel_diameter (uint8_t new_wheel_diameter){
	user_defined_properties.wheel_diameter = new_wheel_diameter;
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(USER_DEFINED_PROPERTIES_UPDATE);
	NRF_LOG_INFO("wheel_diameter is changed to= %d .\r\n",new_wheel_diameter);
}

void algorithmApp_set_gear_count (uint8_t gear_type, uint8_t new_gear_count){
	
	if (gear_type == CRANK_IDENTIFIER){
		user_defined_properties.crank_gear_count = new_gear_count;
	} else if (gear_type == WHEEL_IDENTIFIER){
		user_defined_properties.wheel_gear_count = new_gear_count;
	} else {
		NRF_LOG_ERROR("spisSimDriver_set_gear_count called with invalid gear_type= %d\r\n",gear_type);
	}
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(USER_DEFINED_PROPERTIES_UPDATE);
	NRF_LOG_INFO("gear_count is changed to= %d for gear type= %d\r\n",new_gear_count, gear_type);
}

void algorithmApp_set_teeth_count (uint8_t gear_type, uint8_t gear_index, uint8_t new_teeth_count){
	NRF_LOG_INFO("teeth_count for gear type=%d in gear index= %d is changed to count= %d.\r\n", gear_type, gear_index, new_teeth_count);
	/*TODO: */
}

bool algorithmApp_init(void){
	bool ret_code  = true;
	
	memset (&user_defined_properties, 0, sizeof(user_defined_properties_t));
	
	//decode bicycle preoperties defined by user to a static struct
	
	return ret_code;
}
