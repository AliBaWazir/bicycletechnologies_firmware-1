#ifndef SPIS_APP_H__
#define SPIS_APP_H__

#include <stdint.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif

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


void spisApp_update_data_avail_flags(spi_data_avail_flag_e flag, bool data_available);	
bool spisApp_init(void);
    
void spisApp_spi_wait(void);
	
#ifdef __cplusplus
}
#endif

#endif // SPIS_APP_H__
