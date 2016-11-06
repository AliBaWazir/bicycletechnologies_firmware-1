#ifndef _TRACE_H_
#define _TRACE_H_

#include "gfx.h"
#include "tm_stm32_rtc.h"
#include "tm_stm32_gps.h"

#define DEBUG

extern GFILE *myfile;

extern osMutexId traceMutex;

typedef struct {
	float Latitude;                                       /*!< Latitude position from GPS, -90 to 90 degrees response. */
	float Longitude;                                      /*!< Longitude position from GPS, -180 to 180 degrees response. */
	float Altitude;                                       /*!< Altitude above the seain units of meters */
	uint8_t Satellites;                                   /*!< Number of satellites in use for GPS position. */
	float Direction;                                      /*!< Course on the ground in relation to North. */
	uint8_t Validity;																			/*!< GPS validation; 1: valid; 0: invalid. */
} my_GPS;

void deleteTraceFile(void);
void closeTraceFile(void);
void openTraceFile(void);
void TRACE(const char *fmt, ...);

TM_RTC_Result_t updateRTC(TM_RTC_t* data, TM_RTC_Format_t format);
TM_RTC_Result_t getRTC(TM_RTC_t* data, TM_RTC_Format_t format);

void saveGPS(TM_GPS_Data_t* gpsData);
my_GPS getGPS();

#endif /* _TRACE_H_ */
