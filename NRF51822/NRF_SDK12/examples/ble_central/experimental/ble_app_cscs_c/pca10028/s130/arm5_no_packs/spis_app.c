/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
 
 /**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include "sdk_config.h"
#include "nrf_drv_spis.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#define NRF_LOG_MODULE_NAME "SPIS APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "cscs_app.h"
#include "spis_sim_driver.h"
#include "algorithm_app.h"
#include "hrs_app.h"
#include "../spis_app.h"

/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define SPI_DRIVER_SIM_MODE 1 /****************************************************
							   *This flag is set to 1 only if SPI slave interaction
							   *is in simulation mode.
							   ***************************************************/

#define SPI_INSTANCE        1 /**< SPIS instance index. */


#define SPI_DUMMY_COMMAND   0xF0       //this command is received when SPI master reads a response

/*************************************************
 *SPI GETTER command definitions
 *************************************************/
//Check Data Availabilable Flags 
#define SPI_GET_AVAILABLE_DATA_FLAGS     0xDA

//Group 0: Shifting Algorithm Parameters
#define SPI_GET_SPEED                  0x01
#define SPI_GET_CADENCE                0x02
#define SPI_GET_DISTANCE               0x03
#define SPI_GET_HR                     0x04
#define SPI_GET_CADENCE_SETPOINT       0x05
#define SPI_GET_BATTERY_LEVEL          0x06

//Group 1: Bike configuration
#define SPI_GET_WHEEL_DIAMETER         0x10
#define SPI_GET_GEAR_COUNT             0x11
#define SPI_GET_TEETH_COUNT_ON_GEAR    0x12

//Group 2: Bluetooth Device Configuration								
#define SPI_GET_ADV_DEVICE_COUNT            0x20
#define SPI_GET_ADV_DEVICE_DATA_BY_INDEX    0x21
#define SPI_GET_PAIRED_DEVICES              0x22
#define SPI_GET_CONNECTED_DEVICES           0x23
#define SPI_GET_CSC_DEVICE_NAME             0x27
#define SPI_GET_HR_DEVICE_NAME              0x28
#define SPI_GET_PHONE_DEVICE_NAME           0x29


/*************************************************
 *SPI SETTER command definitions
 *************************************************/
//Group 0: Shifting Algorithm Parameters							
#define SPI_SET_CADENCE_SETPOINT       0x08
#define SPI_SET_GEAR_LEVEL_LOCKED      0x09

//Group 1: Bike configuration
#define SPI_SET_WHEEL_DIAMETER         0x18
#define SPI_SET_GEAR_COUNT             0x19
#define SPI_SET_TEETH_COUNT_ON_GEAR    0x1A

//Group 2: Bluetooth Device Configuration
#define SPI_BEGIN_SCAN                 0x2A
#define SPI_CONNECT_DEVICE             0x2B
#define SPI_FORGET_DEVICE              0x2C


/*************************************************
 *SPI argument indeces
 *************************************************/
#define INDEX_ARG_BASE                1                  // second bit rx[1] denotes the first argument
#define INDEX_ARG_SETPOINT            INDEX_ARG_BASE     // index of argument setpoint used with SPI_SET_CADENCE_SETPOINT
#define INDEX_ARG_WHEEL_DIAMETER      INDEX_ARG_BASE
#define INDEX_ARG_GEAR_TYPE           INDEX_ARG_BASE     // 0xCA for crank, 0xEE for wheel
#define INDEX_ARG_CRANK_GEARS_COUNT  (INDEX_ARG_BASE)
#define INDEX_ARG_WHEEL_GEARS_COUNT  (INDEX_ARG_BASE+1) 
#define INDEX_ARG_GEAR_INDEX         (INDEX_ARG_BASE+1) //index starts from 0 for fisrt gear
#define INDEX_ARG_TEETH_COUNT        (INDEX_ARG_BASE+2) //teeth count in a gear defined by INDEX_ARG_GEAR_TYPE and INDEX_ARG_GEAR_INDEX.



/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/



/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(SPI_INSTANCE);/**< SPIS instance. */


