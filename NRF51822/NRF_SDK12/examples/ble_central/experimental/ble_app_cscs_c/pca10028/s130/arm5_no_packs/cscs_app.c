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

#define DFAULT_WHEEL_DIAMETER_CM 50
#define PI                       3.14159265

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/
typedef struct{
	uint32_t value;
	bool     is_read;
}uint32_data_field_t;

typedef struct{
	uint16_t value;
	bool     is_read;
}uint16_data_field_t;

typedef struct{
	double   value;
	bool     is_read;
}double_data_field_t;

typedef struct{
	uint32_t            first_wheel_revolutions; //This number records the initial wheel revolutions state reported from sensor . Used for debugging
	uint32_data_field_t oldWheelRevolutions;
	uint16_data_field_t oldCrankRevolutions;
	uint16_data_field_t oldWheelEventTime; 
	uint16_data_field_t oldCrankEventTime;
	double_data_field_t travelDistance_m;       //this is the traveled distance for the current cycling session
    double_data_field_t totalTravelDistance;    // This is the total traveled distance since first use of bike with sensor
	double_data_field_t wheel_speed_mps; /*m/s*/
	double_data_field_t crank_cadence_rpm;
} cscs_instantanious_data_t;

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static ble_cscs_c_t                 m_ble_cscs_c;      /**< Structure used to identify the Cycling Speed and Cadence client module. */
static cscs_instantanious_data_t    cscs_instantanious_data;
static double                       wheel_circumference_cm = 2*PI*DFAULT_WHEEL_DIAMETER_CM;               /*The circumfrance is calculated by wheel diameter specified by user*/
//static uint32_t                     wheel_revolution_offset = 0;                                     /*Offset from the first reading*/
//static uint16_t                     crank_revolution_offset = 0;
//static uint16_t                     wheel_event_time_offset = 0;
//static uint16_t                     crank_event_time_offset = 0;

/* TODO: create a function to update wheelCircumference by user data*/
/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/

static int32_t process_wheel_data(uint32_t new_cumulative_wheel_revs, uint16_t new_wheel_event_time)
{
    /* This function called only when wheel Revolution Data Present
     * Wheel Event Time unit has resolution of 1/1024 seconds
     */
    
    int32_t wheel_revolutions_diff   = 0;
    double wheel_event_time_diff_s   = 0.0;
    
	if (cscs_instantanious_data.first_wheel_revolutions <=0){
		cscs_instantanious_data.first_wheel_revolutions = new_cumulative_wheel_revs;
	}
		
    if (cscs_instantanious_data.oldWheelRevolutions.value > 0) {
        // Assuming that the comulative wheel revolutions will never exceed the max value 4,294,967,295
		wheel_revolutions_diff = new_cumulative_wheel_revs - cscs_instantanious_data.oldWheelRevolutions.value;
		if (wheel_revolutions_diff < 0){
			NRF_LOG_ERROR("Sensor error: new wheel revolutions < old wheel revolutions \r\n");
			return wheel_revolutions_diff;
		} else {
			/*TODO: create static function for updating*/
			cscs_instantanious_data.travelDistance_m.value = cscs_instantanious_data.travelDistance_m.value + ((wheel_revolutions_diff * wheel_circumference_cm)/100.0);
			cscs_instantanious_data.travelDistance_m.is_read = false;
			cscs_instantanious_data.totalTravelDistance.value =(new_cumulative_wheel_revs * wheel_circumference_cm)/100.0;
		    cscs_instantanious_data.totalTravelDistance.is_read = false;
		}
    }
	
	if (cscs_instantanious_data.oldWheelEventTime.value > 0) {
        if (new_wheel_event_time < cscs_instantanious_data.oldWheelEventTime.value){
			//Time roll off
			wheel_event_time_diff_s = (65535 + new_wheel_event_time - cscs_instantanious_data.oldWheelEventTime.value)/1024.0; //time diff in seconds
		} else{
			wheel_event_time_diff_s = (new_wheel_event_time - cscs_instantanious_data.oldWheelEventTime.value)/1024.0; //time diff in seconds
		}
    }
				
	if (wheel_event_time_diff_s > 0) {
        //speed is in units of m/s
        cscs_instantanious_data.wheel_speed_mps.value = (((wheel_revolutions_diff * wheel_circumference_cm)/100.0)/wheel_event_time_diff_s);
		cscs_instantanious_data.wheel_speed_mps.is_read = false;
    } else{
		//wheel is stopped. Set speed at 0 m/s
		cscs_instantanious_data.wheel_speed_mps.value = 0.0;
		cscs_instantanious_data.wheel_speed_mps.is_read = false;
	}
    
	//update the old revolution and envent time fields with new data
    cscs_instantanious_data.oldWheelRevolutions.value = new_cumulative_wheel_revs;
	cscs_instantanious_data.oldWheelRevolutions.is_read = false;
    cscs_instantanious_data.oldWheelEventTime.value = new_wheel_event_time;
	cscs_instantanious_data.oldWheelEventTime.is_read = false;
    
	return wheel_revolutions_diff;
}

