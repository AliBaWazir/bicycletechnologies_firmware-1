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

#include "spis_sim_driver.h"
#include "../spis_app.h"

/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define SPIS_INSTANCE 1 /**< SPIS instance index. */

//SPI requests definitions
#define SPIS_REQUEST_AVAILABLE_DATA     0xDA
#define SPIS_REQUEST_SPEED              0x01
#define SPIS_REQUEST_CADENCE            0x02
#define SPIS_REQUEST_DISTANCE           0x03
#define SPIS_REQUEST_HR                 0x04
#define SPIS_REQUEST_BATTERY            0x05


#define SPIS_DRIVER_SIM_MODE 1 /*
								This flag is set to 1 only if SPI slave interaction
								is in simulation mode.
								*/

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/


/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(SPIS_INSTANCE);/**< SPIS instance. */


static uint8_t       m_tx_buf[4];          /**< TX buffer. */
static uint8_t       m_rx_buf[sizeof(m_tx_buf) + 1];    /**< RX buffer. */
static const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */

static volatile bool spis_xfer_done = true; /**< Flag used to indicate that SPIS instance completed the transfer. */

uint8_t bitField[4] = {0x7F, 0x7F, 0x7F, 0x7F};  //static example for now.


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
        NRF_LOG_INFO(" Transfer completed. Received: %s\r\n",(uint32_t)m_rx_buf);

		switch (command){
			
			case SPIS_REQUEST_AVAILABLE_DATA:
				memcpy(m_tx_buf, bitField, 4); // Copy current available data into tx buffer in preperation for clock out. 
			break;
			
			case SPIS_REQUEST_SPEED:
				if (SPIS_DRIVER_SIM_MODE){
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_SPEED);
				} else {
					/*TODO: get data from CSCS app*/
					m_tx_buf[0] = 0xFA;
				}
			break;
			
			case SPIS_REQUEST_CADENCE:
				if (SPIS_DRIVER_SIM_MODE){
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_CADENCE);
				} else {
					/*TODO: get data from CSCS app*/
					m_tx_buf[0] = 0xCA;
				}
			break;
				
			case SPIS_REQUEST_DISTANCE:
				if (SPIS_DRIVER_SIM_MODE){
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_DISTANCE);
				} else {
					/*TODO: get data from CSCS app*/
					m_tx_buf[0] = 0xDE;
				}
			break;
			
			case SPIS_REQUEST_HR:
				if (SPIS_DRIVER_SIM_MODE){
					m_tx_buf[0] = spisSimDriver_get_current_data(CSCS_DATA_HR);
				} else {
					/*TODO: get data from CSCS app*/
					m_tx_buf[0] = 0xEA;
				}
			break;
			
			case SPIS_REQUEST_BATTERY:
				/*TODO: figure out which device's battery*/
				m_tx_buf[0] = 0xBA;
			break;
			
			default:
				NRF_LOG_ERROR("command in rx buffer is unknown. command= 0x%x\r\n",command);
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
    spis_config.def = 0xDF; // This is the data to clock if SPI peripheral can't get control of the memory. Try transfer again.
    spis_config.orc = 0xDC; //This is the data to clock if OVER READ BUFFER

    APP_ERROR_CHECK(nrf_drv_spis_init(&spis, &spis_config, spisApp_event_handler));
    
}

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/
bool spisApp_init(void)
{
	bool ret_code = true;
    
 	spisApp_config();
	
	// if data need to be generated in simulation mode, initialize SPIS simulation driver
	if (ret_code && SPIS_DRIVER_SIM_MODE){
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
        
        while (!spis_xfer_done)
        {
            __WFE();//wait for event (sleep)
        }
}
