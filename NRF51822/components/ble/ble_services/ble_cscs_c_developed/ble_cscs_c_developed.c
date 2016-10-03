/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 * Example code written by Thomas R. 
 *
 */

/*  This module is a modified version of the Cycling Speed and Cadence Service Client provided with the SDK from
 *  Nordic Semiconductors.
 */

/**@cond To Make Doxygen skip documentation generation for this file.
 * @{
 */
 
 #include "sdk_config.h"
#if BLE_CSCS_C_ENABLED
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_error.h"
#include "app_util.h"
#include "ble_cscs_c_developed.h" //renaming the h file is needed
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"
#include "sdk_common.h"


#define NRF_LOG_MODULE_NAME "BLE_CSCS_C"
#include "nrf_log.h"

//#define LOG                    app_trace_log         /**< Debug logger macro that will be used in this file to do logging of important information over UART. */
//#define LOG(...)

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */

#define CSCM_FLAG_WHEEL_PRESENT  (0x01 << 0)           /**< Bit mask used to extract the presence of Wheel Revolution Data. */
#define CSCM_FLAG_CRANK_PRESENT  (0x01 << 1)           /**< Bit mask used to extract the presence of Crank Revolution Data. */

#define CSCF_FLAG_WHEEL_SUPP     (0x01 << 0)         /**< Bit mask to extract if Wheel Revolution Data is supported. */
#define CSCF_FLAG_CRANK_SUPP     (0x01 << 1)         /**< Bit mask to extract if Crank Revolution Data is supported. */
#define CSCF_FLAG_MULT_SENS_SUPP (0x01 << 2)         /**< Bit mask to extract if Multiple Sensor Locations is supported. */

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */

typedef enum
{
    READ_REQ,  /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ  /**< Type identifying that this tx_message is a write request. */
} tx_request_t;

/**@brief Structure for writing a message to the peer, i.e. CCCD.
 */
typedef struct
{
    uint8_t                  gattc_value[WRITE_MESSAGE_LENGTH];  /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                       /**< GATTC parameters for this message. */
} write_params_t;

/**@brief Structure for holding data to be transmitted to the connected central.
 */
typedef struct
{
    uint16_t     conn_handle;  /**< Connection handle to be used when transmitting this message. */
    tx_request_t type;         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t       read_handle;  /**< Read request message. */
        write_params_t write_req;    /**< Write request message. */
    } req;
} tx_message_t;

/*The following variable is used when there are multiple clients*/
//static ble_cscs_c_t * mp_ble_csc_c_start;           /**< Pointer to the first instance of the CSC Client module */

uint8_t m_csc_c_numof                     =1;                /**< Number of structs available */
static tx_message_t   m_tx_buffer[TX_BUFFER_SIZE];  /**< Transmit buffer for messages to be transmitted to the central. */
static uint32_t       m_tx_insert_index  = 0;        /**< Current index in the transmit buffer where the next message should be inserted. */
static uint32_t       m_tx_index         = 0;               /**< Current index in the transmit buffer from where the next message to be transmitted resides. */

/**@brief Function for passing any pending request from the buffer to the stack.
 */
static void tx_buffer_process(void)
{
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;

        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            NRF_LOG_INFO("[CSCS_C]: SD Read/Write API returns Success..\r\n");
            m_tx_index++;
            m_tx_index &= TX_BUFFER_MASK;
        }
        else
        {
            NRF_LOG_INFO("[HRS_C]: SD Read/Write API returns error. This message sending will be "
                "attempted again..\r\n");
        }
    }
}


/**@brief     Function for handling write response events.
 *
 * @param[in] p_ble_cscs_c Pointer to the Cycling Speed and Cadence Client structure.
 * @param[in] p_ble_evt    Pointer to the BLE event received.
 */
