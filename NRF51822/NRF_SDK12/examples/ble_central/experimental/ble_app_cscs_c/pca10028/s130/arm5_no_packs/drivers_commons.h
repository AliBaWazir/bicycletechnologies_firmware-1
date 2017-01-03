#ifndef DRIVERS_COMMON_H__
#define DRIVERS_COMMON_H__

#include <stdint.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif

	
#define CRANK_IDENTIFIER 0xCA
#define WHEEL_IDENTIFIER 0xEE
	
#define MAX_GEARS_COUNT  16    //defines max of number of gears in crank/wheel

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
	uint32_t  wheel_diameter_cm;
	uint32_t  crank_gears_count;
	uint32_t  wheel_gears_count;
	uint32_t  crank_gears_teeth_count [MAX_GEARS_COUNT];
	uint32_t  wheel_gears_teeth_count [MAX_GEARS_COUNT];
} user_defined_bike_config_data_t;

typedef enum{
	USER_DEFINED_CADENCE_SETPOINT,
	USER_DEFINED_BIKE_CONFIG_DATA
} user_defined_properties_type_e;


/* not needed
typedef enum{
	GEAR_TYPE_CRANK,
	GEAR_TYPE_WHEEL,
	GEAR_TYPE_UNKNOWN
} gear_type_e;
*/

#ifdef __cplusplus
}
#endif

#endif // DRIVERS_COMMON_H__
