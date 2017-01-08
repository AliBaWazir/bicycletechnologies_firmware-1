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
#include "nrf_delay.h"

#include "i2c_app.h"
#include "i2c_sim_driver.h"


/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define I2C_IN_SIMULATION_MODE   1        /*This is set to 1 only if the I2C app is running in debugging mode 
                                           *to charecterize the I2C link between nRF module and gear controller
										   */

#define MAX_DELAY_COUNTER        11000     //max value to wait looping until receiving transfer aknowledgement from slave 
										   //value is obtained by experiment

/* TWI instance ID. */
#define TWI_INSTANCE_ID     0

/* Common addresses definition I2C slaves*/
#define GEAR_CONTROLLER_I2C_ADDR  (0x90U >> 1)

#define GEAR_CONTROLLER_REG_GEARSTATUS      0x00U

//SOC
#define SOC_I2C_ADDR              (0xAA >> 1)
#define STATE_OF_CHARGE_REG        0x2c
#define FULL_CHARGE_CAPACITY_REG   0x12

//BNO
#define BNO055_I2C_ADDR           (0x29 >> 1)
#define BNO_ID_REG                 0x00


/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/
typedef struct{
	uint8_t bno055_;
	uint8_t soc_state_of_charge;
	uint8_t gearContoller_front_gear;
	uint8_t gearContoller_back_gear;
} i2c_slaves_data_t;

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Buffer for current gear status read from gear controller */
static uint8_t m_gear_status;

/* Struct to hold new data received from I2C slave devices*/
i2c_slaves_data_t m_i2c_slaves_data_new;

/* Struct to hold old data received from I2C slave devices for comparison purposes*/
i2c_slaves_data_t m_i2c_slaves_data_old;

/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/



/**
 * @brief Function for I2C scan for SOC. For testing purposes
 */
static bool i2cApp_i2c_scan_soc(uint8_t reg_addr, uint8_t* dest_array){
	
	bool ret= false;
	uint8_t* addr; 
	ret_code_t err_code;
	uint8_t i2c_address = 0x00;
	uint8_t working_i2c_addresses[256];
	uint8_t array_index=0;
	
	addr= &reg_addr;
	
	for(i2c_address= 0;i2c_address<1;i2c_address++){
		
		m_xfer_done = false;
		
		//writing to pointer byte
		err_code = nrf_drv_twi_tx(&m_twi, SOC_I2C_ADDR, addr, 1, false);
		while (m_xfer_done == false);
		
		
		//reading first byte
		m_xfer_done = false;
		err_code = nrf_drv_twi_rx(&m_twi, SOC_I2C_ADDR, dest_array, 1);
		APP_ERROR_CHECK(err_code);
		
		(*addr)++;
		//writing to pointer byte
		m_xfer_done = false;
		err_code = nrf_drv_twi_tx(&m_twi, SOC_I2C_ADDR, addr, 1, false);
		while (m_xfer_done == false);
		
		//reading second byte
		m_xfer_done = false;
		err_code = nrf_drv_twi_rx(&m_twi, SOC_I2C_ADDR, dest_array, 1);
		APP_ERROR_CHECK(err_code);
		
	}
	
	return true;
}


/**
 * @brief Function for I2C read.
 */
static bool i2cApp_i2c_read(uint8_t i2c_address, uint8_t reg_addr, uint8_t* dest_array){
	
	bool        ret        = true;
	ret_code_t  err_code   = NRF_SUCCESS;
	uint32_t    counter    = MAX_DELAY_COUNTER;

	m_xfer_done = false;
	//writing to pointer byte
	err_code = nrf_drv_twi_tx(&m_twi, i2c_address, &reg_addr, sizeof(reg_addr), false);
	APP_ERROR_CHECK(err_code);
    while ((!m_xfer_done) && (counter>0)){
		counter--;
	}
	
	if(counter == 0){
		//no acknowldgement is received
		NRF_LOG_ERROR("i2cApp_i2c_read: no acknowldgement is received for i2c slave adress= 0x%x\r\n", i2c_address);
		ret = false; 
	}
	
	if(ret){
		//reading
		m_xfer_done = false;
		err_code = nrf_drv_twi_rx(&m_twi, i2c_address, dest_array, sizeof(dest_array));
		APP_ERROR_CHECK(err_code);
	}
	
	return ret;
	
}