static int32_t process_crank_data(uint16_t new_cumulative_crank_revs, uint16_t new_crank_event_time)
{
    /* This function called only when Crank Revolution data present
     * Last Crank Event Time unit has a resolution of 1/1024 seconds
     */
    
    int32_t     crank_revolutions_diff   = 0;
    double      crank_event_time_diff_s = 0.0;
    double      cadence_rpm             = 0.0;

	if (cscs_instantanious_data.oldCrankRevolutions.value > 0) {
        // Assuming that the comulative crank revolutions will never exceed the max value 65,535
		crank_revolutions_diff = new_cumulative_crank_revs - cscs_instantanious_data.oldCrankRevolutions.value;
		if (crank_revolutions_diff <0){
			NRF_LOG_ERROR("Sensor error: new crank revolutions < old crank revolutions \r\n");
			return crank_revolutions_diff;
		}
    }
		
    if (cscs_instantanious_data.oldCrankEventTime.value > 0) {
        if (new_crank_event_time < cscs_instantanious_data.oldCrankEventTime.value){
			//Time roll off
			crank_event_time_diff_s = (65535 + new_crank_event_time - cscs_instantanious_data.oldCrankEventTime.value)/1024.0;
		} else{
			crank_event_time_diff_s = (new_crank_event_time - cscs_instantanious_data.oldCrankEventTime.value)/1024.0;
		}
    } 
    
    if (crank_event_time_diff_s > 0) {
        //cadence is in rpm
		cadence_rpm = (crank_revolutions_diff /(crank_event_time_diff_s/60) );
    }

    cscs_instantanious_data.oldCrankRevolutions.value = new_cumulative_crank_revs;
	cscs_instantanious_data.oldCrankRevolutions.is_read = false;
    cscs_instantanious_data.oldCrankEventTime.value = new_crank_event_time;
	cscs_instantanious_data.oldCrankEventTime.is_read= false;
	cscs_instantanious_data.crank_cadence_rpm.value = cadence_rpm;
	cscs_instantanious_data.crank_cadence_rpm.is_read = false;
		
	return crank_revolutions_diff;
}

