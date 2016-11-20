#ifndef CONN_MANAGER_APP_H__
#define CONN_MANAGER_APP_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_gap.h"
	
#define SCANNING_WAITING_PERIOD_MS     10000       //during this period central will continue scanning for any advertising peripherals

typedef enum{
	ADVERTISED_DEVICE_TYPE_UNKNOWN = 0,
	ADVERTISED_DEVICE_TYPE_CSCS_SENSOR =1,
	ADVERTISED_DEVICE_TYPE_HRS_SENSOR,
	ADVERTISED_DEVICE_TYPE_PHONE
}advertised_device_type_e;

void connManagerApp_debug_print_conn_params(const ble_gap_conn_params_t* conn_params);
void connManagerApp_map_conn_handler_to_device_type (advertised_device_type_e device_type, const uint16_t conn_handle);
void connManagerApp_conn_params_update (const uint16_t conn_handle, const ble_gap_conn_params_t* conn_params);
bool connManagerApp_get_memory_access_in_progress (void);
void connManagerApp_set_memory_access_in_progress (bool is_in_progress);
bool connManagerApp_scan_start(uint32_t scanning_interval_ms);
void connManagerApp_whitelist_disable(void);
bool connManagerApp_advertised_device_connect(uint8_t advertised_device_id);
bool connManagerApp_advertised_device_store(advertised_device_type_e device_type, const ble_gap_evt_adv_report_t* adv_report);
advertised_device_type_e connManagerApp_get_device_type (const ble_gap_addr_t *peer_addr);
void connManagerApp_on_disconnection (ble_gap_evt_t* evt);
bool connManagerApp_init(void);



#ifdef __cplusplus
}
#endif

#endif // CONN_MANAGER_APP_H__