static void on_write_rsp(ble_cscs_c_t * p_ble_cscs_c, const ble_evt_t * p_ble_evt)
{
    // Check if the event on the link for this instance
    if (p_ble_cscs_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
		
	  // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}


/**@brief     Function for handling read response events.
 *
 * @details   This function will validate the read response and raise the appropriate
 *            event to the application.
 *
 * @param[in] p_csc_c   Pointer to the Cycling Speed and Cadence Client Structure.
 * @param[in] p_ble_evt Pointer to the SoftDevice event.
 */
/*
NOT USED in SDK v12
static void on_read_rsp(ble_cscs_c_t * p_csc_c, const ble_evt_t * p_ble_evt)
{
    const ble_gattc_evt_read_rsp_t * p_response;

    p_response = &p_ble_evt->evt.gattc_evt.params.read_rsp;

    if (p_response->handle == p_csc_c->cscf_handle)
    {
        ble_cscs_c_evt_t evt;
        uint16_t feature;

        evt.evt_type = BLE_CSCS_C_EVT_CSCF_READ_RESP;

        feature = uint16_decode(&p_response->data[0]);
        evt.params.cscf.wheel_revo_supported = (feature & CSCF_FLAG_WHEEL_SUPP ? 1 : 0);
        evt.params.cscf.crank_revo_supported = (feature & CSCF_FLAG_CRANK_SUPP ? 1 : 0);
        evt.params.cscf.multiple_sensor_loc  = (feature & CSCF_FLAG_MULT_SENS_SUPP ? 1 : 0);

        p_csc_c->evt_handler(p_csc_c, &evt);
    }
    // Check if there is any buffered transmissions and send them.
    tx_buffer_process();
}
*/

/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function will uses the Handle Value Notification received from the SoftDevice
 *            and checks if it is a notification of the heart rate measurement from the peer. If
 *            it is, this function will decode the heart rate measurement and send it to the
 *            application.
 *
 * @param[in] p_ble_csc_c Pointer to the Cycling Speed and Cadence Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(ble_cscs_c_t * p_ble_cscs_c, const ble_evt_t * p_ble_evt)
{
	  const ble_gattc_evt_hvx_t * p_notif = &p_ble_evt->evt.gattc_evt.params.hvx;
	
    // Check if the event if on the link for this instance
    if (p_ble_cscs_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
	  // Check if this is a cycling speed and cadence notification.
		/*TODO: debug why this equality is not true!*/
    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_cscs_c->peer_db.csc_handle)
    {
        uint32_t              index           = 0;
			  uint_fast8_t          flags           = 0;
			  ble_cscs_c_evt_t      ble_cscs_c_evt;

			  //memset(&ble_cscs_c_evt, 0, sizeof(ble_cscs_c_evt_t));
			  ble_cscs_c_evt.evt_type     = BLE_CSCS_C_EVT_CSC_NOTIFICATION;
			  ble_cscs_c_evt.conn_handle  = p_ble_evt->evt.gattc_evt.conn_handle;;
			
			/*TODO: if this doesn't work, get flags following the code for rscs*/
			  flags = p_ble_evt->evt.gattc_evt.params.hvx.data[index++];
			
        if ((flags & CSCM_FLAG_WHEEL_PRESENT))
        {
            ble_cscs_c_evt.params.csc_meas.is_wheel_rev_data_present = 1;
            ble_cscs_c_evt.params.csc_meas.cumulative_wheel_revs =
                uint32_decode(&p_ble_evt->evt.gattc_evt.params.hvx.data[index]);
            index += 4;
            ble_cscs_c_evt.params.csc_meas.last_wheel_event_time =
                uint16_decode(&p_ble_evt->evt.gattc_evt.params.hvx.data[index]);
            index += 2;
        }
        if ((flags & CSCM_FLAG_CRANK_PRESENT))
        {
            ble_cscs_c_evt.params.csc_meas.is_crank_rev_data_present = 1;
            ble_cscs_c_evt.params.csc_meas.cumulative_crank_revs =
                uint16_decode(&p_ble_evt->evt.gattc_evt.params.hvx.data[index]);
            index += 2;
            ble_cscs_c_evt.params.csc_meas.last_crank_event_time =
                uint16_decode(&p_ble_evt->evt.gattc_evt.params.hvx.data[index]);
            index += 2;
        }

        p_ble_cscs_c->evt_handler(p_ble_cscs_c, &ble_cscs_c_evt);
    }
}

/**@brief     Function for handling events from the database discovery module.
 *
 * @details   This function will handle an event from the database discovery module, and determine
 *            if it relates to the discovery of heart rate service at the peer. If so, it will
 *            call the application's event handler indicating that the Cycling Speed and Cadence
 *            service has been discovered at the peer. It also populates the event with the service
 *            related information before providing it to the application.
 *
 * @param[in] p_evt Pointer to the event received from the database discovery module.
 *
 */
void ble_cscs_on_db_disc_evt(ble_cscs_c_t * p_ble_cscs_c, const ble_db_discovery_evt_t * p_evt)
{
    // Check if the Cycling Speed and Cadence Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_CYCLING_SPEED_AND_CADENCE &&
        p_evt->params.discovered_db.srv_uuid.type == BLE_UUID_TYPE_BLE)
    {
        ble_cscs_c_evt_t evt;
        evt.conn_handle = p_evt->conn_handle;

        // Find the CCCD Handle of the Cycling Speed and Cadence characteristic.
        uint32_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_CSC_MEASUREMENT_CHAR)
            {
                // Found Cycling Speed and Cadence characteristic. Store CCCD handle and break.
                evt.params.cscs_db.csc_cccd_handle =
                    p_evt->params.discovered_db.charateristics[i].cccd_handle;
                evt.params.cscs_db.csc_handle      =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                break;
            } else{
								NRF_LOG_ERROR("Cycling Speed and Cadence characteristic is not found in discovered database.\r\n");
						}
							
        }

        NRF_LOG_INFO("Cycling Speed and Cadence Service discovered at peer.\r\n");

        //If the instance has been assigned prior to db_discovery, assign the db_handles
        if (p_ble_cscs_c->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if ((p_ble_cscs_c->peer_db.csc_cccd_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_ble_cscs_c->peer_db.csc_handle == BLE_GATT_HANDLE_INVALID))
            {
                p_ble_cscs_c->peer_db = evt.params.cscs_db;
            }
        }

        evt.evt_type = BLE_CSCS_C_EVT_DISCOVERY_COMPLETE;
				if(p_ble_cscs_c->evt_handler != NULL){
						p_ble_cscs_c->evt_handler(p_ble_cscs_c, &evt);
				} else{
					  NRF_LOG_ERROR("p_ble_cscs_c->evt_handler is NULL\r\n");
				}
        
    }
}

