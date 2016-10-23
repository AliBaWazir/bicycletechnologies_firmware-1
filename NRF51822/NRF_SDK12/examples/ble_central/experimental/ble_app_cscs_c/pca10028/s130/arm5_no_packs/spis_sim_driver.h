#ifndef SPIS_SIM_DRIVER_H__
#define SPIS_SIM_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "drivers_commons.h"


#ifdef __cplusplus
extern "C" {
#endif

uint8_t spisSimDriver_get_current_data(cscs_data_type_e data_type);
bool spisSimDriver_init();


#ifdef __cplusplus
}
#endif

#endif // SPIS_SIM_DRIVER_H__
