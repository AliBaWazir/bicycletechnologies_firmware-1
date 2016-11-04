#include "trace.h"
#include "tm_stm32_gps.h"
#include "tm_stm32_delay.h"

void runGPS(){
	bool isRTCSet = false;
	TM_GPS_Data_t GPS_Data;
	TM_GPS_Result_t result, current;
	TM_GPS_Float_t GPS_Float_Lat;
	TM_GPS_Float_t GPS_Float_Lon;
	TM_GPS_Float_t GPS_Float_Alt;
	TM_GPS_Distance_t GPS_Distance;
	char buffer[40];
	float temp;
	
		/* Delay init */
	TM_DELAY_Init();
	
	/* Initialize GPS on 9600 baudrate */
	TM_GPS_Init(&GPS_Data, 9600);
	
		/* Reset counter */
	TM_DELAY_SetTime(0);
	int validCount = 0;
	while(1) {
		/* Update GPR data */
		/* Call this as faster as possible */
		result = TM_GPS_Update(&GPS_Data);
		/* If we didn't receive any useful data in the start */
		if (result == TM_GPS_Result_FirstDataWaiting && TM_DELAY_Time() > 3000) {
			/* If we didn't receive nothing within 3 seconds */
			TM_DELAY_SetTime(0);
			TRACE("GPS: nothing received after 3 seconds\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "GPS: nothing received after 3 seconds\n");
#endif	
		}
		/* If we have any unread data */
		if (result == TM_GPS_Result_NewData) {
			/* We received new packet of useful data from GPS */
			current = TM_GPS_Result_NewData;
			
			/* Is GPS signal valid? */
			if (GPS_Data.Validity) {
				/* If you want to make a GPS tracker, now is the time to save your data on SD card */
				if((!isRTCSet) && (validCount > 5)){
#ifdef DEBUG
					TM_USART_Puts(USART3, "GPS: Update RTC\n");
#endif	
					TM_RTC_t rtcd;
					getRTC(&rtcd, TM_RTC_Format_BIN);
					uint32_t rtcTime = TM_RTC_GetUnixTimeStamp(&rtcd);
					rtcd.Year = GPS_Data.Date.Year;
					rtcd.Month = GPS_Data.Date.Month;
					rtcd.Day = GPS_Data.Date.Date;
					rtcd.Hours = GPS_Data.Time.Hours;
					rtcd.Minutes = GPS_Data.Time.Minutes;
					rtcd.Seconds = GPS_Data.Time.Seconds;
					rtcd.Subseconds = GPS_Data.Time.Hundredths;
					uint32_t gpsTime = TM_RTC_GetUnixTimeStamp(&rtcd);
					
					if ( ((rtcTime >= gpsTime) && ((rtcTime-gpsTime)>5)) || ((gpsTime >= rtcTime) && ((gpsTime-rtcTime)>5))){
						if(updateRTC(&rtcd, TM_RTC_Format_BIN) != TM_RTC_Result_Ok){
							TRACE("ERROR: Saving RTC Based on GPS Failed\n");
#ifdef DEBUG
							TM_USART_Puts(USART3, "ERROR: Saving RTC Based on GPS Failed\n.");
#endif	
						};
						closeTraceFile();
						openTraceFile();
					}
					isRTCSet = true;
				}
				
				/* We have valid GPS signal */
				TRACE("GPS: New GPS Data Received\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "GPS: New GPS Data Received\n");
#endif	
#ifndef GPS_DISABLE_GPGGA
				/* Latitude */
				/* Convert float to integer and decimal part, with 6 decimal places */
				TM_GPS_ConvertFloat(GPS_Data.Latitude, &GPS_Float_Lat, 6);
				
				/* Longitude */
				/* Convert float to integer and decimal part, with 6 decimal places */
				TM_GPS_ConvertFloat(GPS_Data.Longitude, &GPS_Float_Lon, 6);
				
				/* Altitude */
				/* Convert float to integer and decimal part, with 6 decimal places */
				TM_GPS_ConvertFloat(GPS_Data.Altitude, &GPS_Float_Alt, 6);

				TRACE("Latitude=%d.%d,Longitude=%d.%d,SatsInUse=%02d,Date=% 4d/%02d/%02d,UTCTime=%02d.%02d.%02d:%02d,Fix=%d\n", GPS_Float_Lat.Integer, GPS_Float_Lat.Decimal, 
				GPS_Float_Lon.Integer, GPS_Float_Lon.Decimal, GPS_Data.Satellites, GPS_Data.Date.Year, GPS_Data.Date.Month, GPS_Data.Date.Date,
				GPS_Data.Time.Hours, GPS_Data.Time.Minutes, GPS_Data.Time.Seconds, GPS_Data.Time.Hundredths, 
				GPS_Data.Fix, GPS_Float_Alt.Integer, GPS_Float_Alt.Decimal); 
#endif
				if(validCount < 35){
					validCount++;
				}
			} else {
				/* GPS signal is not valid */
				TRACE("GPS: Data received is not valid\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "GPS: Data received is not valid\n");
#endif	
			}
		} else if (result == TM_GPS_Result_FirstDataWaiting && current != TM_GPS_Result_FirstDataWaiting) {
			current = TM_GPS_Result_FirstDataWaiting;
			//TM_USART_Puts(USART3, "Waiting first data from GPS!\n");
		} else if (result == TM_GPS_Result_OldData && current != TM_GPS_Result_OldData) {
			current = TM_GPS_Result_OldData;
			/* We already read data, nothing new was received from GPS */
		}
	}
	
}