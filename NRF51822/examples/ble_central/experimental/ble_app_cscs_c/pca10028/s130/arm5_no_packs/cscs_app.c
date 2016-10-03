/*
* Copyright (c) 2016 Shifty: Automatic Gear Selection System. All Rights Reserved.
 *
 * The code developed in this file is for the 4th year project: Shifty. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with the Shifty team.
 * Aportion of the source code here has been adopted from the Nordic Semiconductor' RSCS main application
 * Created by: Ali Ba Wazir
 *
 */

/**
 * @brief BLE Cycling Speed and Cadence Collector (CSCS) application file.
 *
 */
 
/**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include "cscs_app.h"
#include "app_error.h"
#define NRF_LOG_MODULE_NAME "CSCS APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "peer_manager.h"


/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/



/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static ble_cscs_c_t            m_ble_cscs_c;      /**< Structure used to identify the Cycling Speed and Cadence client module. */


/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
/**@brief Cycling Speed and Cadence Collector Handler.
 */
static void cscsApp_cscs_c_evt_handler(ble_cscs_c_t * p_csc_c, ble_cscs_c_evt_t * p_csc_c_evt)
{
    uint32_t err_code;

    switch (p_csc_c_evt->evt_type)
    {
        case BLE_CSCS_C_EVT_DISCOVERY_COMPLETE:
            // Initiate bonding.
            err_code = ble_cscs_c_handles_assign(&m_ble_cscs_c,
                                                 p_csc_c_evt->conn_handle,
                                                 &p_csc_c_evt->params.cscs_db);
            APP_ERROR_CHECK(err_code);

            err_code = pm_conn_secure(p_csc_c_evt->conn_handle, false);
            APP_ERROR_CHECK(err_code);

            // Cycling Speed and Cadence service discovered. Enable Cycling Speed and Cadence notifications.
            err_code = ble_cscs_c_csc_notif_enable(p_csc_c);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("Cycling Speed and Cadence service discovered \r\n");
            break;

        case BLE_CSCS_C_EVT_CSC_NOTIFICATION:
        {
            NRF_LOG_INFO("\r\n");
            if(p_csc_c_evt->params.csc_meas.is_wheel_rev_data_present){
								NRF_LOG_INFO("Comulative Wheel Revolution   = %d\r\n",
							                p_csc_c_evt->params.csc_meas.cumulative_wheel_revs);
							  NRF_LOG_INFO("Last Wheel Event time         = %d\r\n",
							                p_csc_c_evt->params.csc_meas.last_wheel_event_time);
						}
					  
					  if(p_csc_c_evt->params.csc_meas.is_crank_rev_data_present){
								NRF_LOG_INFO("Comulative Crank Revolution   = %d\r\n",
							                p_csc_c_evt->params.csc_meas.cumulative_crank_revs);
							NRF_LOG_INFO("Last Crank Event time         = %d\r\n",
							                p_csc_c_evt->params.csc_meas.last_crank_event_time);
						}
						
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
void cscsApp_on_ble_evt(const ble_evt_t *p_ble_evt){
		
	  // Check if the event if on the link for this instance
    if (m_ble_cscs_c.conn_handle != p_ble_evt->evt.gattc_evt.conn_handle){
			  NRF_LOG_INFO("cscsApp_on_ble_evt called with event not for this instance\r\n");
			  return;
    } else{
			  ble_cscs_c_on_ble_evt(&m_ble_cscs_c, p_ble_evt);
		}
	
}

/*Wrapper function*/
void cscsApp_on_db_disc_evt(const ble_db_discovery_evt_t *p_evt){
	
		ble_cscs_on_db_disc_evt(&m_ble_cscs_c, p_evt);
	
}


/*Wrapper function*/
void cscsApp_on_ble_event(const ble_evt_t * p_ble_evt)
{
	
	ble_cscs_c_on_ble_evt(&m_ble_cscs_c, p_ble_evt);
	
}
	  

/**
 * @brief Cycling Speed and Cadence collector initialization.
 */
void cscsApp_cscs_c_init(void)
{
    ble_cscs_c_init_t cscs_c_init_obj;
		//memset(&cscs_c_init_obj, 0, sizeof(ble_cscs_c_init_t));
	
    cscs_c_init_obj.evt_handler                      = cscsApp_cscs_c_evt_handler;
	  /*Added to accept all features from sensor*/
	  cscs_c_init_obj.feature = BLE_CSCS_FEATURE_WHEEL_REV_BIT 
                          	| BLE_CSCS_FEATURE_CRANK_REV_BIT
	                          | BLE_CSCS_FEATURE_MULTIPLE_SENSORS_BIT;

    uint32_t err_code = ble_cscs_c_init(&m_ble_cscs_c, &cscs_c_init_obj);
    APP_ERROR_CHECK(err_code);
}
