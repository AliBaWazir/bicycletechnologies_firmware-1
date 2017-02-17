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
#define GEAR_CONT_I2C_ADDR                 (0x90U >> 1)
#define GEAR_CONT_FRONT_GEARS_COUNT_COMMAND 0x01
#define GEAR_CONT_BACK_GEARS_COUNT_COMMAND  0x02
#define GEAR_CONT_FRONT_GEARS_POS_COMMAND   0x03    //Front Gear Positions
#define GEAR_CONT_BACK_GEARS_POS_COMMAND    0x04    //Back Gear Positions
#define GEAR_CONT_INIT_GEARS_POS_COMMAND    0x06    //Initial Gear Positions. This will be in gear number not ADC value
#define GEAR_CONT_WRITE_DES_GEARS_COMMAND   0x10
#define GEAR_CONT_GEARSTATE_COMMAND         0x20

//SOC
#define SOC_I2C_ADDR              (0xAA >> 1)
#define STATE_OF_CHARGE_REG        0x2c
#define FULL_CHARGE_CAPACITY_REG   0x12

//BNO
#define BNO055_I2C_ADDR           (0x28)
#define BNO_ID_REG                 0x00

//Timer Defines
#define APP_TIMER_PRESCALER          0               /**< Value of the RTC1 PRESCALER register. If changed, remember to change prescaler in main.c*/
#define I2CAPP_MS_TO_TICK(MS) (APP_TIMER_TICKS(100, APP_TIMER_PRESCALER) * (MS / 100))
#define I2CAPP_DATA_POLL_PERIOD_MS   10000           //time period in ms to pull data from sensors 

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/
typedef struct{
	uint8_t bno055_id;
	uint8_t soc_state_of_charge;
	uint16_t gearContoller_gears_state;
} i2c_slaves_data_t;

typedef enum{
	I2C_OPER_UNKNOWN = 0,
	//bno055 operations
	I2C_OPER_READ_BNO055_ID,
	//SOC operations
	I2C_OPER_READ_STATE_OF_CHARGE,
	//motor controller operations
	I2C_OPER_READ_SCREEN_ORIENT,
	I2C_OPER_READ_CURR_GEARS_STATE,
	I2C_OPER_READ_CURR_ADC_POSITION,
	I2C_OPER_READ_MOTOR_TEMP,
	I2C_OPER_READ_INT_FLAGS,
	I2C_OPER_READ_LOG_SIZE, //number of bytes in a log
	I2C_OPER_READ_LOG //log data
} i2c_operation_type_e;

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
/* Polling Timer*/
APP_TIMER_DEF(i2c_polling_timer_id);
static bool polling_timer_started  = false;    //set after timer is started for first time

/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Buffer for current gear status read from gear controller */
//static uint8_t m_gear_status;

/* Struct to hold new data received from I2C slave devices*/
i2c_slaves_data_t m_i2c_slaves_data_new;

/* Struct to hold old data received from I2C slave devices for comparison purposes*/
i2c_slaves_data_t m_i2c_slaves_data_old;

static new_meas_callback_f    new_meas_cb    = NULL;    // Function pointer to the function to be called when a new measuremnt is obtained

i2c_operation_type_e i2c_operation_type      = I2C_OPER_UNKNOWN;