static uint8_t       m_tx_buf[4];          /**< TX buffer. */
static uint8_t       m_rx_buf[sizeof(m_tx_buf) + 1];    /**< RX buffer. */
static const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */

static volatile bool spis_xfer_done = true; /**< Flag used to indicate that SPIS instance completed the transfer. */

uint8_t bitField[4] = {0x7F, 0x7F, 0x7F, 0x7F};  //static example for now.

static uint32_t data_availability_flags = 0x00000000; 



/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/

/**
 * @brief SPIS user event handler.
 *
 * @param event
 */
static void spisApp_event_handler(nrf_drv_spis_event_t event)
{
    if (event.evt_type == NRF_DRV_SPIS_XFER_DONE){
		
		uint8_t command = m_rx_buf[0];
		
        memset(m_tx_buf, 0x00, sizeof(m_tx_buf)); // clear the tx for visual clarity
        
        spis_xfer_done = true;
        NRF_LOG_INFO("spisApp_event_handler: transfer completed. Received: 0x%x\r\n",command);

		switch (command){
			case SPI_DUMMY_COMMAND
				//do nothing
			break;//SPI_DUMMY_COMMAND
			
			/**************************** GETTERS ********************************/
			case SPI_GET_AVAILABLE_DATA_FLAGS:
				if (SPI_DRIVER_SIM_MODE){
					spisSimDriver_get_data_availability_flags(m_tx_buf);
				} else {
					//memcpy(m_tx_buf, bitField, 4); // Copy current available data into tx buffer in preperation for clock out. 
					memcpy(m_tx_buf, &data_availability_flags, sizeof(data_availability_flags));
				}

			break;
			
			case SPI_GET_SPEED:
				if (SPI_DRIVER_SIM_MODE){
					//m_tx_buf[0] = 0xFA;
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_SPEED);
				} else {
					m_tx_buf[0]= cscsApp_get_current_speed_kmph();
				}
			break;
			
			case SPI_GET_CADENCE:
				if (SPI_DRIVER_SIM_MODE){
					//m_tx_buf[0] = 0xCA;
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_CADENCE);
				} else {
					m_tx_buf[0]= cscsApp_get_current_cadence_rpm();
				}
			break;
				
			case SPI_GET_DISTANCE:
				if (SPI_DRIVER_SIM_MODE){
					//m_tx_buf[0] = 0xDE;
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_DISTANCE);
				} else {
					m_tx_buf[0]= cscsApp_get_current_distance_km();
				}
			break;
			
			case SPI_GET_HR:
				if (SPI_DRIVER_SIM_MODE){
					//m_tx_buf[0] = 0xEA;
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_HR);
				} else {
					m_tx_buf[0] = hrsApp_get_current_hr_bpm();
				}
			break;//SPI_GET_HR
			
			case SPI_GET_CADENCE_SETPOINT:
				m_tx_buf[0] = algorithmApp_get_cadence_setpoint();
			break;//SPI_GET_CADENCE_SETPOINT
			
			case SPI_GET_BATTERY_LEVEL:
				/*TODO: call i2c app to retirieve batttery level from SO*/
			break;//SPI_GET_BATTERY_LEVEL
						
			case SPI_GET_WHEEL_DIAMETER:
				m_tx_buf[0] = algorithmApp_get_wheel_diameter_cm();
			break;//SPI_GET_WHEEL_DIAMETER
				
			case SPI_GET_GEAR_COUNT:
				m_tx_buf[0] = algorithmApp_get_gears_count_crank();
				m_tx_buf[1] = algorithmApp_get_gears_count_wheel();
			break;//SPI_GET_GEAR_COUNT
			
			case SPI_GET_TEETH_COUNT_ON_GEAR:
				m_tx_buf[0] = algorithmApp_get_teeth_count(m_rx_buf[INDEX_ARG_GEAR_TYPE], m_rx_buf[INDEX_ARG_GEAR_INDEX]);
			break;//SPI_GET_TEETH_COUNT_ON_GEAR
			
			
				
			/**************************** SETTERS ********************************/
			case SPI_SET_CADENCE_SETPOINT:
				algorithmApp_set_cadence_setpoint (m_rx_buf[INDEX_ARG_SETPOINT]);
			break;//SPI_SET_CADENCE_SETPOINT
				
			case SPI_SET_GEAR_LEVEL_LOCKED:
				algorithmApp_set_gear_level_locked ();
			break;//SPI_SET_GEAR_LEVEL_LOCKED
				
			case SPI_SET_WHEEL_DIAMETER:
				algorithmApp_set_wheel_diameter(m_rx_buf[INDEX_ARG_WHEEL_DIAMETER]);
			break;//SPI_SET_WHEEL_DIAMETER
				
			case SPI_SET_GEAR_COUNT:
				if (!algorithmApp_set_gear_count(m_rx_buf[INDEX_ARG_CRANK_GEARS_COUNT], m_rx_buf[INDEX_ARG_WHEEL_GEARS_COUNT])){
					NRF_LOG_ERROR("spisApp_event_handler: algorithmApp_set_gear_count failed \r\n");
				}
			break;//SPI_SET_GEAR_COUNT
				
			case SPI_SET_TEETH_COUNT_ON_GEAR:
				algorithmApp_set_teeth_count(m_rx_buf[INDEX_ARG_GEAR_TYPE], m_rx_buf[INDEX_ARG_GEAR_INDEX], m_rx_buf[INDEX_ARG_TEETH_COUNT]);
			break;//SPI_SET_GEAR_COUNT
				
			default:
				NRF_LOG_ERROR("spisApp_event_handler: command in rx buffer is unknown. command= 0x%x\r\n",command);
			break;
		}

    }     
}

