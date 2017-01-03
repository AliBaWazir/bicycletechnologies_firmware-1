/*
* Copyright (c) 2016 Shifty: Automatic Gear Selection System. All Rights Reserved.
 *
 * The code developed in this file is for the 4th year project: Shifty. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with the Shifty team.
 * Created by: Ali Ba Wazir
 * Jan 2017
 */

/**
 * @brief BLE Connection Manager application file.
 *
 */
 
/**********************************************************************************************
* INCLUDES
***********************************************************************************************/
#include <string.h>
#include "sdk_config.h"
#include "fds.h"
#include "sdk_common.h"
#include "app_error.h"
#include "fstorage.h"
#include "app_timer.h"
#define NRF_LOG_MODULE_NAME "APPLICATION_FDS APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "application_fds_app.h"

/**********************************************************************************************
* MACRO DEFINITIONS
***********************************************************************************************/
#define USER_DEFINED_PROPERTIES_FILE_ID    0x1000
#define CADENCE_SET_POINT_REC_KEY          0x1000
#define BIKE_CONFIG_DATA_REC_KEY           0x1010

/**********************************************************************************************
* TYPE DEFINITIONS
***********************************************************************************************/


/**********************************************************************************************
* STATIC VARIABLES
***********************************************************************************************/
static bool m_app_fds_initialized    = false;
//static bool m_app_fds_delete_queued  = false;
//static bool m_app_fds_delete_ongoing = false;
 
/**********************************************************************************************
* STATIC FUCNCTIONS
***********************************************************************************************/
static void applicationFdsApp_evt_handler(fds_evt_t const * const p_fds_evt)
{
	

    switch (p_fds_evt->id)
    {
        case FDS_EVT_WRITE:
        case FDS_EVT_UPDATE:
			//check if this event concerns application FDS
            if ( (p_fds_evt->write.file_id == USER_DEFINED_PROPERTIES_FILE_ID)
                || (p_fds_evt->write.record_key == CADENCE_SET_POINT_REC_KEY)
			    || (p_fds_evt->write.record_key == BIKE_CONFIG_DATA_REC_KEY))
            {
				
                if (p_fds_evt->id == FDS_EVT_WRITE){
					if(p_fds_evt->result == FDS_SUCCESS){
						NRF_LOG_DEBUG("applicationFdsApp_evt_handler: writing record with key= %d was successful \r\n",
									   p_fds_evt->write.record_key);
					} else{
						NRF_LOG_ERROR("applicationFdsApp_evt_handler: writing record with key= %d failed with result= \r\n", 
									   p_fds_evt->write.record_key,
									   p_fds_evt->result);
					}
					
					//read the written/updated data
					switch(p_fds_evt->write.record_key){
						case CADENCE_SET_POINT_REC_KEY:
							//read back the written value
							if(!applicationFdsApp_fds_read(USER_DEFINED_CADENCE_SETPOINT, (uint8_t*) &cadence_setpoint_rpm)){
								NRF_LOG_ERROR("applicationFdsApp_evt_handler: applicationFdsApp_fds_read() failed to read cadence_setpoint_rpm\r\n");
							}
						break;
						
						case BIKE_CONFIG_DATA_REC_KEY:
							//read back the written value
							if(!applicationFdsApp_fds_read(USER_DEFINED_BIKE_CONFIG_DATA, (uint8_t*) &user_defined_bike_config_data)){
								NRF_LOG_ERROR("applicationFdsApp_evt_handler: applicationFdsApp_fds_read() failed to read bike config\r\n");
							}
						break;
						
						default:
							NRF_LOG_ERROR("applicationFdsApp_evt_handler: called with unrecognized record key= %d\r\n", p_fds_evt->write.record_key);
						break;
					}
					
                } else{
					if(p_fds_evt->result == FDS_SUCCESS){
						NRF_LOG_DEBUG("applicationFdsApp_evt_handler: updating record with key= %d was successful \r\n",
									   p_fds_evt->write.record_key);
					} else{
						NRF_LOG_ERROR("applicationFdsApp_evt_handler: updating record with key= %d failed with result= \r\n", 
									   p_fds_evt->write.record_key,
									   p_fds_evt->result);
					}
                }
				
            }
        break;

        case FDS_EVT_DEL_RECORD:
            if ( (p_fds_evt->del.file_id == USER_DEFINED_PROPERTIES_FILE_ID)
                || (p_fds_evt->del.record_key == CADENCE_SET_POINT_REC_KEY)
			    || (p_fds_evt->del.record_key == BIKE_CONFIG_DATA_REC_KEY))
            {

				if(p_fds_evt->result == FDS_SUCCESS){
					NRF_LOG_DEBUG("applicationFdsApp_evt_handler: deleting record with key= %d was successful \r\n",
								   p_fds_evt->del.record_key);
			    } else{
				    NRF_LOG_ERROR("applicationFdsApp_evt_handler: deleting record with key= %d failed with result= \r\n", 
								   p_fds_evt->del.record_key,
								   p_fds_evt->result);
			    }

            }
        break;

        case FDS_EVT_DEL_FILE:
        {
			if ((p_fds_evt->del.file_id == USER_DEFINED_PROPERTIES_FILE_ID)
                 && (p_fds_evt->del.record_key == FDS_RECORD_KEY_DIRTY))
            {

				if (p_fds_evt->result == FDS_SUCCESS){
					NRF_LOG_DEBUG("applicationFdsApp_evt_handler: deleting file with ID= %d was successful \r\n",
								   p_fds_evt->del.file_id);

                }else {
					NRF_LOG_ERROR("applicationFdsApp_evt_handler: deleting file with ID= %d failed with result= \r\n", 
					p_fds_evt->del.file_id,
					p_fds_evt->result);
                }

            }
        }
        break;

        case FDS_EVT_GC:

        break;

        default:
			break;
    }

	/*
    if (m_app_fds_delete_queued)
    {
        m_app_fds_delete_queued  = false;
        peer_data_delete();
    }
	*/
}

