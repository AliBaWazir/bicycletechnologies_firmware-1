/*
* Copyright (c) 2016 Shifty: Automatic Gear Selection System. All Rights Reserved.
 *
 * The code developed in this file is for the 4th year project: Shifty. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with the Shifty team.
 * Created by: Ali Ba Wazir
 * Nov 2016
 */

/**
 * @brief BLE Connection Manager application file.
 *
 */
 
/**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include <string.h>
#include "nrf_delay.h"
#include "app_error.h"
#include "fstorage.h"
#include "ble_conn_state.h"
#include "peer_manager.h"
#include "app_timer.h"
#include "bsp.h" /*TODO: move all bsp code from main file to this file*/
#define NRF_LOG_MODULE_NAME "CONN_MANAGER APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "connection_manager_app.h"

/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define ADVERTISED_DEVICES_COUNT_MAX   10          //max of advertised devices to store data of
#define MAX_CONNECTIONS_COUNT          2           //same as CENTRAL_LINK_COUNT

#define SCAN_INTERVAL                  0x00A0      /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                    0x0050      /**< Determines scan window in units of 0.625 millisecond. */

#define MIN_CONNECTION_INTERVAL   MSEC_TO_UNITS(7.5, UNIT_1_25_MS)   /**< Determines minimum connection interval in millisecond. */
#define MAX_CONNECTION_INTERVAL   MSEC_TO_UNITS(30, UNIT_1_25_MS)    /**< Determines maximum connection interval in millisecond. */
#define SLAVE_LATENCY             0                                  /**< Determines slave latency in counts of connection events. */
#define SUPERVISION_TIMEOUT       MSEC_TO_UNITS(4000, UNIT_10_MS)    /**< Determines supervision time-out in units of 10 millisecond. */

#define BETWEEN_CONNECTIONS_DELAY_MS   5000        //sequent connections will be performed with a delay between them to allow handling connections

#define CONN_MANAGER_APP_STANDALONE_MODE      1    /*This boolean is set to true only if connection_manager_app
													*is in standalone mode. That means no interaction with SPI will be made
													**/
													
#define APP_TIMER_PRESCALER        0               /**< Value of the RTC1 PRESCALER register. If changed, remember to change prescaler in main.c*/
#define CONNMANAGER_MS_TO_TICK(MS) (APP_TIMER_TICKS(100, APP_TIMER_PRESCALER) * (MS / 100))

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/

typedef struct{
	advertised_device_type_e   device_type;
	int8_t                     rssi;
	ble_gap_addr_t             peer_addr;
} advertised_devices_data_t;

typedef struct{
	uint8_t                    count;
	advertised_devices_data_t  advertised_devices_data [ADVERTISED_DEVICES_COUNT_MAX];
} advertised_devices_t;

/*
typedef struct{
	advertised_device_type_e   device_type;
	ble_gap_conn_params_t      conn_params;
} periph_set_conn_params_t;
*/

typedef enum
{
    BLE_NO_SCAN,        /**< No advertising running. */
    BLE_WHITELIST_SCAN, /**< Advertising with whitelist. */
    BLE_FAST_SCAN,      /**< Fast advertising running. */
} ble_scan_mode_t;

typedef struct{
	uint8_t queued_request_count;
	uint8_t queue_request[MAX_CONNECTIONS_COUNT];  //connection requests will be queued in terms of device IDs.
}connection_requests_queue_t;

/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static bool                      m_memory_access_in_progress      = false; /**< Flag to keep track of ongoing operations on persistent memory. */
static ble_gap_scan_params_t     m_scan_param;                             /**< Scan parameters requested for scanning and connection. */
static ble_scan_mode_t           m_scan_mode = BLE_FAST_SCAN;              /**< Scan mode used by application. */
static volatile bool             m_whitelist_temporarily_disabled = false; /**< True if whitelist has been temporarily disabled. */

/**
 * @brief Connection parameters requested for connection.
 */
static ble_gap_conn_params_t cscs_connection_params =
{
    (uint16_t)MIN_CONNECTION_INTERVAL, // Default Minimum connection.
    (uint16_t)MAX_CONNECTION_INTERVAL, // Default Maximum connection.
    (uint16_t)SLAVE_LATENCY,           // Default Slave latency.
    (uint16_t)SUPERVISION_TIMEOUT      // Default Supervision time-out.
};