static void spisApp_config(void){
    
    // Enable the constant latency sub power mode to minimize the time it takes
    // for the SPIS peripheral to become active after the CSN line is asserted
    // (when the CPU is in sleep mode).
    //commented out due to HardFault
    //NRF_POWER->TASKS_CONSTLAT = 1;

    LEDS_CONFIGURE(BSP_LED_0_MASK);
    LEDS_OFF(BSP_LED_0_MASK);

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

    nrf_drv_spis_config_t spis_config = NRF_DRV_SPIS_DEFAULT_CONFIG;
    spis_config.csn_pin               = APP_SPIS_CS_PIN;
    spis_config.miso_pin              = APP_SPIS_MISO_PIN;
    spis_config.mosi_pin              = APP_SPIS_MOSI_PIN;
    spis_config.sck_pin               = APP_SPIS_SCK_PIN;
    spis_config.def = 0xF0; // This is the data to clock if SPI peripheral can't get control of the memory. Try transfer again.
    spis_config.orc = 0xF1; //This is the data to clock if OVER READ BUFFER

    APP_ERROR_CHECK(nrf_drv_spis_init(&spis, &spis_config, spisApp_event_handler));
    
}

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/

void spisApp_update_data_avail_flags(spi_data_avail_flag_e flag, bool data_available){
	
	if (data_available){
		data_availability_flags|= (flag);
	} else{
		data_availability_flags&= ~(flag);
	}
}


bool spisApp_init(void)
{
	bool ret_code = true;
    
 	spisApp_config();
	
	// if data need to be generated in simulation mode, initialize SPIS simulation driver
	if (ret_code && SPI_DRIVER_SIM_MODE){
		ret_code = spisSimDriver_init();
	}

    if (ret_code){
		NRF_LOG_INFO("SPIS APP initialized successfully\r\n");
	} else {
		NRF_LOG_ERROR("SPIS APP failed to initialize\r\n");
	}
 	
 	return ret_code;
}
void spisApp_spi_wait(){
    
    if (spis_xfer_done){
	//set buffers
        memset(m_rx_buf, 0, m_length);
        spis_xfer_done = false;
        APP_ERROR_CHECK(nrf_drv_spis_buffers_set(&spis, m_tx_buf, m_length, m_rx_buf, m_length));
    }
        
    while (!spis_xfer_done){
        __WFE();//wait for event (sleep)
    }
}
