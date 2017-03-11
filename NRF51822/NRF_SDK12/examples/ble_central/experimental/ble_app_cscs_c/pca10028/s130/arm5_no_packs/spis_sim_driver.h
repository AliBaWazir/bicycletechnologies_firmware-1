#ifndef SPIS_SIM_DRIVER_H__
#define SPIS_SIM_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "drivers_commons.h"


#ifdef __cplusplus
extern "C" {
#endif
	
bool spisSimDriver_get_data_availability_flags(uint8_t* dest_buffer);

void spisSimDriver_set_cadence_setpoint (uint8_t new_cadence_setpoint);
void spisSimDriver_set_wheel_diameter (uint8_t new_wheel_diameter);
void spisSimDriver_set_gear_count (uint8_t gear_type, uint8_t new_gear_count);
void spisSimDriver_set_teeth_count (uint8_t gear_type, uint8_t gear_index, uint8_t new_gear_teeth);

uint8_t spisSimDriver_get_current_battery(void);
uint8_t spisSimDriver_get_current_data (cscs_data_type_e data_type);
bool spisSimDriver_init (void);


#ifdef __cplusplus
}
#endif

#endif // SPIS_SIM_DRIVER_H__
