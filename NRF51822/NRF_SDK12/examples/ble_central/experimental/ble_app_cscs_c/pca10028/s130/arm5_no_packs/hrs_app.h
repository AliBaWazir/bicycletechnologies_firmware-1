#ifndef HRS_APP_H__
#define HRS_APP_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble_cscs_c_developed.h"
#include "drivers_commons.h"



#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	HRS_APP_RET_CODE_SUCCESS,
	HRS_APP_RET_CODE_SENSOR_ERROR,
	HRS_APP_RET_CODE_BAD_ARGUMENT,
	HRS_RET_CODE_UNKNOWN_ERROR
}hrsApp_ret_code_e;
/**
 * @brief Cycling Speed and Cadence collector initialization.
 */
void hrsApp_on_ble_evt(const ble_evt_t *p_ble_evt);
void hrsApp_on_db_disc_evt(const ble_db_discovery_evt_t *p_evt);
void hrsApp_on_ble_event(const ble_evt_t *p_ble_evt);

uint8_t hrsApp_get_current_hr_bpm(void);
float hrsApp_get_curr_hr_deviation(void);

void hrsApp_assing_new_meas_callback(new_meas_callback_f cb);

bool hrsApp_hrs_c_init(void);

#ifdef __cplusplus
}
#endif

#endif // HRS_APP_H__
