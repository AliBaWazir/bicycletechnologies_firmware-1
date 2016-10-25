
#include "trace.h"
#include <stdio.h>
#include <string.h>

//SDCard Stuff
GFILE *myfile;

osMutexId traceMutex;

void closeTraceFile(void)
{
	gfileClose(myfile);
}

void openTraceFile(void)
{	
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK)  {
		// handle failure code
	}
	
	char timedBuffer[128];
	TM_RTC_GetDateTime(&RTCD, TM_RTC_Format_BIN);
	sprintf(timedBuffer, "%d_%d_%d-%d_%d_%d.csv",RTCD.Year,RTCD.Month,RTCD.Day,RTCD.Hours,RTCD.Minutes,RTCD.Seconds);
	
	if(gfileExists(timedBuffer)){
		gfileDelete(timedBuffer);
	}
	myfile = gfileOpen(timedBuffer, "w");
	
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}
}

void TRACE(const char *fmt, ...)
{
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK){
		// handle failure code
	}
	char buffer[256];
	char timedBuffer[512];
	int charcount = 0;
	
	TM_RTC_GetDateTime(&RTCD, TM_RTC_Format_BIN);
	charcount += sprintf(timedBuffer, "[%d/%d/%d || %d:%d:%d],",RTCD.Year,RTCD.Month,RTCD.Day,RTCD.Hours,RTCD.Minutes,RTCD.Seconds);
	
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
	
	TM_RTC_SetDateTime(data, format);
	
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}	
}