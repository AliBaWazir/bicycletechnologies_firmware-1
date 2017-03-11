/*
* Copyright (c) 2016 Shifty: Automatic Gear Selection System. All Rights Reserved.
 *
 * The code developed in this file is for the 4th year project: Shifty. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with the Shifty team.
 * Aportion of the source code here has been adopted from the Nordic Semiconductor' RSCS main application
 * Created by: Ali Ba Wazir
 * Oct 2016
 */

/**
 * @brief BLE Heart Rate Collector (HRC) application file.
 *
 */
 
/**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include "app_error.h"
#define NRF_LOG_MODULE_NAME "HRC APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "peer_manager.h"

#include "hrs_app.h"
#include "ble_hrs_c.h"
#include "ble_bas_c.h" /*TODO: if the battery app is needed for all sensors, creat a separate file for it*/


/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
//[http://www.heart.org/HEARTORG/HealthyLiving/PhysicalActivity/FitnessBasics/Target-Heart-Rates_UCM_434341_Article.jsp#.WMMsC_nyuUk] 
#define AVERAGE_HR_READING_BPM         80   //This is the average human heart rate during rest


/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static ble_hrs_c_t        	  m_ble_hrs_c;              /**< Structure used to identify the heart rate client module. */
static ble_bas_c_t        	  m_ble_bas_c;              /**< Structure used to identify the Battery Service client module. */
static uint16_data_field_t    inst_hr_value;            // instantaious value of heart rate reading

static new_meas_callback_f    new_meas_cb    = NULL;    // Function pointer to the function to be called when a new measuremnt is obtained
/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
/**@brief Heart Rate Collector Handler.
 */
static void hrsApp_hrs_c_evt_handler(ble_hrs_c_t * p_hrs_c, ble_hrs_c_evt_t * p_hrs_c_evt)
{
    uint32_t err_code;

    switch (p_hrs_c_evt->evt_type)
    {
        case BLE_HRS_C_EVT_DISCOVERY_COMPLETE:
		{
            err_code = ble_hrs_c_handles_assign(p_hrs_c ,
                                                p_hrs_c_evt->conn_handle,
                                                &p_hrs_c_evt->params.peer_db);
            APP_ERROR_CHECK(err_code);

            // Initiate bonding.
            err_code = pm_conn_secure(p_hrs_c_evt->conn_handle, false);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }

            // Heart rate service discovered. Enable notification of Heart Rate Measurement.
            err_code = ble_hrs_c_hrm_notif_enable(p_hrs_c);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_DEBUG("Heart rate service discovered \r\n");
        }break; //BLE_HRS_C_EVT_DISCOVERY_COMPLETE

        case BLE_HRS_C_EVT_HRM_NOTIFICATION:
        {	
			if(p_hrs_c_evt->params.hrm.hr_value > MAX_BLE_SENSOR_MEAS){
				inst_hr_value.value = MAX_BLE_SENSOR_MEAS;
			} else{
				inst_hr_value.value = p_hrs_c_evt->params.hrm.hr_value;
			}
            
			inst_hr_value.is_read= false;
			
			NRF_LOG_INFO("New Heart Rate reading= %d (bpm)\r\n", inst_hr_value.value);

			//call the new measurement callback 
			if(new_meas_cb != NULL){
				new_meas_cb(SPI_AVAIL_FLAG_HR, true);
			}
        }break;//BLE_HRS_C_EVT_HRM_NOTIFICATION

        default:
            break;
    }
}


/* TODO: id battery service to be functioning for all sensors,
 * create a seperate app and move this function to it
 */
