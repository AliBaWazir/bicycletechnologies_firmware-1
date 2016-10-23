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
 * @brief SPI slave simulation driver.
 * This deiver will generate the required data that needs to be exchanged between the SPI master
 * (ST board) and SPI slave (nRF board).
 *
 */
 
/**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include "app_error.h"
#define NRF_LOG_MODULE_NAME "SPI SLAVE SIM DRIVER"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "spis_sim_driver.h"

/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define SIM_DATA_ARRAY_SIZE 20
#define MAX_SIM_SPEED       40   //For cyclists in Copenhagen, the average cycling speed is 15.5 km/h 
                                 //(9.6 mph). On a racing bicycle, a reasonably fit rider can ride 
								 //at 40 km/h (25 mph) on flat ground for short periods.

#define MAX_SIM_CADENCE     80   //Recreational and utility cyclists typically cycle around 60–80 rpm. 
								 //According to cadence measurement of 7 professional cyclists during 
								 //3 week races they cycle about 90 rpm during flat and long (~190 km) 
							     //group stages and individual time trials of ~50 km.
#define MAX_SIM_HR          100

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/
/*typedef struct{
	uint8_t wheel_speed_kmph;
	uint8_t crank_cadence_rpm;
	uint8_t heart_rate_bpm;
}spisSimDriver_inst_data_t;
*/


/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
//data to be set by slave
static uint8_t speed_kmph_sim_array [SIM_DATA_ARRAY_SIZE];
static uint8_t cadence_rpm_sim_array [SIM_DATA_ARRAY_SIZE];
static uint8_t hr_bpm_sim_array [SIM_DATA_ARRAY_SIZE];

//data to be set by master
/*TODO: implement APIs to set these values after receiving update requests from SPIS app*/
/*
static uint8_t cadence_set_point_rpm   = 0;
static uint8_t crank_gear_count        = 0;
static uint8_t wheel_gear_count        = 0;
*/

/*TODO: figure out how to do theeth count per gear*/

//indexes for the current value of sim data
static uint8_t speed_index      = 0;
static uint8_t cadence_index    = 0;
static uint8_t hr_index         = 0;

/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
//update the current index to the next value
static bool spisSimDriver_update_index (cscs_data_type_e index_type){
	bool ret_code = true;
	
	switch (index_type){
		case CSCS_DATA_SPEED:
			if ((speed_index+1)<SIM_DATA_ARRAY_SIZE){
				speed_index +=1;
			} else {
				//reset the index to 0
				speed_index = 0;
			}
		break;
		
		case CSCS_DATA_CADENCE:
			if ((cadence_index+1)<SIM_DATA_ARRAY_SIZE){
				cadence_index +=1;
			} else {
				//reset the index to 0
				cadence_index = 0;
			}	
		break;
		
		case CSCS_DATA_HR:
			if ((hr_index+1)<SIM_DATA_ARRAY_SIZE){
				hr_index +=1;
			} else {
				//reset the index to 0
				hr_index = 0;
			}
		break;
		
		default:
			ret_code = false;
		break;
	}
	
	return ret_code;
}

static bool spisSimDriver_init_speed_array(){
	bool    ret_code  = true;
	uint8_t i         = 0;
	
	for (i =0; i<SIM_DATA_ARRAY_SIZE; i++){
		if (i<=(SIM_DATA_ARRAY_SIZE/2)){
			// at the first half simulation cycle, speed will increase gradually from 0 to MAX_SIM_SPEED km/h
			speed_kmph_sim_array[i]= i*(MAX_SIM_SPEED/(SIM_DATA_ARRAY_SIZE/2));
		} else {
			// at the second half simulation cycle, speed will decrease gradually from MAX_SIM_SPEED km/h to 0
			// make use of symmetry
			uint8_t symmetric_i = (SIM_DATA_ARRAY_SIZE/2) - (i -SIM_DATA_ARRAY_SIZE/2);
			speed_kmph_sim_array[i]= speed_kmph_sim_array[symmetric_i];
		}
		
	}
	
	return ret_code;
}

