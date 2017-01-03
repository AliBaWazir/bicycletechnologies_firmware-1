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
#include "application_fds_app.h"

#include "algorithm_app.h"


/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define DEFAULT_CADENCE_SETPOINT_RPM   80

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/
typedef enum{
	BIKE_CONFIG_DATA_UPDATE,
	CADENCE_SETPOINT_UPDATE
} algorithmApp_event_e;

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static user_defined_bike_config_data_t  user_defined_bike_config_data;  //will contain user defined bike configuration data

static bool  gear_level_locked = false;                     /*TODO: this boolean has to be checked before running the gear shifting algorithm
															 *If set to true, lock gear level at the middle to maximize efficiency.  
															 */
static uint32_t  cadence_setpoint_rpm= 0;

/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
static void algorithmApp_fire_event (algorithmApp_event_e event){
	switch (event){
		case BIKE_CONFIG_DATA_UPDATE:
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
	return (uint8_t)cadence_setpoint_rpm;
}

uint8_t algorithmApp_get_wheel_diameter_cm(void){
	return (uint8_t)user_defined_bike_config_data.wheel_diameter_cm;
}

uint8_t algorithmApp_get_gears_count_crank(void){
	return (uint8_t)user_defined_bike_config_data.crank_gears_count;
}

uint8_t algorithmApp_get_gears_count_wheel(void){
	return (uint8_t)user_defined_bike_config_data.wheel_gears_count;
}

uint8_t algorithmApp_get_teeth_count(uint8_t gear_type, uint8_t gear_index){
	
	if (gear_index >= MAX_GEARS_COUNT){
		NRF_LOG_WARNING("algorithmApp_get_teeth_count: called with grear index=%d higher than max gear count\r\n", gear_index);
		return 0;
	} else{
		if(gear_type == CRANK_IDENTIFIER){
			return (uint8_t)user_defined_bike_config_data.crank_gears_teeth_count[gear_index];
		} else if (gear_type == WHEEL_IDENTIFIER){
			return (uint8_t)user_defined_bike_config_data.wheel_gears_teeth_count[gear_index];
		}else{
			NRF_LOG_ERROR("algorithmApp_get_teeth_count: called with unknown gear_type=0x%x\r\n", gear_type);
			return 0;
		}
	}
}

/************************* SETTERS ****************************/
void algorithmApp_set_cadence_setpoint(uint8_t new_cadence_setpoint){
	cadence_setpoint_rpm = new_cadence_setpoint;
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(CADENCE_SETPOINT_UPDATE);
	
	NRF_LOG_INFO("algorithmApp_set_cadence_setpoint: cadence_setpoint is changed to= %d .\r\n",new_cadence_setpoint);
}

void algorithmApp_set_gear_level_locked(){
	gear_level_locked = true;
	NRF_LOG_INFO("algorithmApp_set_gear_level_locked: gear level is locked= %d .\r\n");

}

void algorithmApp_set_wheel_diameter (uint8_t new_wheel_diameter){
	user_defined_bike_config_data.wheel_diameter_cm = new_wheel_diameter;
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(BIKE_CONFIG_DATA_UPDATE);
	NRF_LOG_INFO("algorithmApp_set_wheel_diameter: wheel_diameter is changed to= %d .\r\n",new_wheel_diameter);
}

bool algorithmApp_set_gear_count(uint8_t crank_gears_count, uint8_t wheel_gears_count){
	
	if ((crank_gears_count > MAX_GEARS_COUNT) || (wheel_gears_count > MAX_GEARS_COUNT)){
		NRF_LOG_WARNING("algorithmApp_set_gear_count: crank_gears_count= %d or wheel_gears_count= %d is greater than MAX_GEARS_COUNT.\r\n",crank_gears_count, wheel_gears_count);
		return false;
	} else{
		user_defined_bike_config_data.crank_gears_count= crank_gears_count;
		user_defined_bike_config_data.wheel_gears_count= wheel_gears_count;
	}
	
	//fire update event to encode updated data in ROM
	algorithmApp_fire_event(BIKE_CONFIG_DATA_UPDATE);
	NRF_LOG_INFO("gear_count is changed to crank_gears_count= %d, wheel_gears_count= %d\r\n",crank_gears_count, wheel_gears_count);
	return true;
}

void algorithmApp_set_teeth_count (uint8_t gear_type, uint8_t gear_index, uint8_t new_teeth_count){
	
	if (gear_type == CRANK_IDENTIFIER){
		user_defined_bike_config_data.crank_gears_teeth_count[gear_index] = new_teeth_count;
	} else if (gear_type == WHEEL_IDENTIFIER){
		user_defined_bike_config_data.wheel_gears_teeth_count[gear_index] = new_teeth_count;
	} else {
		NRF_LOG_ERROR("algorithmApp_set_teeth_count: called with invalid gear_type= %d\r\n",gear_type);
	}
	
    //fire update event to encode updated data in ROM
	algorithmApp_fire_event(BIKE_CONFIG_DATA_UPDATE);
    NRF_LOG_INFO("algorithmApp_set_teeth_count: teeth_count for gear type=%d in gear index[%d] is updated to count= %d.\r\n", gear_type, gear_index, new_teeth_count);
	
}

bool algorithmApp_init(void){
	
	bool ret_code  = true;
	
	memset (&user_defined_bike_config_data, 0, sizeof(user_defined_bike_config_data_t));
	
	/*decode bicycle preoperties defined by user from FDS to a static struct*/
	if(!applicationFdsApp_init()){
		NRF_LOG_ERROR("algorithmApp_init: applicationFdsApp_init() failed\r\n");
		ret_code= false;
	} else{
		
		//read cadence setpoint from FDS
		if(!applicationFdsApp_fds_read(USER_DEFINED_CADENCE_SETPOINT, (uint8_t*) &cadence_setpoint_rpm)){
			
			NRF_LOG_WARNING("algorithmApp_init: applicationFdsApp_fds_read() failed. Default cadenceSetPoint value will be written\r\n");
			
			/*TODO: figure out a way to ask user to enter cadence setpoint for fisrt time use.*/
			
			//For now, if cadence setpoint is not already set, set it to default value
			uint32_t cadence_setpoint_value = DEFAULT_CADENCE_SETPOINT_RPM;
			if(!applicationFdsApp_fds_store(USER_DEFINED_CADENCE_SETPOINT, (uint8_t*)&cadence_setpoint_value)){
				NRF_LOG_ERROR("algorithmApp_init: applicationFdsApp_fds_store() failed to write cadence_setpoint_rpm\r\n");
				ret_code= false;
			} else{
				//data will be read in the callback
				/*read back the written value
				if(!applicationFdsApp_fds_read(USER_DEFINED_CADENCE_SETPOINT, (uint8_t*) &cadence_setpoint_rpm)){
					NRF_LOG_ERROR("algorithmApp_init: applicationFdsApp_fds_read() failed to read cadence_setpoint_rpm\r\n");
					ret_code= false;
				}
				*/
			}
			
		} else{
			NRF_LOG_INFO("algorithmApp_init: cadence setpoint=%d is retrieved from flash data storage\r\n", cadence_setpoint_rpm);
		}
		
		if(ret_code){

			//read bike configration data from FDS
			if(!applicationFdsApp_fds_read(USER_DEFINED_BIKE_CONFIG_DATA, (uint8_t*) &user_defined_bike_config_data)){
				NRF_LOG_WARNING("algorithmApp_init: applicationFdsApp_fds_read() failed. Bike default config will be written\r\n");
				
				/*TODO: figure out a way to ask user to enter bike configuration for fisrt time use.*/
				
				//For now, if cadence setpoint is not already set, set it to default value
				user_defined_bike_config_data_t default_config;
				memset (&default_config, 0, sizeof(user_defined_bike_config_data_t));
				default_config.wheel_diameter_cm= 72;
				default_config.crank_gears_count= 1;
				default_config.crank_gears_teeth_count[0]= 32;
				default_config.wheel_gears_count= 8;
				default_config.wheel_gears_teeth_count[0]= 11;
			    default_config.wheel_gears_teeth_count[1]= 13;
				default_config.wheel_gears_teeth_count[2]= 15;
				default_config.wheel_gears_teeth_count[3]= 18;
				default_config.wheel_gears_teeth_count[4]= 22; 
				default_config.wheel_gears_teeth_count[5]= 26; 
				default_config.wheel_gears_teeth_count[6]= 30; 
				default_config.wheel_gears_teeth_count[7]= 34;
				if(!applicationFdsApp_fds_store(USER_DEFINED_BIKE_CONFIG_DATA, (uint8_t*)&default_config)){
					NRF_LOG_ERROR("algorithmApp_init: applicationFdsApp_fds_store() failed to write bike config\r\n");
					ret_code= false;
				} else{
					//data will be read in the callback
					/*read back the written value
					if(!applicationFdsApp_fds_read(USER_DEFINED_BIKE_CONFIG_DATA, (uint8_t*) &user_defined_bike_config_data)){
						NRF_LOG_ERROR("algorithmApp_init: applicationFdsApp_fds_read() failed to read bike config\r\n");
						ret_code= false;
					}*/
				}
			} else{
				NRF_LOG_INFO("algorithmApp_init: bike config struct is retrieved from flash data storage\r\n");
			}
			
		}

	}
	
	
	return ret_code;
}
