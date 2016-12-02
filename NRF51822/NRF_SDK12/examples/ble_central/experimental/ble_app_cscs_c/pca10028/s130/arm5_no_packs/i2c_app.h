#ifndef I2C_APP_H__
#define I2C_APP_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void i2cApp_wait(void);
bool i2cApp_init(void);


#ifdef __cplusplus
}
#endif

#endif // I2C_APP_H__

