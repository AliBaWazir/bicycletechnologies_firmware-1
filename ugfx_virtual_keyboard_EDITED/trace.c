
#include "trace.h"
#include <stdio.h>
#include <string.h>

//SDCard Stuff
GFILE *myfile;

osMutexId traceMutex;

my_GPS myGPSData;

void closeTraceFile(void)
{
#ifdef DEBUG
					TM_USART_Puts(USART3, "Closing File\n");
#endif	
	gfileClose(myfile);
}

void openTraceFile(void)
{	
#ifdef DEBUG
					TM_USART_Puts(USART3, "Opening File\n");
#endif	
	TM_RTC_t rtcd;
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK)  {
		// handle failure code
	}
	
	char timedBuffer[128];
	TM_RTC_GetDateTime(&rtcd, TM_RTC_Format_BIN);
	sprintf(timedBuffer, "%d_%02d_%02d-%02d_%02d_%02d.csv",rtcd.Year,rtcd.Month,rtcd.Day,rtcd.Hours,rtcd.Minutes,rtcd.Seconds);
#ifdef DEBUG
	char buffer[50];
	sprintf(buffer, "Opening File: %d_%02d_%02d-%02d_%02d_%02d.csv\n",rtcd.Year,rtcd.Month,rtcd.Day,rtcd.Hours,rtcd.Minutes,rtcd.Seconds);
	TM_USART_Puts(USART3, buffer);
#endif		
	if(gfileExists(timedBuffer)){
		gfileDelete(timedBuffer);
	}
	myfile = gfileOpen(timedBuffer, "w");
	
	myGPSData.Validity = 0;
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}
}

void TRACE(const char *fmt, ...)
{
	TM_RTC_t rtcd;
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK){
		// handle failure code
	}
	char buffer[256];
	char timedBuffer[512];
	int charcount = 0;
	
	TM_RTC_GetDateTime(&rtcd, TM_RTC_Format_BIN);
	charcount += sprintf(timedBuffer, "[%d/%02d/%02d || %02d:%02d:%02d],",rtcd.Year,rtcd.Month,rtcd.Day,rtcd.Hours,rtcd.Minutes,rtcd.Seconds);
#ifdef DEBUG
	sprintf(buffer, "Saving Into SDCard: [%d/%02d/%02d || %02d:%02d:%02d]\n",rtcd.Year,rtcd.Month,rtcd.Day,rtcd.Hours,rtcd.Minutes,rtcd.Seconds);
	TM_USART_Puts(USART3, buffer);
#endif
  va_list args;
  va_start (args, fmt);
  charcount += vsprintf (buffer, fmt, args);
	strcat(timedBuffer, buffer);
	gfileWrite(myfile, timedBuffer, charcount);
	va_end(args);
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}
}

TM_RTC_Result_t updateRTC(TM_RTC_t* data, TM_RTC_Format_t format)
{
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK){
		// handle failure code
	}
#ifdef DEBUG
	char buffer[50];
	sprintf(buffer, "Updating the RTC with: [%d/%02d/%02d || %02d:%02d:%02d]\n",data->Year,data->Month,data->Day,data->Hours,data->Minutes,data->Seconds);
	TM_USART_Puts(USART3, buffer);
#endif				
	TM_RTC_SetDateTime(data, format);
	
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}	
}

TM_RTC_Result_t getRTC(TM_RTC_t* data, TM_RTC_Format_t format)
{
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK){
		// handle failure code
	}
	
	TM_RTC_GetDateTime(data, format);
	
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}	
}

void saveGPS(TM_GPS_Data_t* gpsData){
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK){
		// handle failure code
	}
	
	myGPSData.Latitude = gpsData->Latitude;
	myGPSData.Longitude = gpsData->Longitude;
	myGPSData.Altitude = gpsData->Altitude;
	myGPSData.Direction = gpsData->Direction;
	myGPSData.Validity = gpsData->Validity;
	
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}	
}

my_GPS getGPS(){
	my_GPS temp;
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK){
		// handle failure code
	}
	
	temp = myGPSData;
	
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}	
	return temp;
}