static ble_gap_conn_params_t hr_connection_params =
{
    (uint16_t)MIN_CONNECTION_INTERVAL, // Default Minimum connection.
    (uint16_t)MAX_CONNECTION_INTERVAL, // Default Maximum connection.
    (uint16_t)SLAVE_LATENCY,           // Default Slave latency.
    (uint16_t)SUPERVISION_TIMEOUT      // Default Supervision time-out.
};

static ble_gap_conn_params_t phone_connection_params =
{
    (uint16_t)MIN_CONNECTION_INTERVAL, // Default Minimum connection.
    (uint16_t)MAX_CONNECTION_INTERVAL, // Default Maximum connection.
    (uint16_t)SLAVE_LATENCY,           // Default Slave latency.
    (uint16_t)SUPERVISION_TIMEOUT      // Default Supervision time-out.
};

static advertised_devices_t      advertised_devices;                                 /* Structure used to contain data for advertised sensors results after ble scan */
static advertised_devices_t*     advertised_devices_p       = &advertised_devices;   // for debugging

/*static periph_set_conn_params_t  periph_set_conn_params[MAX_CONNECTIONS_COUNT];     * Array to hold connection params for HR and CSCS sensors.
																					  * All devices in the same category assumed to have the same connection params
																					  * Array entries are indexes by conn handler
																					  **/
static advertised_device_type_e device_type_at_conn_handler [MAX_CONNECTIONS_COUNT];
APP_TIMER_DEF(scanning_timer_id);

static bool cscs_peripheral_connected  = false;           //this boolean is set to true when a CSCS sensor is connected "paired" 
static bool hr_peripheral_connected    = false;           //this boolean is set to true when a HR sensor is connected "paired" 
static bool phone_peripheral_connected = false;           //this boolean is set to true when a phone peripheral is connected "paired" 

static bool busy_connecting            = false;           //set to true when softdevice stack is busy connecting to a peripheral
														  //reset to false when BLE_GAP_EVT_CONNECTED is received

static connection_requests_queue_t connection_requests_queue;       //queued connection request in terms of advertising devices IDs


/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
static ble_gap_addr_t* connManagerApp_get_peer_addr(uint8_t advertised_device_index){
	
	ble_gap_addr_t* ble_gap_addr = NULL;
	
	if (advertised_device_index<ADVERTISED_DEVICES_COUNT_MAX){
		ble_gap_addr = &(advertised_devices.advertised_devices_data[advertised_device_index].peer_addr);
	}
	
	return ble_gap_addr;
}

static bool connManagerApp_is_peer_address_matching(const uint8_t* existing_peer_address, const uint8_t* new_peer_address){
	
	/*TODO: create a generic function to compare arrays given the length*/
	uint8_t i = 0;
	bool    is_matching = true;
	
	for (i=0; (i< BLE_GAP_ADDR_LEN) && (is_matching); i++){
		if (existing_peer_address[i] != new_peer_address[i]){
			is_matching = false;
		}
	}
		
	return is_matching;
}


static bool connManagerApp_connect_all(){
	
	uint8_t        i            = 0;
	bool           ret_code     = true;
	
	//connect to all advertised devices
	for (i=0; (i<(advertised_devices.count)) && (ret_code); i++){
		
		advertised_device_type_e  tmp_device_type          = ADVERTISED_DEVICE_TYPE_UNKNOWN;
		bool                      device_already_connected = false;
		
		//skip peripheral that is already connected
		tmp_device_type = advertised_devices.advertised_devices_data[i].device_type;
		switch (tmp_device_type){
			case ADVERTISED_DEVICE_TYPE_CSCS_SENSOR:
				if(cscs_peripheral_connected){
					device_already_connected = true;
				}
				break;
			case ADVERTISED_DEVICE_TYPE_HRS_SENSOR:
				if(hr_peripheral_connected){
					device_already_connected = true;
				}
				break;
			case ADVERTISED_DEVICE_TYPE_PHONE:
				if(phone_peripheral_connected){
					device_already_connected = true;
				}
				break;
			default:
				NRF_LOG_ERROR("connManagerApp_connect_all: advertised device is of unknown type=%d \r\n", tmp_device_type);
				ret_code = false;
			break;
		}
		
		if (!device_already_connected){
			if (!connManagerApp_advertised_device_connect(i)){
				NRF_LOG_ERROR("connManagerApp_init: failed to connect to advertised device with index= %d\r\n",i);
				ret_code = false;
			} else{
				/*TODO: figure out if this delay is a must*/
				//delay to allow handling the connection event. This is replaced by storing connection requests in queue
				nrf_delay_ms(BETWEEN_CONNECTIONS_DELAY_MS);
			}	
		}
		
	}
	
	return ret_code;
}

