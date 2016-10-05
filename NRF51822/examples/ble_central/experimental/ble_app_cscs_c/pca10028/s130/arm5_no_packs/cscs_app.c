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

#define DFAULT_WHEEL_DIAMETER 0.5
#define PI                    3.14159265

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/
typedef struct{
		uint32_t value;
		bool     is_read;
}uint32_data_field_t;

typedef struct{
		double   value;
		bool     is_read;
}double_data_field_t;

typedef struct{
		uint32_data_field_t oldWheelRevolution;
		uint32_data_field_t oldCrankRevolution;
		double_data_field_t travelDistance;
	  double_data_field_t totalTravelDistance;
	  double_data_field_t speed; /*m/s*/
	  double_data_field_t travel_cadence;
		double_data_field_t oldWheelEventTime; 
		double_data_field_t oldCrankEventTime;
} cscs_instantanious_data_t;

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static ble_cscs_c_t                 m_ble_cscs_c;      /**< Structure used to identify the Cycling Speed and Cadence client module. */
static cscs_instantanious_data_t    cscs_instantanious_data;
static double                       wheelCircumference = 2*PI*DFAULT_WHEEL_DIAMETER;               /*The circumfrance is calculated by wheel diameter specified by user*/
/* TODO: create a function to update wheelCircumference by user data*/
/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/

static double process_wheel_data(uint32_t new_cumulative_wheel_revs, uint16_t new_wheel_event_time)
{
    /* This function called only when wheel Revolution Data Present
     * Last Wheel Event Time unit has resolution of 1/1024 seconds
     */
    
    double wheel_revolution_diff  = 0.0;
    double wheel_event_time_diff  = 0.0;
    double travel_speed           = 0.0;
    
    if (cscs_instantanious_data.oldWheelRevolution.value != 0) {
        wheel_revolution_diff = new_cumulative_wheel_revs - cscs_instantanious_data.oldWheelRevolution.value;
			/*TODO: create static function for updating*/
			  cscs_instantanious_data.travelDistance.value = cscs_instantanious_data.travelDistance.value + ((wheel_revolution_diff * wheelCircumference) / 1000.0);
        cscs_instantanious_data.travelDistance.is_read = false;
			  cscs_instantanious_data.totalTravelDistance.value = ((new_cumulative_wheel_revs * wheelCircumference) / 1000.0);
			  cscs_instantanious_data.totalTravelDistance.is_read= false;
    }
    if (cscs_instantanious_data.oldWheelEventTime.value != 0) {
        wheel_event_time_diff = ((new_wheel_event_time - cscs_instantanious_data.oldWheelEventTime.value));
    }
    if (wheel_event_time_diff > 0) {
        wheel_event_time_diff = wheel_event_time_diff / 1024.0; //time diff in seconds
        //speed is in units of m/s
        travel_speed = (((wheel_revolution_diff * wheelCircumference)/wheel_event_time_diff));

				cscs_instantanious_data.speed.value = travel_speed;
			  cscs_instantanious_data.speed.is_read = false;
    }
    
    cscs_instantanious_data.oldWheelRevolution.value = new_cumulative_wheel_revs;
		cscs_instantanious_data.oldWheelRevolution.is_read = false;
    cscs_instantanious_data.oldWheelEventTime.value = new_wheel_event_time;
		cscs_instantanious_data.oldWheelEventTime.is_read = false;
    
		return wheel_revolution_diff;
}

static double process_crank_data(uint16_t new_cumulative_crank_revs, uint16_t new_crank_event_time)
{
    /* This function called only when Crank Revolution data present
     * Last Crank Event Time unit has a resolution of 1/1024 seconds
     */
    
    double      crank_revolution_diff = 0.0;
    double      crank_event_time_diff = 0.0;
    double      travel_cadence        = 0.0;

    if (cscs_instantanious_data.oldCrankEventTime.value != 0) {
        crank_event_time_diff = new_crank_event_time - cscs_instantanious_data.oldCrankEventTime.value;
    }
    if (cscs_instantanious_data.oldCrankRevolution.value != 0) {
        crank_revolution_diff = new_cumulative_crank_revs - cscs_instantanious_data.oldCrankRevolution.value;
    }
    if (crank_event_time_diff > 0) {
        crank_event_time_diff = crank_event_time_diff / 1024.0;
        travel_cadence = ((crank_revolution_diff / crank_event_time_diff) * 60); //rp..
    }

    cscs_instantanious_data.oldCrankRevolution.value = new_cumulative_crank_revs;
		cscs_instantanious_data.oldCrankRevolution.is_read = false;
    cscs_instantanious_data.oldCrankEventTime.value = new_crank_event_time;
		cscs_instantanious_data.oldCrankEventTime.is_read= false;
		cscs_instantanious_data.travel_cadence.value = travel_cadence;
		cscs_instantanious_data.travel_cadence.is_read = false;
		
		return crank_revolution_diff;
}

