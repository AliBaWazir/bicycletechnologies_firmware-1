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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app_error.h"
#define NRF_LOG_MODULE_NAME "SPI SLAVE SIM DRIVER"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_timer.h"
#include "nrf_gpio.h"

#include "spis_sim_driver.h"

/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define SIM_DATA_ARRAY_SIZE 10   //The size has to be even number
#define MAX_SIM_SPEED       40   //For cyclists in Copenhagen, the average cycling speed is 15.5 km/h 
                                 //(9.6 mph). On a racing bicycle, a reasonably fit rider can ride 
								 //at 40 km/h (25 mph) on flat ground for short periods.

#define MAX_SIM_CADENCE     80   //Recreational and utility cyclists typically cycle around 60–80 rpm. 
								 //According to cadence measurement of 7 professional cyclists during 
								 //3 week races they cycle about 90 rpm during flat and long (~190 km) 
							     //group stages and individual time trials of ~50 km.
#define MAX_SIM_DISTANCE    200
#define MAX_SIM_HR          100
#define MAX_SIM_BATTERY     100

//Timer Defines
#define APP_TIMER_PRESCALER           0               /**< Value of the RTC1 PRESCALER register. If changed, remember to change prescaler in main.c*/
#define I2CAPP_MS_TO_TICK(MS) (APP_TIMER_TICKS(100, APP_TIMER_PRESCALER) * (MS / 100))
#define SPI_SIM_IRQ_TOGGLE_PERIOD_MS  1000            //time period in ms to pull data from sensors 

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
//data availability flags in simulation mode
static uint32_t sim_data_availability_flags = 0; //by default all falgs are off

//data to be set by slave
static uint8_t speed_kmph_sim_array [SIM_DATA_ARRAY_SIZE];
static uint8_t cadence_rpm_sim_array [SIM_DATA_ARRAY_SIZE];
static uint8_t distance_km_sim_array [SIM_DATA_ARRAY_SIZE];
static uint8_t hr_bpm_sim_array [SIM_DATA_ARRAY_SIZE];
static uint8_t battery_sim_array [SIM_DATA_ARRAY_SIZE];


//indexes for the current value of sim data
static uint8_t speed_index      = 0;
static uint8_t cadence_index    = 0;
static uint8_t distance_index   = 0;
static uint8_t hr_index         = 0;
static uint8_t battery_index    = 0;

/* IRQ toggling timer*/
APP_TIMER_DEF(spi_sim_irq_timer_id);

/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/

