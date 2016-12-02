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
 * @brief I2C communication application file.
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

/* TWI instance ID. */
#define TWI_INSTANCE_ID     0

/* Common addresses definition for Gear Controller */
#define GEAR_CONTROLLER_I2C_ADDR            (0x90U >> 1)

#define GEAR_CONTROLLER_REG_GEAR_STATUS      0x00U

/* Pin Assignments */
#define GEAR_CONTROLLER_I2C_SCL_PIN          27
#define GEAR_CONTROLLER_I2C_SDA_PIN          28 /*TODO: get the correct pin numbers*/


/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/


/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Buffer for current gear status read from gear controller */
static uint8_t m_gear_status;



/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
/**
 * @brief Function for reading data from gear sttus register.
 */
static void read_gear_status()
{
    m_xfer_done = false;

    /* Read 1 byte from the specified address*/
    ret_code_t err_code = nrf_drv_twi_rx(&m_twi, GEAR_CONTROLLER_I2C_ADDR, &m_gear_status, sizeof(m_gear_status));
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Function for handling data from gear controller.
 *
 * @param[in] gear_status          Current gear status reported from Gear Controller
 */
__STATIC_INLINE void data_handler(uint8_t gear_status)
{
    uint8_t crank_gear = 0;
	uint8_t wheel_gear = 0;
	
	crank_gear = gear_status & 0x0F;
	wheel_gear = (gear_status >> 4);
	
	NRF_LOG_INFO("current crank_gear status: %d\r\n", crank_gear);
	NRF_LOG_INFO("current wheel_gear status: %d\r\n", wheel_gear);
}


/**
 * @brief TWI events handler.
 */
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                data_handler(m_gear_status);
            }
            m_xfer_done = true;
            break;
        default:
            break;
    }
}


/**
 * @brief TWI initialization.
 */
static void twi_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_lm75b_config = {
       .scl                = GEAR_CONTROLLER_I2C_SCL_PIN,
       .sda                = GEAR_CONTROLLER_I2C_SDA_PIN,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/
void i2cApp_wait(){
	//nrf_delay_ms(500);

	do
    {
		__WFE();
    }while (m_xfer_done == false);

    read_gear_status();
	
}
/**
 * @brief Connection Manager App initialization.
 */
bool i2cApp_init(void){
	
	bool         ret                = true;
	//ret_code_t   nrf_err          = NRF_SUCCESS;
	
	twi_init();
	
	if (I2C_IN_SIMULATION_MODE){
		ret = i2cSimDriver_init();
	}
		
	return ret;	
}