static void scanning_timer_handler( void * callback_data){
	
	uint32_t* scanning_request_id_p = (uint32_t*) callback_data;
	uint32_t  err                   = NRF_SUCCESS;   
	
	if (scanning_request_id_p != NULL){
		NRF_LOG_DEBUG("scanning_timer_handler: scanning request with ID=%d timed out\r\n", *scanning_request_id_p);
	}
	
	// Stop scanning.
	NRF_LOG_DEBUG("scanning_timer_handler: stopping scan.\r\n");
	err = sd_ble_gap_scan_stop();

	if (err != NRF_SUCCESS){
		NRF_LOG_ERROR("scanning_timer_handler: sd_ble_gap_scan_stop failed, reason %d\r\n", err);
	}
	
	//stop scanning timer
	(void)app_timer_stop(scanning_timer_id);
	
	if (CONN_MANAGER_APP_STANDALONE_MODE){
		/*if in standalone mode, connect to all advertized devices once scannig is finished*/
		if(!connManagerApp_connect_all()){
			NRF_LOG_ERROR("scanning_timer_handler: connManagerApp_connect_all failed \r\n");
		}
	}
}

static void connManagerApp_handle_queue_conn_requests(){
	
	uint8_t current_queued_conn_count = connection_requests_queue.queued_request_count;
	//ensure that there are only MAX_CONNECTIONS_COUNT-1 connection requests in queue
	if ( current_queued_conn_count < MAX_CONNECTIONS_COUNT){
		//serve last connection request in queue
		if(!connManagerApp_advertised_device_connect(connection_requests_queue.queue_request[current_queued_conn_count-1])){
			NRF_LOG_ERROR("connManagerApp_handle_queue_conn_requests: connManagerApp_advertised_device_connect failed \r\n");
		}else{
			connection_requests_queue.queued_request_count--;
		}
	}
}
/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/

void connManagerApp_debug_print_conn_params(const ble_gap_conn_params_t* conn_params){
	NRF_LOG_DEBUG("connManagerApp_debug_print_conn_params: -----------------------------------------------------------\r\n");
	NRF_LOG_DEBUG("connManagerApp_debug_print_conn_params: min conn interval =0x%x\r\n", conn_params->min_conn_interval);
	NRF_LOG_DEBUG("connManagerApp_debug_print_conn_params: max conn interval =0x%x\r\n", conn_params->max_conn_interval);
	NRF_LOG_DEBUG("connManagerApp_debug_print_conn_params: slave latency =0x%x\r\n", conn_params->slave_latency);
	NRF_LOG_DEBUG("connManagerApp_debug_print_conn_params: conn supervision timeout =0x%x\r\n", conn_params->conn_sup_timeout);
	NRF_LOG_DEBUG("connManagerApp_debug_print_conn_params: -----------------------------------------------------------\r\n");
}

//function to get device type based of peer address
advertised_device_type_e connManagerApp_get_device_type (const ble_gap_addr_t *peer_addr){
	
	advertised_device_type_e ret_code      = ADVERTISED_DEVICE_TYPE_UNKNOWN;
	uint8_t                  i             = 0;
	ble_gap_addr_t*          tmp_gap_addr  = NULL;
	
	for (i=0; i<advertised_devices.count; i++){
		
		tmp_gap_addr = connManagerApp_get_peer_addr(i);
		if (tmp_gap_addr == NULL){
			NRF_LOG_ERROR("connManagerApp_get_device_type: failed to get peer address of advertised device with index= %d\r\n",i);
			break;
		} else{
			if (connManagerApp_is_peer_address_matching(tmp_gap_addr->addr,peer_addr->addr)){
				return advertised_devices.advertised_devices_data[i].device_type;
			}
		}
		
	}
	
	return ret_code;
	
}

//function to map connection handler to a device category
void connManagerApp_map_conn_handler_to_device_type (advertised_device_type_e device_type, const uint16_t conn_handle){
	
	device_type_at_conn_handler[conn_handle] = device_type;
}