/**@brief     Function for handling events from the database discovery module.
 *
 * @details   This function will handle an event from the database discovery module, and determine
 *            if it relates to the discovery of heart rate service at the peer. If so, it will
 *            call the application's event handler indicating that the cycling speed and cadence service has been
 *            discovered at the peer. It also populates the event with the service related
 *            information before providing it to the application.
 *
 * @param[in] p_evt Pointer to the event received from the database discovery module.
 *
 */
/*
static void db_discover_evt_handler(ble_db_discovery_evt_t * p_evt)
{
   //commented out because it in not used anymore. See ble_cscs_on_db_disc_evt
}
*/

/*If more that one clients are needed to handle multiple sensors, 
then uint8_t m_csc_c_numof argument is needed.
In that case, follow the implementation from Thomas R's implementation*/
uint32_t ble_cscs_c_init(ble_cscs_c_t * p_ble_cscs_c, const ble_cscs_c_init_t * p_ble_cscs_c_init)
{
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c);
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c_init);

    ble_uuid_t csc_uuid;

    csc_uuid.type = BLE_UUID_TYPE_BLE;
    csc_uuid.uuid = BLE_UUID_CYCLING_SPEED_AND_CADENCE;

    m_csc_c_numof      = 1;                                     //There is only a single csc collector 

    p_ble_cscs_c->evt_handler             = p_ble_cscs_c_init->evt_handler;
	  p_ble_cscs_c->conn_handle             = BLE_CONN_HANDLE_INVALID;
	  p_ble_cscs_c->peer_db.csc_cccd_handle = BLE_GATT_HANDLE_INVALID;
	  p_ble_cscs_c->peer_db.csc_handle      = BLE_GATT_HANDLE_INVALID;

    return ble_db_discovery_evt_register(&csc_uuid);
}


