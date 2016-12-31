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
static user_defined_properties_t  user_defined_properties;  //will contain user defined data

static bool  gear_level_locked = false;                     /*TODO: this boolean has to be checked before running the gear shifting algorithm
															 *If set to true, lock gear level at the middle to maximize efficiency.  
															 */

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
			NRF_LOG_ERROR("algorithmApp_fire_event: called with invalid event= %d\r\n",event);
		break;
	}
	
}

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/

/************************* GETTERS ****************************/
uint8_t algorithmApp_get_cadence_setpoint(void){
	return (uint8_t)user_defined_properties.cadence_setpoint_rpm;
}

uint8_t algorithmApp_get_wheel_diameter_cm(void){
	return (uint8_t)user_defined_properties.wheel_diameter_cm;
}

uint8_t algorithmApp_get_gears_count_crank(void){
	return (uint8_t)user_defined_properties.crank_gears_count;
}

uint8_t algorithmApp_get_gears_count_wheel(void){
	return (uint8_t)user_defined_properties.wheel_gears_count;
}

uint8_t algorithmApp_get_teeth_count(uint8_t gear_type, uint8_t gear_index){
	
	if (gear_index >= MAX_GEARS_COUNT){
		NRF_LOG_WARNING("algorithmApp_get_teeth_count: called with grear index=%d higher than max gear count\r\n", gear_index);
		return 0;
	} else{
		if(gear_type == CRANK_IDENTIFIER){
			return (uint8_t)user_defined_properties.crank_gears_teeth_count[gear_index];
		} else if (gear_type == WHEEL_IDENTIFIER){
			return (uint8_t)user_defined_properties.wheel_gears_teeth_count[gear_index];
		}else{
			NRF_LOG_ERROR("algorithmApp_get_teeth_count: called with unknown gear_type=0x%x\r\n", gear_type);
			return 0;
		}
	}
}

/************************* SETTERS ****************************/
void algorithmApp_set_cadence_setpoint(uint8_t new_cadence_setpoint){
	user_defined_properties.cadence_setpoint_rpm = new_cadence_setpoint;
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(USER_DEFINED_PROPERTIES_UPDATE);
	
	NRF_LOG_INFO("algorithmApp_set_cadence_setpoint: cadence_setpoint is changed to= %d .\r\n",new_cadence_setpoint);
}

void algorithmApp_set_gear_level_locked(){
	gear_level_locked = true;
	NRF_LOG_INFO("algorithmApp_set_gear_level_locked: gear level is locked= %d .\r\n");

}

void algorithmApp_set_wheel_diameter (uint8_t new_wheel_diameter){
	user_defined_properties.wheel_diameter_cm = new_wheel_diameter;
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(USER_DEFINED_PROPERTIES_UPDATE);
	NRF_LOG_INFO("algorithmApp_set_wheel_diameter: wheel_diameter is changed to= %d .\r\n",new_wheel_diameter);
}

bool algorithmApp_set_gear_count(uint8_t crank_gears_count, uint8_t wheel_gears_count){
	
	if ((crank_gears_count > MAX_GEARS_COUNT) || (wheel_gears_count > MAX_GEARS_COUNT)){
		NRF_LOG_WARNING("algorithmApp_set_gear_count: crank_gears_count= %d or wheel_gears_count= %d is greater than MAX_GEARS_COUNT.\r\n",crank_gears_count, wheel_gears_count);
		return false;
	} else{
		user_defined_properties.crank_gears_count= crank_gears_count;
		user_defined_properties.wheel_gears_count= wheel_gears_count;
	}
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(USER_DEFINED_PROPERTIES_UPDATE);
	NRF_LOG_INFO("gear_count is changed to crank_gears_count= %d, wheel_gears_count= %d\r\n",crank_gears_count, wheel_gears_count);
	return true;
}

void algorithmApp_set_teeth_count (uint8_t gear_type, uint8_t gear_index, uint8_t new_teeth_count){
	
	if (gear_type == CRANK_IDENTIFIER){
		user_defined_properties.crank_gears_teeth_count[gear_index] = new_teeth_count;
	} else if (gear_type == WHEEL_IDENTIFIER){
		user_defined_properties.wheel_gears_teeth_count[gear_index] = new_teeth_count;
	} else {
		NRF_LOG_ERROR("algorithmApp_set_teeth_count: called with invalid gear_type= %d\r\n",gear_type);
	}
	
    //fire update event to encode updated data in ROM
	algorithmApp_fire_event(USER_DEFINED_PROPERTIES_UPDATE);
    NRF_LOG_INFO("algorithmApp_set_teeth_count: teeth_count for gear type=%d in gear index[%d] is updated to count= %d.\r\n", gear_type, gear_index, new_teeth_count);
	
}

bool algorithmApp_init(void){
	bool ret_code  = true;
	
	memset (&user_defined_properties, 0, sizeof(user_defined_properties_t));
	
	/*TODO: decode bicycle preoperties defined by user to a static struct*/
	
	return ret_code;
}