static void cscsApp_decode_cscs_meas_data ( ble_cscs_c_meas_t *csc_meas_data)
{
    double            wheel_rev_diff              = 0.0;
		double            crank_rev_diff              = 0.0;
    double            wheel_to_crank_diff_ratio   = 0.0;
	
	  if (csc_meas_data == NULL){
				NRF_LOG_ERROR("cscsApp_decode_cscs_meas_data called with NULL argument\r\n");
			  return;
		}
    
    if (csc_meas_data->is_wheel_rev_data_present)
    {
        wheel_rev_diff = process_wheel_data(csc_meas_data->cumulative_wheel_revs, csc_meas_data->last_wheel_event_time);
        if (csc_meas_data->is_crank_rev_data_present)
        {
            crank_rev_diff = process_crank_data(csc_meas_data->cumulative_crank_revs, csc_meas_data->last_crank_event_time);
            if (crank_rev_diff > 0) {
                wheel_to_crank_diff_ratio = wheel_rev_diff/crank_rev_diff;
                NRF_LOG_INFO("Wheel to crank diff ratio = %f \r\n",wheel_to_crank_diff_ratio);
            }
        }
    }
    else
    {
        if (csc_meas_data->is_crank_rev_data_present)
        {
						crank_rev_diff = process_crank_data(csc_meas_data->cumulative_crank_revs, csc_meas_data->last_crank_event_time);
					  NRF_LOG_INFO("Crank revolution diff = %l \r\n",crank_rev_diff);
        }
    }
}

static void cscsApp_debug_print_inst_data(){
		
		NRF_LOG_INFO("\r\n");
		NRF_LOG_INFO("oldWheelRevolution  = %d \r\n", (uint32_t)cscs_instantanious_data.oldWheelRevolution.value);
		NRF_LOG_INFO("oldCrankRevolution  = %d \r\n", (uint32_t)cscs_instantanious_data.oldCrankRevolution.value);
		NRF_LOG_INFO("travelDistance      = %d \r\n", (uint32_t)cscs_instantanious_data.travelDistance.value);
		NRF_LOG_INFO("totalTravelDistance = %d \r\n", (uint32_t)cscs_instantanious_data.totalTravelDistance.value);
		NRF_LOG_INFO("speed               = %d \r\n", (uint32_t)cscs_instantanious_data.speed.value);
		NRF_LOG_INFO("travel_cadence      = %d \r\n", (uint32_t)cscs_instantanious_data.travel_cadence.value);
		NRF_LOG_INFO("oldWheelEventTime   = %d \r\n", (uint32_t)cscs_instantanious_data.oldWheelEventTime.value);
		NRF_LOG_INFO("oldCrankEventTime   = %d \r\n", (uint32_t)cscs_instantanious_data.oldCrankEventTime.value);
		
}


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
            cscsApp_decode_cscs_meas_data(&(p_csc_c_evt->params.csc_meas));
					  cscsApp_debug_print_inst_data();
					  /*
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
					*/
						
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
		memset(&cscs_c_init_obj, 0, sizeof(ble_cscs_c_init_t));
		
		memset (&cscs_instantanious_data, 0, sizeof(cscs_instantanious_data_t));
	
    cscs_c_init_obj.evt_handler                      = cscsApp_cscs_c_evt_handler;
	  /*Added to accept all features from sensor*/
	  cscs_c_init_obj.feature = BLE_CSCS_FEATURE_WHEEL_REV_BIT 
                          	| BLE_CSCS_FEATURE_CRANK_REV_BIT
	                          | BLE_CSCS_FEATURE_MULTIPLE_SENSORS_BIT;

    uint32_t err_code = ble_cscs_c_init(&m_ble_cscs_c, &cscs_c_init_obj);
    APP_ERROR_CHECK(err_code);
		
}
