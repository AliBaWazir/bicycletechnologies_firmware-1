#ifndef CSCS_APP_H__
#define CSCS_APP_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble_cscs_c_developed.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	CSCS_APP_RET_CODE_SUCCESS,
	CSCS_APP_RET_CODE_SENSOR_ERROR,
	CSCS_APP_RET_CODE_BAD_ARGUMENT,
	CSCS_RET_CODE_UNKNOWN_ERROR
}cscsApp_ret_code_e;
/**
 * @brief Cycling Speed and Cadence collector initialization.
 */
	void cscsApp_on_ble_evt(const ble_evt_t *p_ble_evt);
	void cscsApp_on_db_disc_evt(const ble_db_discovery_evt_t *p_evt);
	void cscsApp_on_ble_event(const ble_evt_t *p_ble_evt);
	void cscsApp_cscs_c_init(void);

#ifdef __cplusplus
}
#endif

#endif // CSCS_APP_H__
