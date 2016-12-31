#ifndef _SPI_H_
#define _SPI_H_

#include <stdbool.h>
#include <stdint.h>

//initialize the SPI data struct containing all the data coming from the NRF51822
struct SPI_data{
	struct Availability{
		uint8_t value[4];
		uint32_t age;
		uint8_t key;
	}avail;
    struct Speed{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }speed;
    struct Cadence{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }cadence;
		struct Distance{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }distance;
    struct HeartRate{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }heartRate;
    struct Batt{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }batt;
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