//update the current index to the next value for CSCS arrays
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
			
		case CSCS_DATA_DISTANCE:
			if ((distance_index+1)<SIM_DATA_ARRAY_SIZE){
				distance_index +=1;
			} else {
				//reset the index to 0
				distance_index = 0;
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

static bool spisSimDriver_init_distance_array(){
	bool    ret_code  = true;
	uint8_t i         = 0;
	
	for (i =0; i<SIM_DATA_ARRAY_SIZE; i++){
		if (i<=(SIM_DATA_ARRAY_SIZE/2)){
			// at the first half simulation cycle, travelled distance will increase gradually from 0 to MAX_SIM_DISTANCE km
			distance_km_sim_array[i]= i*(MAX_SIM_DISTANCE/(SIM_DATA_ARRAY_SIZE/2));
		} else {
			// at the second half simulation cycle, travelled distance will decrease gradually from MAX_SIM_DISTANCE km to 0
			// make use of symmetry
			uint8_t symmetric_i = (SIM_DATA_ARRAY_SIZE/2) - (i -SIM_DATA_ARRAY_SIZE/2);
			distance_km_sim_array[i]= distance_km_sim_array[symmetric_i];
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


static bool spisSimDriver_init_battery_array(){
	bool    ret_code  = true;
	uint8_t i         = 0;
	
	for (i =0; i<SIM_DATA_ARRAY_SIZE; i++){
		if (i<=(SIM_DATA_ARRAY_SIZE/2)){
			// at the first half simulation cycle, hr will increase gradually from 0 to MAX_SIM_BATTERY
			battery_sim_array[i]= i*(MAX_SIM_BATTERY/(SIM_DATA_ARRAY_SIZE/2));
		} else {
			// at the second half simulation cycle, cadence will decrease gradually from MAX_SIM_BATTERY to 0
			// make use of symmetry
			uint8_t symmetric_i = (SIM_DATA_ARRAY_SIZE/2) - (i -SIM_DATA_ARRAY_SIZE/2);
			battery_sim_array[i]= battery_sim_array[symmetric_i];
		}
		
	}
	
	return ret_code;
}


static void spisSimDriver_spi_irq_timer_handler( void * callback_data){
	
	uint32_t* polling_request_id_p = (uint32_t*) callback_data;
	uint32_t  nrf_err              = NRF_SUCCESS;   
	
	/*TODO: figure out if this is necessary*/
	if (polling_request_id_p != NULL){
		NRF_LOG_DEBUG("spisSimDriver_spi_irq_timer_handler: polling request with ID=%d timed out\r\n", *polling_request_id_p);
	}
	
	//stop timer

	(void)app_timer_stop(spi_sim_irq_timer_id);

	
	//toggle spi irq
	nrf_gpio_pin_toggle(APP_SPIS_IRQ_PIN);
	NRF_LOG_DEBUG("spisSimDriver_spi_irq_timer_handler: SPI IRQ is toggled\r\n");

	
	//start timer againg
	nrf_err = app_timer_start(spi_sim_irq_timer_id, I2CAPP_MS_TO_TICK(SPI_SIM_IRQ_TOGGLE_PERIOD_MS), (void*)NULL);
	if (nrf_err != NRF_SUCCESS)
	{
		NRF_LOG_ERROR("spisSimDriver_spi_irq_timer_handler: app_timer_start failed with error = %d\r\n", nrf_err);
	}
}

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/

uint8_t spisSimDriver_get_current_battery(){
	
	uint8_t current_data  = 0;

	current_data = battery_sim_array[battery_index];
	
	//update the index to get next data next time
	if ((battery_index+1)<SIM_DATA_ARRAY_SIZE){
		battery_index +=1;
	} else {
		//reset the index to 0
		battery_index = 0;
	}
	
	return current_data;
}
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
		
		case CSCS_DATA_DISTANCE:
			current_data = distance_km_sim_array[distance_index];
			//update the index to get next data next time
			if (!spisSimDriver_update_index(CSCS_DATA_DISTANCE)){
				NRF_LOG_ERROR("spisSimDriver_update_index failed to update cadence index.\r\n");
			}
		break;
		
		case CSCS_DATA_HR:
			current_data = hr_bpm_sim_array[hr_index];
			//update the index to get next data next time
			if (!spisSimDriver_update_index(CSCS_DATA_HR)){
				NRF_LOG_ERROR(".spisSimDriver_update_index failed to update heart rate index\r\n");
			}
		break;
		
		default:
			NRF_LOG_ERROR("spisSimDriver_get_current_data called with invalid argument. data_type= %d\r\n"
		                  ,data_type);
		break;
	}
	
	return current_data;
}

bool spisSimDriver_get_data_availability_flags(uint8_t* dest_buffer){
	
	if (dest_buffer == NULL){
		
		NRF_LOG_ERROR("spisSimDriver_get_data_availability_flags: called with NULL dest_buffer\r\n");
		return false;
		
	} else{
		
		sim_data_availability_flags |= 0xDB; //0b11011011;
		sim_data_availability_flags = (sim_data_availability_flags<<8);
		
		sim_data_availability_flags |= 0xCC; //0b11001100
		sim_data_availability_flags = (sim_data_availability_flags<<8);
		
		sim_data_availability_flags |= 0xAA; //0b10101010
		sim_data_availability_flags = (sim_data_availability_flags<<8);
		
		sim_data_availability_flags |= 0x01; //0b00000001 //only speed is present
		
	}
	
	memcpy(dest_buffer, &sim_data_availability_flags, sizeof(sim_data_availability_flags));
	
	return true;
	
}

/*Initialize SPIS simulation driver*/
bool spisSimDriver_init(void){
	bool     ret_code  = true;
	uint32_t nrf_err   = NRF_SUCCESS;
	
	//initialze sim data arrays
	ret_code = spisSimDriver_init_speed_array();
	
	if (ret_code){
		ret_code = spisSimDriver_init_cadence_array();
	}
	
    if (ret_code){
		ret_code = spisSimDriver_init_distance_array();
	}
		
	if (ret_code){
		ret_code = spisSimDriver_init_hr_array();
	}
	
	if (ret_code){
		ret_code = spisSimDriver_init_battery_array();
	}
	
	if (ret_code){
		//use timer to toggle SPI IRQ
		nrf_err = app_timer_create(&spi_sim_irq_timer_id,
									APP_TIMER_MODE_SINGLE_SHOT,
									spisSimDriver_spi_irq_timer_handler);
		if (nrf_err != NRF_SUCCESS){
			NRF_LOG_DEBUG("spisSimDriver_init: app_timer_create failed with error= %d\r\n", nrf_err);
			ret_code = false;
		} else{
			
			//start the timer
			nrf_err = app_timer_start(spi_sim_irq_timer_id, I2CAPP_MS_TO_TICK(SPI_SIM_IRQ_TOGGLE_PERIOD_MS), (void*)NULL);
			if (nrf_err != NRF_SUCCESS)
			{
				NRF_LOG_ERROR("spisSimDriver_init: app_timer_start failed with error = %d\r\n", nrf_err);
				ret_code = false;
			}
			
		}
	}
	
	return ret_code;
}