static bool spisSimDriver_init_cadence_array(){
	bool    ret_code  = true;
	uint8_t i         = 0;
	
	for (i =0; i<SIM_DATA_ARRAY_SIZE; i++){
		if (i<=(SIM_DATA_ARRAY_SIZE/2)){
			// at the first half simulation cycle, cadence will increase gradually from 0 to MAX_SIM_CADENCE rpm
			cadence_rpm_sim_array[i]= i*(MAX_SIM_CADENCE/(SIM_DATA_ARRAY_SIZE/2));
		} else {
			// at the second half simulation cycle, cadence will decrease gradually from MAX_SIM_CADENCE rpm to 0
			// make use of symmetry
			uint8_t symmetric_i = (SIM_DATA_ARRAY_SIZE/2) - (i -SIM_DATA_ARRAY_SIZE/2);
			cadence_rpm_sim_array[i]= cadence_rpm_sim_array[symmetric_i];
		}
		
	}
	
	return ret_code;
}

static bool spisSimDriver_init_hr_array(){
	bool    ret_code  = true;
	uint8_t i         = 0;
	
	for (i =0; i<SIM_DATA_ARRAY_SIZE; i++){
		if (i<=(SIM_DATA_ARRAY_SIZE/2)){
			// at the first half simulation cycle, hr will increase gradually from 0 to MAX_SIM_HR bpm
			hr_bpm_sim_array[i]= i*(MAX_SIM_HR/(SIM_DATA_ARRAY_SIZE/2));
		} else {
			// at the second half simulation cycle, cadence will decrease gradually from MAX_SIM_HR bpm to 0
			// make use of symmetry
			uint8_t symmetric_i = (SIM_DATA_ARRAY_SIZE/2) - (i -SIM_DATA_ARRAY_SIZE/2);
			hr_bpm_sim_array[i]= hr_bpm_sim_array[symmetric_i];
		}
		
	}
	
	return ret_code;
}



/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/
uint8_t spisSimDriver_get_current_data(cscs_data_type_e data_type) {
	uint8_t current_data  = 0;
	
	switch (data_type){
		case CSCS_DATA_SPEED:
			current_data = speed_kmph_sim_array[speed_index];
			//update the index to get next data next time
			if (!spisSimDriver_update_index(CSCS_DATA_SPEED)){
				NRF_LOG_ERROR("spisSimDriver_update_index failed to update speed index.\r\n");
			}
		break;
		
		case CSCS_DATA_CADENCE:
			current_data = cadence_rpm_sim_array[cadence_index];
			//update the index to get next data next time
			if (!spisSimDriver_update_index(CSCS_DATA_CADENCE)){
				NRF_LOG_ERROR("spisSimDriver_update_index failed to update cadence index.\r\n");
			}
		break;
		
		case CSCS_DATA_HR:
			current_data = hr_bpm_sim_array[hr_index];
			//update the index to get next data next time
			if (!spisSimDriver_update_index(CSCS_DATA_HR)){
				NRF_LOG_ERROR(".spisSimDriver_update_index failed to update hear rate index\r\n");
			}
		break;
		
		default:
			NRF_LOG_ERROR("spisSimDriver_get_current_data called with invalid argument. data_type= %d\r\n"
		                  ,data_type);
		break;
	}
	
	return current_data;
}

/*Initialize SPIS simulation driver*/
bool spisSimDriver_init(){
	bool ret_code  = true;
	
	//initialze sim data arrays
	ret_code = spisSimDriver_init_speed_array();
	
	if (ret_code){
		ret_code = spisSimDriver_init_cadence_array();
	}
		
	if (ret_code){
		ret_code = spisSimDriver_init_hr_array();
	}
	
	return ret_code;
}