/**
 * @brief Function for I2C read.
 */
static void i2cApp_i2c_write(uint8_t i2c_address, uint8_t reg_addr, uint8_t* source_array){
	
	/*TODO: modify this function to be similar to i2c_read*/
	ret_code_t err_code;
	
	m_xfer_done = false;
	//writing to pointer byte
	err_code = nrf_drv_twi_tx(&m_twi, i2c_address, &reg_addr, 1, false);
	APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
	
	//writing data
	m_xfer_done = false;
	err_code = nrf_drv_twi_tx(&m_twi, i2c_address, source_array, sizeof(source_array), false);
	APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
}


/**
 * @brief Function for reading data from gear sttus register.
 */
/*static void read_gear_status()
{
    m_xfer_done = false;

     Read 1 byte from the specified address
    ret_code_t err_code = nrf_drv_twi_rx(&m_twi, GEAR_CONTROLLER_I2C_ADDR, &m_gear_status, sizeof(m_gear_status));
    APP_ERROR_CHECK(err_code);
}
*/

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
	
	NRF_LOG_INFO("current crank_gear status: 0x%x\r\n", crank_gear);
	NRF_LOG_INFO("current wheel_gear status: 0x%x\r\n", wheel_gear);
	
}



__STATIC_INLINE void soc_data_handler(uint8_t state_of_charge_new, uint8_t state_of_charge_old)
{

	if(state_of_charge_new != state_of_charge_old){
		//state of charge value is changed. Report to GUI
		/*TODO: Report to GUI*/
	}
	
	NRF_LOG_DEBUG("soc_data_handler: new SOC state of charge= %d percent\r\n", state_of_charge_new);
	
}

/**
 * @brief TWI events handler.
 */
static void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX){
				
				if(p_event->xfer_desc.address == SOC_I2C_ADDR){
					soc_data_handler(m_i2c_slaves_data_new.soc_state_of_charge, m_i2c_slaves_data_old.soc_state_of_charge);
				} else{
					/*TODO: add else if statements to handle other adresses*/
					//data_handler(m_gear_status);
				}
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
       .scl                = NRF_SCL_PIN,
       .sda                = NRF_SDA_PIN,
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

    //read_gear_status();
	
}
/**
 * @brief Connection Manager App initialization.
 */
bool i2cApp_init(void){
	
	bool         ret                = true;
	//ret_code_t   nrf_err          = NRF_SUCCESS;
	
	twi_init();
	
	memset(&m_i2c_slaves_data_old, 0x00, sizeof(i2c_slaves_data_t));
	memset(&m_i2c_slaves_data_new, 0x00, sizeof(i2c_slaves_data_t));

	
	if (I2C_IN_SIMULATION_MODE){
		
		//read id register of BNO055. For testing purpose only
		//i2cApp_i2c_read(BNO055_I2C_ADDR, 0xAB, &m_gear_status);
		
		//read stateOfCharge register of SOC. For testing purpose only
		m_i2c_slaves_data_old.soc_state_of_charge= m_i2c_slaves_data_new.soc_state_of_charge;
		if(!i2cApp_i2c_read(SOC_I2C_ADDR, STATE_OF_CHARGE_REG, &(m_i2c_slaves_data_new.soc_state_of_charge))){
			NRF_LOG_ERROR("i2cApp_init: i2cApp_i2c_read failed for SOC_I2C_ADDR\r\n");
		}
	
		ret = i2cSimDriver_init();
	}
		
	return ret;	
}
