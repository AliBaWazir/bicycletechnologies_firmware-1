#ifndef SPIS_APP_H__
#define SPIS_APP_H__

#include <stdint.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif


	
bool spisApp_init(void);
    
void spi_wait(void);
	
#ifdef __cplusplus
}
#endif

#endif // SPIS_APP_H__
