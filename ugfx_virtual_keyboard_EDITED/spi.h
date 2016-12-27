#ifndef _SPI_H_
#define _SPI_H_

#include <stdbool.h>
#include <stdint.h>

//initialize the SPI data struct containing all the data coming from the NRF51822
struct SPI_data{
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

void nrfSetup(void);
bool nrfGetData(void);
void runTestCase(uint8_t* target);
bool nrfTransmitSingle(uint8_t* in, uint8_t* out);
bool nrfTransmit(uint8_t *buffIn, uint8_t *buffOut, uint32_t len);
bool nrfTransmit2(uint8_t *buffIn, uint8_t *buffOut, uint32_t len);

#endif /* _SPI_H_ */