static ret_code_t applicationFdsApp_data_find (uint16_t file_id,
											   uint16_t record_key,
											   fds_record_desc_t * const p_desc)
{
    ret_code_t       ret= FDS_SUCCESS;
    fds_find_token_t ftok;

	if(p_desc == NULL){
		NRF_LOG_ERROR("applicationFdsApp_data_find: called with NULL p_desc\r\n");
		return NRF_ERROR_INVALID_PARAM;
	}		

    memset(&ftok, 0x00, sizeof(fds_find_token_t));

    ret = fds_record_find(file_id, record_key, p_desc, &ftok);

    if (ret != FDS_SUCCESS)
    {
        return NRF_ERROR_NOT_FOUND;
    }

    return NRF_SUCCESS;
}


/**********************************************************************************************
* PUBLIC FUCNCTIONS
***********************************************************************************************/

/*This function returns false if no record is find in FDS for the specidied data_type */
bool applicationFdsApp_fds_read(user_defined_properties_type_e data_type, const void* dest_p){
	
	bool       ret_code          = true;
	bool       record_found      = false;
	uint16_t   record_key        = 0;
	uint8_t    bytes_to_read     = 0;
	//uint8_t    words_to_read    = 0;
	//uint8_t    remainder         = 0;
	
	fds_record_desc_t   record_desc;
    fds_flash_record_t  flash_record;
    fds_find_token_t    ftok;
	
	memset(&ftok, 0x00, sizeof(fds_find_token_t));

	/*
	remainder= bytes_to_read%sizeof(uint32_t);
	if (remainder != 0){
		words_to_read= bytes_to_read/sizeof(uint32_t)+1;
	} else{
		words_to_read= bytes_to_read/sizeof(uint32_t);
	}
	*/
	
	switch (data_type){
		case USER_DEFINED_CADENCE_SETPOINT:
		case USER_DEFINED_BIKE_CONFIG_DATA:
		{
			if(data_type == USER_DEFINED_CADENCE_SETPOINT){
				record_key = CADENCE_SET_POINT_REC_KEY;
				bytes_to_read = sizeof(uint32_t);
			} else{
				record_key= BIKE_CONFIG_DATA_REC_KEY;
				bytes_to_read = sizeof(user_defined_bike_config_data_t);
			}
			while (fds_record_find_by_key(record_key, &record_desc, &ftok) == FDS_SUCCESS){
						
				record_found= true;
				if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS){
					NRF_LOG_ERROR("applicationFdsApp_fds_read: failed to open record with record_key= %d\r\n", CADENCE_SET_POINT_REC_KEY);
				} else{

					memcpy((uint8_t*)dest_p, (uint8_t*)flash_record.p_data, bytes_to_read);
				}
						
				(void)fds_record_close(&record_desc);
					
			}
				
			if(!record_found){
				NRF_LOG_DEBUG("applicationFdsApp_fds_read: did not find record with key= %d\r\n", record_key);
				ret_code= false;
			}
		}
		break;
					
		default: 
			NRF_LOG_ERROR("applicationFdsApp_fds_read: called with invalid data_type= %d\r\n", data_type);
			ret_code= false;
		break;
	}
	return ret_code;
}


