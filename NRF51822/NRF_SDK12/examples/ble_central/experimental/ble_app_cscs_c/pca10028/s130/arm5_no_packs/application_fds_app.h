#ifndef APPLICATION_FDS_APP_H__
#define APPLICATION_FDS_APP_H__

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"

#include "drivers_commons.h"

#ifdef __cplusplus
extern "C" {
#endif
	
bool applicationFdsApp_fds_read(user_defined_properties_type_e data_type, const void* dest_p);
bool applicationFdsApp_fds_store(user_defined_properties_type_e data_type, uint8_t* source_p);
	
bool applicationFdsApp_init(void);

#ifdef __cplusplus
}
#endif

#endif // APPLICATION_FDS_APP_H__

