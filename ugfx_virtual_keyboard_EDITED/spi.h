#ifndef _SPI_H_
#define _SPI_H_

#include <stdbool.h>
#include <stdint.h>
#include "gui.h"

#define DUMMY_VALUE			0xF0

#define FLAG_SPEED			0x01
#define FLAG_CADENCE 		0x02
#define FLAG_DISTANCE		0x04
#define FLAG_HEARTRATE	0x08
#define FLAG_BATTERY		0x10

#define FLAG_DEV_COUNT		0x01
#define	FLAG_PAIRED_DEV		0x02
#define FLAG_CONN_DEV			0x04
#define FLAG_CSC_DEV_NAME	0x08
#define FLAG_HR_DEV_NAME	0x10
#define FLAG_PHO_DEV_NAME	0x20

#define DATA_VALID_TIME		1
#define DATA_INVALID_TIME	2

#define MAXIMUM_SPEED							0xEF
#define MAXIMUM_CADENCE						0xEF
#define MAXIMUM_DISTANCE					0xEF
#define MAXIMUM_HEART_RATE				0xEF
#define MAXIMUM_CADENCE_SET_POINT	0xEF
#define MAXIMUM_BATTERY						0x64
#define MAXIMUM_WHEEL_DIAMETER		0xEF

#define GEAR_COMMAND_FRONT	0xCA
#define GEAR_COMMAND_BACK		0xEE

#define NRF_SCAN_PERIOD			10 //Seconds

//initialize the SPI data struct containing all the data coming from the NRF51822
struct SPI_data{
	struct Availability{
		uint8_t value[4];
		uint32_t age;
	}avail;
	struct Speed{
			uint8_t value;
			uint32_t age;
	}speed;
	struct Cadence{
			uint8_t value;
			uint32_t age;
	}cadence;
	struct Distance{
			uint8_t value;
			uint32_t age;
	}distance;
	struct HeartRate{
			uint8_t value;
			uint32_t age;
	}heartRate;
	struct CadenceSetPoint{
			uint8_t value;
			uint32_t age;
	}cadenceSetPoint;
	struct Batt{
			uint8_t value;
			uint32_t age;
	}batt;
	struct WheelDiameter{
			uint8_t value;
			uint32_t age;
	}wheelDiameter;
	struct Gears{
			uint8_t frontGears[MAXIMUM_FRONT_GEARS+1];
			uint8_t backGears[MAXIMUM_BACK_GEARS+1];
			uint32_t age;
	}gears;
	struct Bluetooth{
		uint8_t deviceCount;
		uint32_t age;
	}bluetooth;
};

void runSPI(void);
void nrfSetup(void);

#endif /* _SPI_H_ */