#ifndef I2C_SIM_DRIVER_H__
#define I2C_SIM_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

#include "nrf_drv_twi.h"


#ifdef __cplusplus
extern "C" {
#endif

bool i2cSimDriver_init(const nrf_drv_twi_t* twi, bool* xfer_done_p);


#ifdef __cplusplus
}
#endif

#endif // I2C_SIM_DRIVER_H__

