#ifndef CSCS_APP_H__
#define CSCS_APP_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble_cscs_c_developed.h"


#ifdef __cplusplus
extern "C" {
#endif

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
