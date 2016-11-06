#ifndef CONN_MANAGER_APP_H__
#define CONN_MANAGER_APP_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_gap.h"

typedef enum{
	ADVERTISED_DEVICE_TYPE_CSCS_SENSOR =1,
	ADVERTISED_DEVICE_TYPE_HRS_SENSOR,
	ADVERTISED_DEVICE_TYPE_UNKNOWN
}advertised_device_type_e;

void connManagerApp_debug_print_conn_params(const ble_gap_conn_params_t* conn_params);
void connManagerApp_conn_params_update (const ble_gap_conn_params_t* conn_params);
bool connManagerApp_get_memory_access_in_progress (void);
void connManagerApp_set_memory_access_in_progress (bool is_in_progress);
bool connManagerApp_scan_start(void);
void connManagerApp_whitelist_disable(void);
bool connManagerApp_advertised_device_connect(uint8_t advertised_device_id);
bool connManagerApp_advertised_device_store(advertised_device_type_e device_type, const ble_gap_evt_adv_report_t* adv_report);
bool connManagerApp_init(void);



#ifdef __cplusplus
}
#endif

#endif // CONN_MANAGER_APP_H__