bool applicationFdsApp_fds_store(user_defined_properties_type_e data_type, uint8_t* source_p){
	
	bool       ret_code          = true;
	ret_code_t ret               = FDS_SUCCESS;
	uint8_t    bytes_to_write    = 0; //number of bytes to write
	uint8_t    words_to_write    = 0; //number of words to write in FDS. a word is 4 bytes
	uint8_t    remainder         = 0;
	
	fds_record_t        record;
	fds_record_desc_t   record_desc;
	fds_record_chunk_t  record_chunk;
	
	if(source_p == NULL){
		NRF_LOG_ERROR("applicationFdsApp_fds_store: source is NULL\r\n");
		return false;
	}
	
	switch (data_type){
		case USER_DEFINED_CADENCE_SETPOINT:
		case USER_DEFINED_BIKE_CONFIG_DATA:
		{
			
			// Set up data.
			if(data_type == USER_DEFINED_CADENCE_SETPOINT){
				record.key = CADENCE_SET_POINT_REC_KEY;
				bytes_to_write = sizeof(uint32_t);
			} else{
				record.key = BIKE_CONFIG_DATA_REC_KEY;
				bytes_to_write = sizeof(user_defined_bike_config_data_t);
			}
			//calculate how many words to write
			remainder= bytes_to_write%sizeof(uint32_t);
			if (remainder != 0){
				words_to_write= bytes_to_write/sizeof(uint32_t)+1;
			} else{
				words_to_write= bytes_to_write/sizeof(uint32_t);
			}
			record_chunk.p_data         = source_p;
			record_chunk.length_words   = words_to_write;
			// Set up record.
			record.file_id = USER_DEFINED_PROPERTIES_FILE_ID;
			record.data.p_chunks = &record_chunk;
			record.data.num_chunks = 1;
			
			//check if the record already exists
			ret = applicationFdsApp_data_find(record.file_id, record.key, &record_desc);

			if (ret == NRF_ERROR_NOT_FOUND){
				//write a new record
				ret = fds_record_write(&record_desc, &record);
			} else {
				// NRF_SUCCESS
				// Update existing record.
				ret = fds_record_update(&record_desc, &record);
			}

			if(ret != FDS_SUCCESS){
				NRF_LOG_ERROR("applicationFdsApp_fds_store:fds_record_write/update failed with ret= %d\r\n", ret);
				ret_code= false;;
			}
			
		}
		break;
					
		default: 
			NRF_LOG_ERROR("applicationFdsApp_fds_store: called with invalid data_type= %d\r\n", data_type);
			ret_code= false;
		break;
	}
	
	return ret_code;
}

bool applicationFdsApp_init(void){
	
	bool ret_code  = true;
	ret_code_t ret;

	if (!m_app_fds_initialized){
		
		ret = fds_register(applicationFdsApp_evt_handler);
		if (ret != NRF_SUCCESS)
		{
			return NRF_ERROR_INTERNAL;
		}

		ret = fds_init();
		if (ret != NRF_SUCCESS)
		{
			return NRF_ERROR_NO_MEM;
		}

		m_app_fds_initialized = true;
		
	}
	
	return ret_code;
}