//function to update connection parameters of for a device category
void connManagerApp_conn_params_update (const uint16_t conn_handle, const ble_gap_conn_params_t* conn_params){
	if (device_type_at_conn_handler[conn_handle]== ADVERTISED_DEVICE_TYPE_CSCS_SENSOR){
		
		//update connection parameters for CSCS devices category
		cscs_connection_params.min_conn_interval= conn_params->min_conn_interval;
		cscs_connection_params.max_conn_interval= conn_params->max_conn_interval;
		cscs_connection_params.slave_latency    = conn_params->slave_latency;
		cscs_connection_params.conn_sup_timeout = conn_params->conn_sup_timeout;
		NRF_LOG_DEBUG("connManagerApp_conn_params_update: updated conn params for CSCS \r\n");

	} else if(device_type_at_conn_handler[conn_handle]== ADVERTISED_DEVICE_TYPE_HRS_SENSOR){
		
		//update connection parameters for HR devices category
		hr_connection_params.min_conn_interval= conn_params->min_conn_interval;
		hr_connection_params.max_conn_interval= conn_params->max_conn_interval;
		hr_connection_params.slave_latency    = conn_params->slave_latency;
		hr_connection_params.conn_sup_timeout = conn_params->conn_sup_timeout;
		NRF_LOG_DEBUG("connManagerApp_conn_params_update: updated conn params for HR \r\n");
		
	} else if (device_type_at_conn_handler[conn_handle]== ADVERTISED_DEVICE_TYPE_PHONE){
		
		//update connection parameters for phone devices category
		phone_connection_params.min_conn_interval= conn_params->min_conn_interval;
		phone_connection_params.max_conn_interval= conn_params->max_conn_interval;
		phone_connection_params.slave_latency    = conn_params->slave_latency;
		phone_connection_params.conn_sup_timeout = conn_params->conn_sup_timeout;
		NRF_LOG_DEBUG("connManagerApp_conn_params_update: updated conn params for phone \r\n");
		
	} else{
		
		NRF_LOG_WARNING("connManagerApp_conn_params_update: called with ADVERTISED_DEVICE_TYPE_UNKNOWN \r\n");
		
	}
	/*TODO save the updated connection parameters in flash once updated*/
	
}



bool connManagerApp_get_memory_access_in_progress (void){
	return m_memory_access_in_progress;
}

void connManagerApp_set_memory_access_in_progress (bool is_in_progress){
	m_memory_access_in_progress = is_in_progress;
}
/**@brief Function to start scanning.
 */