uint32_t ble_cscs_c_handles_assign(ble_cscs_c_t *    p_ble_cscs_c,
                                   uint16_t          conn_handle,
                                   ble_cscs_c_db_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c);
    p_ble_cscs_c->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_cscs_c->peer_db = *p_peer_handles;
    }

    return NRF_SUCCESS;
}

/**@brief     Function for handling Disconnected event received from the SoftDevice.
 *
 * @details   This function check if the disconnect event is happening on the link
 *            associated with the current instance of the module, if so it will set its
 *            conn_handle to invalid.
 *
 * @param[in] p_ble_cscs_c Pointer to the CSC Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_disconnected(ble_cscs_c_t * p_ble_cscs_c, const ble_evt_t * p_ble_evt)
{
    if (p_ble_cscs_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_ble_cscs_c->conn_handle             = BLE_CONN_HANDLE_INVALID;
			  p_ble_cscs_c->peer_db.csc_cccd_handle = BLE_GATT_HANDLE_INVALID;
        p_ble_cscs_c->peer_db.csc_handle      = BLE_GATT_HANDLE_INVALID;
			
			 /*
			   TODO: deactivae necessary handles, figure out which ones
        p_ble_csc_c->cscm_cccd_handle = BLE_GATT_HANDLE_INVALID;
        p_ble_csc_c->cscm_handle      = BLE_GATT_HANDLE_INVALID;
        p_ble_csc_c->cscf_handle      = BLE_GATT_HANDLE_INVALID;
		   */
    }
}
void ble_cscs_c_on_ble_evt(ble_cscs_c_t * p_ble_cscs_c, const ble_evt_t * p_ble_evt)
{

    if ( (p_ble_cscs_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }
    
		//find the collector which has the same connection handler.
		/* Not used. Used only if there are multiple collectors
		for (uint_fast8_t i = 0; i < m_csc_c_numof; i++)
    {
        if (mp_ble_csc_c_start[i].conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
        {
            tmp_cscs_c = &mp_ble_csc_c_start[i];
        }
    }
		
    
		if (p_ble_cscs_c == NULL){
        return;
		}
		*/

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnected(p_ble_cscs_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_cscs_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_ble_cscs_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_READ_RSP:
            //on_read_rsp(p_ble_cscs_c, p_ble_evt);
            break;

        default:
            break;
    }
    
		/* TODO: activate this instruction only if needed*/
    //tx_buffer_process();
}


/**@brief Function for creating a message for writing to the CCCD.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
    NRF_LOG_INFO("[CSC_C]: Configuring CCCD. CCCD Handle = %d, Connection Handle = %d\r\n",
        handle_cccd,conn_handle);

    tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_GATT_HVX_NOTIFICATION : 0;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB_16(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB_16(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}


uint32_t ble_cscs_c_csc_notif_enable(ble_cscs_c_t * p_ble_cscs_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_cscs_c);

    if (p_ble_cscs_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    return cccd_configure(p_ble_cscs_c->conn_handle, p_ble_cscs_c->peer_db.csc_cccd_handle, true);
}

/*Not used*/
/*
uint32_t ble_csc_c_cscf_read(uint16_t conn_handle)
{
    tx_message_t * msg;
    ble_cscs_c_t * p_ble_cscs_c = NULL;

    for (uint8_t i = 0; i < m_csc_c_numof; i++)
    {
        if (mp_ble_csc_c_start[i].conn_handle == conn_handle)
        {
            p_ble_cscs_c = &mp_ble_csc_c_start[i];
        }
    }
    if (p_ble_cscs_c == NULL)
    {
        return NRF_ERROR_NOT_FOUND;
    }

    msg                  = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index   &= TX_BUFFER_MASK;

    msg->req.read_handle = p_ble_cscs_c->cscf_handle;
    msg->conn_handle     = p_ble_cscs_c->conn_handle;
    msg->type            = READ_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}
*/

/** @}
 *  @endcond
 */
#endif //BLE_CSCS_C_ENABLED

