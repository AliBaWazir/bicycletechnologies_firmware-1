#ifndef DRIVERS_COMMON_H__
#define DRIVERS_COMMON_H__

#include <stdint.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif

	
#define CRANK_IDENTIFIER 1
#define WHEEL_IDENTIFIER 2

/**********************************************************************************************
* CSCS
***********************************************************************************************/	
//enum to specify what CSCS data to handle
typedef enum {
	CSCS_DATA_SPEED,
	CSCS_DATA_CADENCE,
	CSCS_DATA_DISTANCE,
	CSCS_DATA_HR
} cscs_data_type_e;



/**********************************************************************************************
* ROM STORED DATA
***********************************************************************************************/
typedef struct{
	uint8_t cadence_setpoint;
	uint8_t wheel_diameter;
	uint8_t crank_gear_count;
	uint8_t wheel_gear_count;
	/*TODO: figure out how teeth count can be stored in ROM, 3D array?*/
	
} user_defined_properties_t;

#ifdef __cplusplus
}
#endif

#endif // DRIVERS_COMMON_H__
