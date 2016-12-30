#ifndef ALGORITHM_APP_H__
#define ALGORITHM_APP_H__

#include <stdint.h>
#include <stdbool.h>

#include "drivers_commons.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t algorithmApp_get_cadence_setpoint(void);
uint8_t algorithmApp_get_wheel_diameter_cm(void);
uint8_t algorithmApp_get_gears_count_crank(void);
uint8_t algorithmApp_get_gears_count_wheel(void);
uint8_t algorithmApp_get_teeth_count(uint8_t gear_type, uint8_t gear_index);
	
void algorithmApp_set_cadence_setpoint (uint8_t new_cadence_setpoint);
void algorithmApp_set_wheel_diameter (uint8_t new_wheel_diameter);
void algorithmApp_set_gear_count (uint8_t gear_type, uint8_t new_gear_count);
void algorithmApp_set_teeth_count (uint8_t gear_type, uint8_t gear_index, uint8_t new_gear_teeth);
void algorithmApp_set_gear_level_locked(void);
	
bool algorithmApp_init(void);
    
	
#ifdef __cplusplus
}
#endif

#endif // ALGORITHM_APP_H__
