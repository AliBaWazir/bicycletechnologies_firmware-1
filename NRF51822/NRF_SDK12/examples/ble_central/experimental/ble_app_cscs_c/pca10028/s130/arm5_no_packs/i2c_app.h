#ifndef I2C_APP_H__
#define I2C_APP_H__

#include <stdint.h>
#include <stdbool.h>

#include "drivers_commons.h"

#ifdef __cplusplus
extern "C" {
#endif

void i2cApp_assing_new_meas_callback(new_meas_callback_f cb);
uint8_t i2cApp_get_battery_level(void);
bool i2cApp_write_desired_gears(uint8_t gear_index_front, uint8_t gear_index_back);
void i2cApp_wait(void);
bool i2cApp_init(void);


#ifdef __cplusplus
}
#endif

#endif // I2C_APP_H__

