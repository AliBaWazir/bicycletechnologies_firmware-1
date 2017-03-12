#ifndef DRIVERS_COMMON_H__
#define DRIVERS_COMMON_H__

#include <stdint.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif

#define ALGORITHM_PRINTS_ALL_DATA 1  //if set to 1, algorithm app will print data for all sensors and algorithm output    
	
#define CRANK_IDENTIFIER 0xCA
#define WHEEL_IDENTIFIER 0xEE
	
#define MAX_GEARS_COUNT     16    //defines max of number of gears in crank/wheel
#define MAX_BLE_SENSOR_MEAS 0xEF  // max BLE sensor measuremnt
/**********************************************************************************************
* CSCS and HR
***********************************************************************************************/
//enum to specify what CSCS data to handle
typedef enum {
	CSCS_DATA_SPEED,
	CSCS_DATA_CADENCE,
	CSCS_DATA_DISTANCE,
	CSCS_DATA_HR
} cscs_data_type_e;

/* not needed
typedef enum{
	GEAR_TYPE_CRANK,
	GEAR_TYPE_WHEEL,
	GEAR_TYPE_UNKNOWN
} gear_type_e;
*/

typedef struct{
	uint32_t value;
	bool     is_read;
}uint32_data_field_t;

typedef struct{
	uint16_t value;
	bool     is_read;
}uint16_data_field_t;

typedef struct{
	double   value;
	bool     is_read;
}double_data_field_t;

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

extern uint32_t cadence_setpoint_rpm;
extern user_defined_bike_config_data_t  user_defined_bike_config_data;

typedef bool (*algorithmApp_ratios_poulate_f)(void);

/**********************************************************************************************
* SPI and other apps
***********************************************************************************************/
#define SPI_NO_NEW_MEAS  0xFE         //This value will be retund to SPI master if nRF doesn't have a new measurement
typedef enum {
	//there are 32 bits. Every bit correponds to a data avaialble flag. The following are the indeces of the flags.

	//Group 0: Shifting Algorithm Parameters	
	SPI_AVAIL_FLAG_SPEED                  =(0x01<<0),
	SPI_AVAIL_FLAG_CADENCE                =(0x01<<1),
	SPI_AVAIL_FLAG_DISTANCE               =(0x01<<2),
	SPI_AVAIL_FLAG_HR                     =(0x01<<3),
	SPI_AVAIL_FLAG_BATTERY                =(0x01<<4),
	//Group 1: Bike configuration
	SPI_AVAIL_FLAG_BIKE_CONFIG_PRAMS      =(0x01<<8),
	//Group 2: Bluetooth Device Configuration
	SPI_AVAIL_FLAG_ADVER_DEVICES_COUNT    =(0x01<<16),
	SPI_AVAIL_FLAG_PAIRED_DEVICES         =(0x01<<17),
	SPI_AVAIL_FLAG_CONNECTED_DEVICES      =(0x01<<18),
	SPI_AVAIL_FLAG_CSC_DEVICE_NAME        =(0x01<<19), 
	SPI_AVAIL_FLAG_HR_DEVICE_NAME         =(0x01<<20),
	SPI_AVAIL_FLAG_PHONE_DEVICE_NAME      =(0x01<<21)
}spi_data_avail_flag_e;

typedef void (*new_meas_callback_f)(spi_data_avail_flag_e flag, bool data_available);


#ifdef __cplusplus
}
#endif

#endif // DRIVERS_COMMON_H__
