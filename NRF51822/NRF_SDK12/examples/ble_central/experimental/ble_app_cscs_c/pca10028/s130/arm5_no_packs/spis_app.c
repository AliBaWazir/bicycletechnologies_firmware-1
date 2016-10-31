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
#include "sdk_config.h"
#include "nrf_drv_spis.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "../spis_app.h"


#define SPIS_INSTANCE 1 /**< SPIS instance index. */
static const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(SPIS_INSTANCE);/**< SPIS instance. */


static uint8_t       m_tx_buf[4];          /**< TX buffer. */
static uint8_t       m_rx_buf[sizeof(m_tx_buf) + 1];    /**< RX buffer. */
static const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */

static volatile bool spis_xfer_done = true; /**< Flag used to indicate that SPIS instance completed the transfer. */

uint8_t bitField[4] = {0x7F, 0x7F, 0x7F, 0x7F};  //static example for now.

static void spisApp_buffers_set(){
 	
	memset(m_rx_buf, 0, m_length);
        
	// Actually set the buffers in the SPI peripheral. Any further clocks clock out the data and fill the RX buffer.
    APP_ERROR_CHECK(nrf_drv_spis_buffers_set(&spis, m_tx_buf, m_length, m_rx_buf, m_length));
 	
 	spis_xfer_done = false;
}

/**
 * @brief SPIS user event handler.
 *
 * @param event
 */
static void spis_event_handler(nrf_drv_spis_event_t event)
{
    if (event.evt_type == NRF_DRV_SPIS_XFER_DONE)
    {
        memset(m_tx_buf, 0x00,sizeof(m_tx_buf)); // clear the tx for visual clarity
        
        spis_xfer_done = true;
        NRF_LOG_INFO(" Transfer completed. Received: %s\r\n",(uint32_t)m_rx_buf);

        if(m_rx_buf[0] == 0xDA){
            memcpy(m_tx_buf, bitField, 4); // Copy current available data into tx buffer in preperation for clock out. 
        }

        if(m_rx_buf[0] == 0x01){//speed has been requested. Add to buffer
           m_tx_buf[0] = 0xFA;
        }
        if(m_rx_buf[0] == 0x02){//cadence has been requested. Add to buffer
           m_tx_buf[0] = 0xCA;
        }
        if(m_rx_buf[0] == 0x04){//HR has been requested. Add to buffer
           m_tx_buf[0] = 0xEA;
        }
        if(m_rx_buf[0] == 0x08){//batt has been requested. Add to buffer
           m_tx_buf[0] = 0xBA;
        }
        //more to come here. Refactor into a switch statement?
        
        
        

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

    APP_ERROR_CHECK(nrf_drv_spis_init(&spis, &spis_config, spis_event_handler));
    
}

bool spisApp_init(void)
{
	bool ret_code = true;
 	
    
    
 	spisApp_config();

    
 	NRF_LOG_INFO("SPIS APP initialized successfully\r\n");
 	
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