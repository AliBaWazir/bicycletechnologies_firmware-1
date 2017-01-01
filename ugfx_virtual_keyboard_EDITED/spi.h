#ifndef _SPI_H_
#define _SPI_H_

#include <stdbool.h>
#include <stdint.h>
#include "gui.h"

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
	struct Gears{
			uint8_t frontGears[MAXIMUM_FRONT_GEARS+1];
			uint8_t backGears[MAXIMUM_BACK_GEARS+1];
			uint32_t age;
	}gears;
};

/*
bool nrfGetData(void);
void runTestCase(uint8_t* target);
bool nrfTransmitSingle(uint8_t* out, uint8_t* in);
bool nrfTransmit(uint8_t *buffOut, uint8_t *buffIn, uint32_t len);
bool nrfTransmit2(uint8_t *buffOut, uint8_t *buffIn, uint32_t len);
*/

void runSPI(void);
void nrfSetup(void);

#endif /* _SPI_H_ */