static cscsApp_ret_code_e cscsApp_decode_cscs_meas_data ( ble_cscs_c_meas_t *csc_meas_data)
{
    int32_t             wheel_rev_diff              = 0;
	int16_t             crank_rev_diff              = 0;
    double              wheel_to_crank_diff_ratio   = 0.0;
	cscsApp_ret_code_e  ret_code                    = CSCS_APP_RET_CODE_SUCCESS;
	
	if (csc_meas_data == NULL){
		NRF_LOG_ERROR("cscsApp_decode_cscs_meas_data called with NULL argument\r\n");
	    return CSCS_APP_RET_CODE_BAD_ARGUMENT;
	}
    
    if (csc_meas_data->is_wheel_rev_data_present){
        wheel_rev_diff = process_wheel_data(csc_meas_data->cumulative_wheel_revs, csc_meas_data->last_wheel_event_time);
		if (wheel_rev_diff < 0){
			// sensor error
			ret_code = CSCS_APP_RET_CODE_SENSOR_ERROR;
		} else {
			
			if (csc_meas_data->is_crank_rev_data_present)
			{
				crank_rev_diff = process_crank_data(csc_meas_data->cumulative_crank_revs, csc_meas_data->last_crank_event_time);
				if (crank_rev_diff < 0) {
					// sensor error
					ret_code = CSCS_APP_RET_CODE_SENSOR_ERROR;
				} else {
					wheel_to_crank_diff_ratio = wheel_rev_diff/crank_rev_diff;
					NRF_LOG_INFO("Wheel to crank diff ratio = %d \r\n",wheel_to_crank_diff_ratio);
				}
			}
			
		}
    }
    else{
        if (csc_meas_data->is_crank_rev_data_present)
        {
			crank_rev_diff = process_crank_data(csc_meas_data->cumulative_crank_revs, csc_meas_data->last_crank_event_time);
			if (crank_rev_diff < 0){
				// sensor error
				ret_code = CSCS_APP_RET_CODE_SENSOR_ERROR;
			} else {
				NRF_LOG_INFO("Crank revolution diff = %d \r\n",crank_rev_diff);
			}
        }
    }
	
	return ret_code;
}

static void cscsApp_debug_print_inst_data(){
		
		NRF_LOG_INFO("------------------------------------------------------\r\n");
		NRF_LOG_INFO("travel distance             = %d (m)\r\n", (uint32_t)cscs_instantanious_data.travelDistance_m.value);
		NRF_LOG_INFO("wheel speed                = %d (m/s)\r\n", (uint32_t)cscs_instantanious_data.wheel_speed_mps.value);
		NRF_LOG_INFO("crank cadence            = %d (rpm)\r\n", (uint32_t)cscs_instantanious_data.crank_cadence_rpm.value);
		NRF_LOG_INFO("old wheel revolution  = %d \r\n", (uint32_t)cscs_instantanious_data.oldWheelRevolutions.value);
		NRF_LOG_INFO("old crank revolution   = %d \r\n", (uint32_t)cscs_instantanious_data.oldCrankRevolutions.value);
		NRF_LOG_INFO("old wheel event time = %d \r\n", (uint32_t)cscs_instantanious_data.oldWheelEventTime.value);
		NRF_LOG_INFO("old crank event time  = %d \r\n", (uint32_t)cscs_instantanious_data.oldCrankEventTime.value);
		
}


/**@brief Cycling Speed and Cadence Collector Handler.
 */
static void cscsApp_cscs_c_evt_handler(ble_cscs_c_t * p_csc_c, ble_cscs_c_evt_t * p_csc_c_evt)
{
    uint32_t           err_code;
	cscsApp_ret_code_e ret_code = CSCS_APP_RET_CODE_SUCCESS;

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
            ret_code = cscsApp_decode_cscs_meas_data(&(p_csc_c_evt->params.csc_meas));
			if (ret_code != CSCS_APP_RET_CODE_SUCCESS){
				NRF_LOG_ERROR("cscsApp_decode_cscs_meas_data returned error= %d \r\n", ret_code);
			} else{
				cscsApp_debug_print_inst_data();
			}
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
	
    cscs_c_init_obj.evt_handler = cscsApp_cscs_c_evt_handler;
	/*Added to accept all features from sensor*/
	cscs_c_init_obj.feature = BLE_CSCS_FEATURE_WHEEL_REV_BIT 
                          	 | BLE_CSCS_FEATURE_CRANK_REV_BIT
	                         | BLE_CSCS_FEATURE_MULTIPLE_SENSORS_BIT;

    uint32_t err_code = ble_cscs_c_init(&m_ble_cscs_c, &cscs_c_init_obj);
    APP_ERROR_CHECK(err_code);
		
}