static uint8_t motor_cont_adc_pos_front_gears[MAX_GEARS_COUNT]= {100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t motor_cont_adc_pos_back_gears[MAX_GEARS_COUNT]= {0, 20, 40, 60, 80, 100, 120, 140, 160, 0, 0, 0, 0, 0, 0, 0};

/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/

/**
 * @brief Function for I2C scan for SOC. For testing purposes
 */
static bool i2cApp_i2c_scan_soc(uint8_t reg_addr, uint8_t* dest_array){
	
	//bool ret= false;
	uint8_t* addr; 
	ret_code_t err_code;
	uint8_t i2c_address = 0x00;
	uint8_t working_i2c_addresses[256];
	
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
static bool i2cApp_i2c_read(uint8_t i2c_slave_address, uint8_t reg_addr, uint8_t* dest_array, uint8_t bytes_count){
	
	bool        ret        = true;
	ret_code_t  err_code   = NRF_SUCCESS;
	uint32_t    counter    = MAX_DELAY_COUNTER;

	m_xfer_done = false;
	//writing to pointer byte
	err_code = nrf_drv_twi_tx(&m_twi, i2c_slave_address, &reg_addr, sizeof(reg_addr), false);
	if (err_code != NRF_SUCCESS){
		
		NRF_LOG_ERROR("i2cApp_i2c_read: nrf_drv_twi_tx failed with err_code= %d\r\n", err_code);
		ret = false;
		
	} else{
		
		while (m_xfer_done == false);
		/*
		//run the waiting for acknowledgement counter
		while ((!m_xfer_done) && (counter>0)){
			counter--;
		}
		
		if(counter == 0){
			//no acknowldgement is received
			NRF_LOG_ERROR("i2cApp_i2c_read: no acknowldgement is received for i2c slave adress= 0x%x\r\n", i2c_address);
			ret = false; 
		}
		*/
	}
	
	if(ret){
		
		//reading data
		m_xfer_done = false;
		err_code = nrf_drv_twi_rx(&m_twi, i2c_slave_address, dest_array, bytes_count);
		if (err_code != NRF_SUCCESS){
			NRF_LOG_ERROR("i2cApp_i2c_read: nrf_drv_twi_rx failed with err_code= %d\r\n", err_code);
			ret = false;
			
		}
		
	}
	
	return ret;
	
}


/**
 * @brief Function for I2C read.
 */
static bool i2cApp_i2c_write(uint8_t i2c_slave_address, uint8_t reg_addr, uint8_t* source_array, uint8_t bytes_count){
	
	bool        ret        = true;
	ret_code_t  err_code   = NRF_SUCCESS;
	
	m_xfer_done = false;
	//writing to pointer byte
	err_code = nrf_drv_twi_tx(&m_twi, i2c_slave_address, &reg_addr, sizeof(reg_addr), false);
	if (err_code != NRF_SUCCESS){
		
		NRF_LOG_ERROR("i2cApp_i2c_write: nrf_drv_twi_tx failed to write to pointer byte. err_code= %d\r\n", err_code);
		ret = false;
		
	} else{
		
		while (m_xfer_done == false);
		/*
		//run the waiting for acknowledgement counter
		while ((!m_xfer_done) && (counter>0)){
			counter--;
		}
		
		if(counter == 0){
			//no acknowldgement is received
			NRF_LOG_ERROR("i2cApp_i2c_write: no acknowldgement is received for i2c slave adress= 0x%x\r\n", i2c_address);
			ret = false; 
		}
		*/
	}
	
	if(ret){
		
		//writing data
		m_xfer_done = false;
		err_code = nrf_drv_twi_tx(&m_twi, i2c_slave_address, source_array, bytes_count, false);
		if (err_code != NRF_SUCCESS){
			
			NRF_LOG_ERROR("i2cApp_i2c_write: nrf_drv_twi_tx failed to write data. err_code= %d\r\n", err_code);
			ret = false;
			
		} else{
			
			while (m_xfer_done == false);
			/*
			//run the waiting for acknowledgement counter
			while ((!m_xfer_done) && (counter>0)){
				counter--;
			}
			
			if(counter == 0){
				//no acknowldgement is received
				NRF_LOG_ERROR("i2cApp_i2c_write: no acknowldgement is received for i2c slave adress= 0x%x\r\n", i2c_address);
				ret = false; 
			}
			*/
		}
		
	}
	
	return ret;
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
		
		//call the new measurement callback 
		if(new_meas_cb != NULL){
			new_meas_cb(SPI_AVAIL_FLAG_BATTERY, true);
		}
	}
	
	NRF_LOG_DEBUG("soc_data_handler: new SOC state of charge= %d percent\r\n", state_of_charge_new);
	
}

__STATIC_INLINE void bno055_data_handler(uint8_t bno055_id_new, uint8_t bno055_id_old){
	
	NRF_LOG_DEBUG("bno055_data_handler: new bno055 id= %d\r\n", bno055_id_new);
	
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
				
				if((p_event->xfer_desc.address == SOC_I2C_ADDR) && (i2c_operation_type == I2C_OPER_READ_STATE_OF_CHARGE)){
					soc_data_handler(m_i2c_slaves_data_new.soc_state_of_charge, m_i2c_slaves_data_old.soc_state_of_charge);
					
				} else if((p_event->xfer_desc.address == BNO055_I2C_ADDR) && (i2c_operation_type == I2C_OPER_READ_BNO055_ID)){
					bno055_data_handler(m_i2c_slaves_data_new.bno055_id, m_i2c_slaves_data_old.bno055_id);
					
				} else if((p_event->xfer_desc.address == GEAR_CONT_I2C_ADDR) && (i2c_operation_type == I2C_OPER_READ_CURR_GEARS_STATE)){
					;
				} else{
					/*TODO: add else if statements to handle other adresses*/
					//data_handler(m_gear_status);
				}
            }
            m_xfer_done = true;
            break;
        default:
			NRF_LOG_WARNING("twi_handler: i2c communication to address= 0x%x returned event type= %d\r\n", p_event->xfer_desc.address, p_event->type);
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
       .clear_bus_init     = true
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

static bool i2cApp_poll_data(){
	
	bool   retcode     = true;
	
	// save the recent values as old value for comparison
	m_i2c_slaves_data_old.soc_state_of_charge      = m_i2c_slaves_data_new.soc_state_of_charge;
	m_i2c_slaves_data_old.bno055_id                = m_i2c_slaves_data_new.bno055_id;
	m_i2c_slaves_data_old.gearContoller_gears_state = m_i2c_slaves_data_new.gearContoller_gears_state;
	
	/* read new values */
	// read state of charge from SOC
	i2c_operation_type = I2C_OPER_READ_STATE_OF_CHARGE;
	if(!i2cApp_i2c_read(SOC_I2C_ADDR, STATE_OF_CHARGE_REG, 
		&(m_i2c_slaves_data_new.soc_state_of_charge), sizeof(m_i2c_slaves_data_new.soc_state_of_charge)))
	{
		NRF_LOG_ERROR("i2cApp_poll_data: i2cApp_i2c_read failed for SOC_I2C_ADDR\r\n");
		i2c_operation_type= I2C_OPER_UNKNOWN;
		retcode = false;
	}
	
	/*
	// read BNO inclination angles
	*TODO: create a function to read BNO inclination angles instead of ID*
	if(retcode){
		nrf_delay_ms(1);
		i2c_operation_type = I2C_OPER_READ_BNO055_ID;
		if(!i2cApp_i2c_read(BNO055_I2C_ADDR, BNO_ID_REG, &(m_i2c_slaves_data_new.bno055_id))){
			NRF_LOG_ERROR("i2cApp_poll_data: i2cApp_i2c_read failed for BNO055_I2C_ADDR\r\n");
			retcode = false;
		}
	}
	*/
	
	// read gear status
	if(retcode){
		nrf_delay_ms(1);
		i2c_operation_type = I2C_OPER_READ_CURR_GEARS_STATE;
		if(!i2cApp_i2c_read(GEAR_CONT_I2C_ADDR, GEAR_CONT_GEARSTATE_COMMAND, 
			(uint8_t*)(&(m_i2c_slaves_data_new.gearContoller_gears_state)), sizeof(m_i2c_slaves_data_new.gearContoller_gears_state)))
		{
			NRF_LOG_ERROR("i2cApp_poll_data: i2cApp_i2c_read failed for GEAR_CONTROLLER_I2C_ADDR\r\n");
			retcode = false;
		}
	}
	
	
	return retcode;
	
}

static void i2cApp_polling_timer_handler( void * callback_data){
	
	uint32_t* polling_request_id_p = (uint32_t*) callback_data;
	uint32_t  nrf_err              = NRF_SUCCESS;   
	
	/*TODO: figure out if this is necessary*/
	if (polling_request_id_p != NULL){
		NRF_LOG_DEBUG("polling_timer_handler: polling request with ID=%d timed out\r\n", *polling_request_id_p);
	}
	
	//stop scanning timer if the counter is alrady started previously
	if(polling_timer_started){
		(void)app_timer_stop(i2c_polling_timer_id);
	}
	
	//poll i2c data
	if (!i2cApp_poll_data()){
		/*if in standalone mode, connect to all advertized devices once scannig is finished*/
		NRF_LOG_ERROR("polling_timer_handler: i2cApp_poll_data failed \r\n");
	}
	
	//start timer againg
	nrf_err = app_timer_start(i2c_polling_timer_id, I2CAPP_MS_TO_TICK(I2CAPP_DATA_POLL_PERIOD_MS), (void*)NULL);
	if (nrf_err != NRF_SUCCESS)
	{
		NRF_LOG_ERROR("polling_timer_handler: app_timer_start failed with error = %d\r\n", nrf_err);
	}
}


static bool i2cApp_write_motor_controller_configs(){
	
	bool    retcode             = true;
	uint8_t front_gears_count   = user_defined_bike_config_data.crank_gears_count;
	uint8_t back_gears_count    = user_defined_bike_config_data.wheel_gears_count;
	uint8_t init_gears_state[2] = {0, 3}; //0 for front gear because there is only one gear supported right now, 3 is for the back gear index
	
	if(!i2cApp_i2c_write(GEAR_CONT_I2C_ADDR, GEAR_CONT_FRONT_GEARS_COUNT_COMMAND, 
		&front_gears_count, sizeof(front_gears_count)))
	{
		NRF_LOG_ERROR("i2cApp_write_motor_controller_configs: i2cApp_i2c_write failed for GEAR_CONT_FRONT_GEARS_COUNT_COMMAND\r\n");
		retcode = false;
	}
	
	if(retcode){
		if(!i2cApp_i2c_write(GEAR_CONT_I2C_ADDR, GEAR_CONT_BACK_GEARS_COUNT_COMMAND, 
			&back_gears_count, sizeof(back_gears_count)))
		{
			NRF_LOG_ERROR("i2cApp_write_motor_controller_configs: i2cApp_i2c_write failed for GEAR_CONT_BACK_GEARS_COUNT_COMMAND\r\n");
			retcode = false;
		}
	}
	
	if(retcode){
		if(!i2cApp_i2c_write(GEAR_CONT_I2C_ADDR, GEAR_CONT_FRONT_GEARS_POS_COMMAND, 
			&motor_cont_adc_pos_front_gears[0], front_gears_count))
		{
			NRF_LOG_ERROR("i2cApp_write_motor_controller_configs: i2cApp_i2c_write failed for GEAR_CONT_FRONT_GEARS_POS_COMMAND\r\n");
			retcode = false;
		}
	}
	
	if(retcode){
		if(!i2cApp_i2c_write(GEAR_CONT_I2C_ADDR, GEAR_CONT_BACK_GEARS_POS_COMMAND, 
			&motor_cont_adc_pos_back_gears[0], back_gears_count))
		{
			NRF_LOG_ERROR("i2cApp_write_motor_controller_configs: i2cApp_i2c_write failed for GEAR_CONT_BACK_GEARS_POS_COMMAND\r\n");
			retcode = false;
		}
	}
	
	if(retcode){
		if(!i2cApp_i2c_write(GEAR_CONT_I2C_ADDR, GEAR_CONT_INIT_GEARS_POS_COMMAND, 
			&init_gears_state[0], sizeof(init_gears_state)))
		{
			NRF_LOG_ERROR("i2cApp_write_motor_controller_configs: i2cApp_i2c_write failed for GEAR_CONT_INIT_GEARS_POS_COMMAND\r\n");
			retcode = false;
		}
	}
	
	return retcode;
}
/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/
void i2cApp_assing_new_meas_callback(new_meas_callback_f cb){
	
	new_meas_cb= cb;
	return;
}

uint8_t i2cApp_get_battery_level(void){
	
	return m_i2c_slaves_data_new.soc_state_of_charge;
	
}

bool i2cApp_write_desired_gears(uint8_t gear_index_front, uint8_t gear_index_back){
	bool   retcode = true;
	uint8_t desired_gears_array[2];
	
	/*TODO: check byte order*/
	desired_gears_array[0] = gear_index_front;
	desired_gears_array[1] = gear_index_back;
	
	if(!i2cApp_i2c_write(GEAR_CONT_I2C_ADDR, GEAR_CONT_WRITE_DES_GEARS_COMMAND, 
		&desired_gears_array[0], sizeof(desired_gears_array)))
	{
		NRF_LOG_ERROR("i2cApp_write_motor_controller_configs: i2cApp_i2c_write failed for GEAR_CONT_WRITE_DES_GEARS_COMMAND\r\n");
		retcode = false;
	}
	
	return retcode;
}


void i2cApp_wait(){
	//nrf_delay_ms(500);

	do
    {
		__WFE();
    }while (m_xfer_done == false);

    //read_gear_status();
	
}

/**
 * @brief I2C App initialization.
 */
bool i2cApp_init(void){
	
	bool         ret              = true;
	ret_code_t   nrf_err          = NRF_SUCCESS;
	
	twi_init();
	
	memset(&m_i2c_slaves_data_old, 0x00, sizeof(i2c_slaves_data_t));
	memset(&m_i2c_slaves_data_new, 0x00, sizeof(i2c_slaves_data_t));

	
	if (I2C_IN_SIMULATION_MODE){
		
		//read stateOfCharge register of SOC. For testing purpose only
		m_i2c_slaves_data_old.soc_state_of_charge= m_i2c_slaves_data_new.soc_state_of_charge;
		i2c_operation_type = I2C_OPER_READ_STATE_OF_CHARGE;
		if(!i2cApp_i2c_read(SOC_I2C_ADDR, STATE_OF_CHARGE_REG, 
			&(m_i2c_slaves_data_new.soc_state_of_charge), sizeof(m_i2c_slaves_data_new.soc_state_of_charge)))
		{
			NRF_LOG_ERROR("i2cApp_init: i2cApp_i2c_read failed for SOC_I2C_ADDR\r\n");
			i2c_operation_type = I2C_OPER_UNKNOWN;
		}
	
		ret = i2cSimDriver_init(&m_twi, (bool*)&m_xfer_done);
		
	} else{
		
		//write config data to motor controller
		if(!i2cApp_write_motor_controller_configs()){
			NRF_LOG_ERROR("i2cApp_init: i2cApp_write_motor_controller_configs failed\r\n");
		}
		
		//use timer to poll data from SOC after the specified time interval
		nrf_err = app_timer_create(&i2c_polling_timer_id,
									APP_TIMER_MODE_SINGLE_SHOT,
									i2cApp_polling_timer_handler);
		if (nrf_err != NRF_SUCCESS){
			NRF_LOG_ERROR("i2cApp_init: app_timer_create failed with error= %d\r\n", nrf_err);
			ret = false;
		}
		
	    if(ret){
			//pull data then start the timer
			i2cApp_polling_timer_handler(NULL);
		}
		
	}
		
	return ret;	
}