bool connManagerApp_scan_start(uint32_t scanning_interval_ms){
	
    uint32_t flash_busy;
	bool     ret_code = true;

    // If there is any pending write to flash, defer scanning until it completes.
    (void) fs_queued_op_count_get(&flash_busy);

    if (flash_busy != 0)
    {
        m_memory_access_in_progress = true;
        return false;
    }

    // Whitelist buffers.
    ble_gap_addr_t whitelist_addrs[8];
    ble_gap_irk_t  whitelist_irks[8];

    memset(whitelist_addrs, 0x00, sizeof(whitelist_addrs));
    memset(whitelist_irks,  0x00, sizeof(whitelist_irks));

    uint32_t addr_cnt = (sizeof(whitelist_addrs) / sizeof(ble_gap_addr_t));
    uint32_t irk_cnt  = (sizeof(whitelist_irks)  / sizeof(ble_gap_irk_t));

    #if (NRF_SD_BLE_API_VERSION == 2)

        ble_gap_addr_t * p_whitelist_addrs[8];
        ble_gap_irk_t  * p_whitelist_irks[8];

        for (uint32_t i = 0; i < 8; i++)
        {
            p_whitelist_addrs[i] = &whitelist_addrs[i];
            p_whitelist_irks[i]  = &whitelist_irks[i];
        }

        ble_gap_whitelist_t whitelist =
        {
            .pp_addrs = p_whitelist_addrs,
            .pp_irks  = p_whitelist_irks,
        };

    #endif

    ret_code_t ret;

    // Get the whitelist previously set using pm_whitelist_set().
    ret = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                           whitelist_irks,  &irk_cnt);

    m_scan_param.active   = 0;
    m_scan_param.interval = SCAN_INTERVAL;
    m_scan_param.window   = SCAN_WINDOW;

    if (((addr_cnt == 0) && (irk_cnt == 0)) ||
        (m_scan_mode != BLE_WHITELIST_SCAN) ||
        (m_whitelist_temporarily_disabled))
    {
        // Don't use whitelist.

        m_scan_param.timeout  = 0x0000; // No timeout.

        #if (NRF_SD_BLE_API_VERSION == 2)
            m_scan_param.selective   = 0;
            m_scan_param.p_whitelist = NULL;
        #endif

        #if (NRF_SD_BLE_API_VERSION == 3)
            m_scan_param.use_whitelist  = 0;
            m_scan_param.adv_dir_report = 0;
        #endif
    }
    else
    {
        // Use whitelist.

        m_scan_param.timeout  = 0x001E; // 30 seconds.

        #if (NRF_SD_BLE_API_VERSION == 2)
            whitelist.addr_count     = addr_cnt;
            whitelist.irk_count      = irk_cnt;
            m_scan_param.selective   = 1;
            m_scan_param.p_whitelist = &whitelist;
        #endif

        #if (NRF_SD_BLE_API_VERSION == 3)
            m_scan_param.use_whitelist  = 1;
            m_scan_param.adv_dir_report = 0;
        #endif
    }

    NRF_LOG_DEBUG("connManagerApp_scan_start: starting scan.\r\n");
	//delete all previous advertising devices
	/*TODO: figure out why this memset causes no advertisement reports to be received */
	//memset(&advertised_devices, 0x00, sizeof(advertised_devices_t));
    ret = sd_ble_gap_scan_start(&m_scan_param);
	/*TODO: figure out why sd_ble_gap_scan_start returns NRF_ERROR_INVALID_STATE*/
    if(ret == NRF_ERROR_INVALID_STATE){
		    NRF_LOG_WARNING("sd_ble_gap_scan_start returned NRF_ERROR_INVALID_STATE but will skip it for now.\r\n");
	} else{
		
		APP_ERROR_CHECK(ret);
		
		//timeout scanning after the specified time interval
	    ret = app_timer_start(scanning_timer_id, CONNMANAGER_MS_TO_TICK(scanning_interval_ms), (void*)NULL);
		if (ret != NRF_SUCCESS)
		{
			NRF_LOG_ERROR("connManagerApp_scan_start: app_timer_start failed with error = %d\r\n",ret);
		}

		ret = bsp_indication_set(BSP_INDICATE_SCANNING);
		APP_ERROR_CHECK(ret);
    
		/*TODO: figure out whether to cintinue or not if err_code != NRF_SUCCESS*/
		ret = bsp_indication_set(BSP_INDICATE_IDLE);
		APP_ERROR_CHECK(ret);
		
	}
	
	if(ret != NRF_SUCCESS){
		ret_code = false;
	}
	
	return ret_code;
}

/**@brief Function for disabling the use of whitelist for scanning.
 */