/**@brief Battery level Collector Handler. **/
static void hrsApp_bas_c_evt_handler(ble_bas_c_t * p_bas_c, ble_bas_c_evt_t * p_bas_c_evt)
{
    uint32_t err_code;

    switch (p_bas_c_evt->evt_type)
    {
        case BLE_BAS_C_EVT_DISCOVERY_COMPLETE:
            err_code = ble_bas_c_handles_assign(p_bas_c,
                                                p_bas_c_evt->conn_handle,
                                                &p_bas_c_evt->params.bas_db);
            APP_ERROR_CHECK(err_code);

            // Initiate bonding.
            err_code = pm_conn_secure(p_bas_c_evt->conn_handle, false);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }

            // Batttery service discovered. Enable notification of Battery Level.
            NRF_LOG_DEBUG("Battery Service discovered. Reading battery level.\r\n");

            err_code = ble_bas_c_bl_read(p_bas_c);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_DEBUG("Enabling Battery Level Notification. \r\n");
            err_code = ble_bas_c_bl_notif_enable(p_bas_c);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_BAS_C_EVT_BATT_NOTIFICATION:
        {
            NRF_LOG_DEBUG("Battery Level received %d %%\r\n", p_bas_c_evt->params.battery_level);

            break;
        }

        case BLE_BAS_C_EVT_BATT_READ_RESP:
        {
            NRF_LOG_INFO("Battery Level Read as %d %%\r\n", p_bas_c_evt->params.battery_level);
            break;
        }

        default:
            break;
    }
}


/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/
/*Wrapper function*/
void hrsApp_on_ble_evt(const ble_evt_t *p_ble_evt){
	// Check if the event if on the link for this instance
    if (m_ble_hrs_c.conn_handle != p_ble_evt->evt.gattc_evt.conn_handle){
		//NRF_LOG_DEBUG("hrsApp_on_ble_evt: called with event not for this instance. event ID=0x%x\r\n", p_ble_evt->header.evt_id);
		//NRF_LOG_DEBUG("m_ble_hrs_c.conn_handle= %d, gattc_evt.conn_handle= %d\r\n",
		//				m_ble_hrs_c.conn_handle, p_ble_evt->evt.gattc_evt.conn_handle);
		return;
    } else{
		ble_hrs_c_on_ble_evt(&m_ble_hrs_c, p_ble_evt);
	}

}

/*Wrapper function*/
void hrsApp_on_db_disc_evt(const ble_db_discovery_evt_t *p_evt){
	
	ble_hrs_on_db_disc_evt(&m_ble_hrs_c, p_evt);
	/* TODO: id battery service to be functioning for all sensors,
	 * create a seperate app and add a new argument to this function to 
	 * specify which sensor was disconnected
	 */
    ble_bas_on_db_disc_evt(&m_ble_bas_c, p_evt);
}


/*Wrapper function*/
void hrsApp_on_ble_event(const ble_evt_t * p_ble_evt)
{
	
	ble_hrs_c_on_ble_evt(&m_ble_hrs_c, p_ble_evt);
	/* TODO: id battery service to be functioning for all sensors,
	 * create a seperate app and add a new argument to this function to 
	 * specify which sensor was disconnected
	 */
	ble_bas_c_on_ble_evt(&m_ble_bas_c, p_ble_evt);
}

//function to return current instantanious heart rate
//note: this function is only called by SPI app
uint8_t hrsApp_get_current_hr_bpm(void){
	
	uint8_t ret_value = 0;
	
	if (inst_hr_value.is_read){
		return SPI_NO_NEW_MEAS;
	} else{
		ret_value= (uint8_t)inst_hr_value.value;
		//set the existing measurement as read
		inst_hr_value.is_read= true;
	}		
	return ret_value;
}

//function to return deviation of current HR reading relative to the average value
float hrsApp_get_curr_hr_deviation(void){
	
	return (inst_hr_value.value-AVERAGE_HR_READING_BPM)/(AVERAGE_HR_READING_BPM);
}

void hrsApp_assing_new_meas_callback(new_meas_callback_f cb){
	
	new_meas_cb= cb;
	return;
}

/**
 * @brief Heart Rate collector initialization.
 */
bool hrsApp_hrs_c_init(void)
{
	bool     ret_code = true;
	uint32_t err_code = NRF_SUCCESS;
	ble_hrs_c_init_t  hrs_c_init_obj;
	
	memset(&hrs_c_init_obj, 0x00, sizeof(ble_hrs_c_init_t));
	memset(&inst_hr_value, 0x00, sizeof(uint16_data_field_t));
			
    hrs_c_init_obj.evt_handler = hrsApp_hrs_c_evt_handler;
	
    err_code = ble_hrs_c_init(&m_ble_hrs_c, &hrs_c_init_obj);
    if ( err_code !=NRF_SUCCESS){
		NRF_LOG_INFO("ble_hrs_c_init failed with error= %d\r\n", err_code);
		ret_code = false;
	}



	return ret_code;	
}