void connManagerApp_whitelist_disable(void){
    
	uint32_t err_code;

    if ((m_scan_mode == BLE_WHITELIST_SCAN) && !m_whitelist_temporarily_disabled)
    {
        m_whitelist_temporarily_disabled = true;

        err_code = sd_ble_gap_scan_stop();
        if (err_code == NRF_SUCCESS)
        {
            connManagerApp_scan_start(SCANNING_WAITING_PERIOD_MS);
        }
        else if (err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
    }
    m_whitelist_temporarily_disabled = true;
}


bool connManagerApp_advertised_device_connect(uint8_t advertised_device_id){
	
	bool                     ret_code                   = true;
	uint32_t                 err_code                   = NRF_SUCCESS;
	ble_gap_addr_t          *peer_addr                  = NULL;
	ble_gap_conn_params_t   *connection_params          = NULL;
	uint8_t                  advertised_device_index    = advertised_device_id;  /* for now the id is the same index inside array
														                          * advertised_devices.advertised_devices_data[]
														                          **/
    //check if there is an older connection request that is not completely served
	if(busy_connecting){
		//queue this connection request if there is spcace
		if (connection_requests_queue.queued_request_count < MAX_CONNECTIONS_COUNT){
			connection_requests_queue.queue_request[connection_requests_queue.queued_request_count] = advertised_device_id;
			connection_requests_queue.queued_request_count++;
		}
	}
	
	/*TODO: figure out whether to cintinue or not if err_code != NRF_SUCCESS*/
	err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Initiate connection.
    #if (NRF_SD_BLE_API_VERSION == 2)
	m_scan_param.selective = 0;
    #endif
	
	peer_addr = connManagerApp_get_peer_addr(advertised_device_index);
	if (peer_addr == NULL){
		NRF_LOG_ERROR("connManagerApp_advertised_device_connect: connManagerApp_get_peer_addr for device index= %d returned NULL\r\n", advertised_device_index);
		ret_code = false;
	} else{
		
		advertised_device_type_e device_type = advertised_devices.advertised_devices_data[advertised_device_index].device_type;
		switch (device_type){
			case ADVERTISED_DEVICE_TYPE_CSCS_SENSOR:
				connection_params = &cscs_connection_params;
				break;
			case ADVERTISED_DEVICE_TYPE_HRS_SENSOR:
				connection_params = &hr_connection_params;
				break;
			case ADVERTISED_DEVICE_TYPE_PHONE:
				connection_params = &phone_connection_params;
				break;
			default:
				NRF_LOG_ERROR("connManagerApp_advertised_device_connect: advertised device[%d] is of unknown type=%d \r\n",
								advertised_device_index, device_type);
				break;
		}
		
		if (connection_params == NULL){
			NRF_LOG_ERROR("connManagerApp_advertised_device_connect: connection_params is NULL\r\n");
			ret_code = false;
		} else {
			//NRF_LOG_DEBUG("connManagerApp_advertised_device_connect: debugging conn params set by central before connecting\r\n");
			//connManagerApp_debug_print_conn_params(connection_params);
			err_code = sd_ble_gap_connect(peer_addr,
										  &m_scan_param,
										  connection_params);
			
			m_whitelist_temporarily_disabled = false;

			if (err_code != NRF_SUCCESS){
				NRF_LOG_ERROR("connManagerApp_advertised_device_connect: connection Request failed. Device= %d. Failure reason= %d\r\n",
								advertised_devices.advertised_devices_data[advertised_device_index].device_type,
								err_code);
			} else{
				NRF_LOG_DEBUG("connManagerApp_advertised_device_connect: connection request to device type= %d was sent to softdevice successfully\r\n",
								advertised_devices.advertised_devices_data[advertised_device_index].device_type);
				
				//set busy flag to avoid serving next connection request until the current request is served
				busy_connecting = true;
				
				switch (device_type){
					case ADVERTISED_DEVICE_TYPE_CSCS_SENSOR:
						cscs_peripheral_connected = true;
						break;
					case ADVERTISED_DEVICE_TYPE_HRS_SENSOR:
						hr_peripheral_connected = true;
						break;
					case ADVERTISED_DEVICE_TYPE_PHONE:
						phone_peripheral_connected = true;
						break;
					default:
						NRF_LOG_ERROR("connManagerApp_advertised_device_connect: advertised device [%d] is of unknown type=%d \r\n",
										advertised_device_index, device_type);
						break;
		}
				
			}
		}

		
	}	
		
	return ret_code;
}

bool connManagerApp_advertised_device_store(advertised_device_type_e device_type, const ble_gap_evt_adv_report_t* adv_report){
	
	bool     ret_code          = true;
	uint8_t  i                 = 0;
	uint8_t  last_count        = advertised_devices.count;
	bool     device_stored     = false;
	
	for (i=0; (i<last_count) && (!device_stored); i++){
		if (advertised_devices.advertised_devices_data[i].device_type == device_type
			&& connManagerApp_is_peer_address_matching(advertised_devices.advertised_devices_data[i].peer_addr.addr, adv_report->peer_addr.addr)
		   ){
			
			// defice info is already stored
			device_stored = true;
		}
	}
	
	if (device_stored){
		NRF_LOG_DEBUG("connManagerApp_store_advertised_device: device type=%d is already stored\r\n", device_type);
	} else if(last_count >=ADVERTISED_DEVICES_COUNT_MAX){
		
		NRF_LOG_DEBUG("connManagerApp_store_advertised_device: stored advertised devices reached max. last_count=%d \r\n", last_count);
	
	} else{
		//this is a new advertised device. Store its connection info  and increase count
		advertised_devices.advertised_devices_data[last_count].device_type = device_type;
		advertised_devices.advertised_devices_data[last_count].peer_addr = adv_report->peer_addr;
		advertised_devices.advertised_devices_data[last_count].rssi = adv_report->rssi;
		
		advertised_devices.count ++;
	}
	
	return ret_code;
}

void connManagerApp_on_connection(const ble_gap_evt_t* evt){
	
	advertised_device_type_e device_type      = ADVERTISED_DEVICE_TYPE_UNKNOWN;
			
	NRF_LOG_INFO("Connected to a device with a connection handle= 0x%x.\r\n", evt->conn_handle);
	
	//reset busy_connecting flag
	busy_connecting= false;
	
	APP_ERROR_CHECK_BOOL(evt->conn_handle < TOTAL_LINK_COUNT);
			
	device_type= connManagerApp_get_device_type(&(evt->params.connected.peer_addr));
	if (device_type == ADVERTISED_DEVICE_TYPE_UNKNOWN){
		NRF_LOG_ERROR("on_ble_evt: connManagerApp_get_device_type returned ADVERTISED_DEVICE_TYPE_UNKNOWN\r\n");
	} else{
		connManagerApp_map_conn_handler_to_device_type(device_type, evt->conn_handle);
	}
	
	//serve remaining connection requests in queue
	connManagerApp_handle_queue_conn_requests();
}

void connManagerApp_on_disconnection(ble_gap_evt_t* evt){
	
	advertised_device_type_e disconnected_device_type = ADVERTISED_DEVICE_TYPE_UNKNOWN;
	
	NRF_LOG_DEBUG("Disconnected from a device with a connection handle= 0x%x BLE_HCI Reason= 0x%x \r\n", 
					evt->conn_handle, evt->params.disconnected.reason);
	
	
	//indicate this device is disconnected
	if (evt->conn_handle < MAX_CONNECTIONS_COUNT){
		
		disconnected_device_type = device_type_at_conn_handler[evt->conn_handle];
		switch (disconnected_device_type){
			case ADVERTISED_DEVICE_TYPE_CSCS_SENSOR:
				cscs_peripheral_connected = false;
				break;
			case ADVERTISED_DEVICE_TYPE_HRS_SENSOR:
				hr_peripheral_connected = false;
				break;
			case ADVERTISED_DEVICE_TYPE_PHONE:
				phone_peripheral_connected = false;
				break;
			default:
				NRF_LOG_ERROR("connManagerApp_on_disconnection: advertised device is of unknown type=%d \r\n", disconnected_device_type);
				break;
		}

	} else {
		NRF_LOG_ERROR("connManagerApp_on_disconnection: connection handle= 0x%x is beyond MAX_CONNECTIONS_COUNT\r\n",
						evt->conn_handle);
	}
	
	/*TODO: Figure out when to perform automatic scan*/
	//if (ble_conn_state_n_centrals() < CENTRAL_LINK_COUNT)
	if (ble_conn_state_n_centrals() == 0){
		connManagerApp_scan_start(SCANNING_WAITING_PERIOD_MS);
    }
	//not sure if the following commented lines are needed.
	/*
	err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    memset(&m_ble_db_discovery, 0 , sizeof (m_ble_db_discovery));

    if (m_peer_count == CENTRAL_LINK_COUNT)
    {
		m_peer_count--;
        scan_start();
    }
	*/
}


/**
 * @brief Connection Manager App initialization.
 */
bool connManagerApp_init(void){
	
	bool       ret_code          = true;
	uint32_t   err_code          = NRF_SUCCESS;
	
	memset(&advertised_devices, 0x00, sizeof(advertised_devices_t));
	memset(&connection_requests_queue, 0x00, sizeof(connection_requests_queue_t));
	
	//Create timer to be used to timeout ble scanning
	err_code = app_timer_create(&scanning_timer_id,
                                 APP_TIMER_MODE_SINGLE_SHOT,
                                 scanning_timer_handler);
	if (err_code != NRF_SUCCESS){
		NRF_LOG_DEBUG("connManagerApp_init: failed with error= %d\r\n", err_code);
		ret_code = false;
    }
		
	if (CONN_MANAGER_APP_STANDALONE_MODE && ret_code){
		/*if in standalone mode, scan for all devices at initialization and store results of up to 
		 *ADVERTISED_DEVICES_COUNT_MAX
		 **/
		ret_code = connManagerApp_scan_start(SCANNING_WAITING_PERIOD_MS);
	}
		
	return ret_code;	
}
