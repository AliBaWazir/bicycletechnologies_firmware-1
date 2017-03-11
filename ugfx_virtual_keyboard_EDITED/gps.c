#include <cmath>
#include "trace.h"
#include "tm_stm32_gps.h"
#include "tm_stm32_delay.h"
#include "msg.h"
#include "gps.h"

#define SIMULATE_GPS

#define M_PI (3.141592653589793)

my_GPS gpsData;
static TM_GPS_Data_t GPS_Data;
static TM_GPS_Float_t GPS_Float_Lat;
static TM_GPS_Float_t GPS_Float_Lon;
static TM_GPS_Float_t GPS_Float_Alt;
static TM_GPS_Distance_t GPS_Distance;
bool isRTCSet;
static GDisplay* pixmap;
static pixel_t* surface;

void retrieveGPS();
void sendGPSMSG();
void newGPSData();
void drawTilePixmap(int tilex, int tiley, int tilexOffset, int tileyOffset);

osMessageQDef(gpsQueue, 32, message_t);
osMessageQId  gpsQueue;

char gpsOutput[70];
static gdispImage myImage[9];
static gdispImage marker;

int oldtilex=0;
int oldtiley=0;
int oldtilexOffset=0;
int oldtileyOffset=0;

int zoom_level = 14;

#ifdef SIMULATE_GPS
int mapDisplayCount = 0;
#endif

int long2tilex(double lon, int zoomlevel, int *tileOffset) 
{ 
	double tmp = (lon + 180.0) / 360.0 * pow(2.0, zoomlevel);
	int ret = (int)(floor(tmp));
	*tileOffset = (int)((tmp - ret)*256);
	return ret; 
}

int lat2tiley(double lat, int zoomlevel, int *tileOffset)
{ 
	double tmp = (1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, zoomlevel);
	int ret = (int)(floor(tmp));
	*tileOffset = (int)((tmp - ret)*256);
	return ret; 
}

double tilex2long(int x, int z) 
{
	return x / pow(2.0, z) * 360.0 - 180;
}

double tiley2lat(int y, int z) 
{
	double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
	return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

void runGPS(){	
		/* Delay init */
	TM_DELAY_Init();
	
	/* Initialize GPS on 9600 baudrate */
	TM_GPS_Init(&GPS_Data, 9600);
	
	isRTCSet = false;
	
	pixmap = gdispPixmapCreate((coord_t)494,(coord_t)480);
	
	gpsQueue = osMessageCreate(osMessageQ(gpsQueue), NULL);
	
	message_t *messageReceived;
	while(1) {
		osEvent evt = osMessageGet(gpsQueue, osWaitForever);
		if (evt.status == osEventMessage) {
			messageReceived = (message_t*)evt.value.p;
			if(messageReceived->msg_ID == GET_GPS_MSG){
				TRACE("GPS:,GET_GPS_MSG\n");
				retrieveGPS();
			}
			osPoolFree(mpool, messageReceived);
		}
	}
}

void retrieveGPS(){
	TM_GPS_Result_t result, current;
	char buffer[40];
	float temp;
	
#ifdef SIMULATE_GPS
	sendGPSMSG();
#else	
	TM_DELAY_SetTime(0);
	
	/* Update GPR data */
	/* Call this as faster as possible */
	result = TM_GPS_Update(&GPS_Data);
	/* If we didn't receive any useful data in the start */
	if (result == TM_GPS_Result_FirstDataWaiting && TM_DELAY_Time() > 3000) {
		/* If we didn't receive nothing within 3 seconds */
		TM_DELAY_SetTime(0);
		TRACE("GPS:,nothing received after 3 seconds\n");
	}
	/* If we have any unread data */
	if (result == TM_GPS_Result_NewData) {
		/* We received new packet of useful data from GPS */
		current = TM_GPS_Result_NewData;

		/* Is GPS signal valid? */
		if (GPS_Data.Validity) {
			/* If you want to make a GPS tracker, now is the time to save your data on SD card */
			if(!isRTCSet){
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
						TRACE("GPS:,ERROR: Saving RTC Based on GPS Failed\n");
					};
					closeTraceFile();
					openTraceFile();
					TRACE("GPS:,Updated RTC based on GPS\n");
				}
				isRTCSet = true;
			}
			
			/* We have valid GPS signal */
			TRACE("GPS:,New GPS Data Received\n");
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

			TRACE("GPS:,Latitude=%d.%d,Longitude=%d.%d,SatsInUse=%02d,Date=% 4d/%02d/%02d,UTCTime=%02d.%02d.%02d:%02d,Fix=%d\n", GPS_Float_Lat.Integer, GPS_Float_Lat.Decimal, 
			GPS_Float_Lon.Integer, GPS_Float_Lon.Decimal, GPS_Data.Satellites, GPS_Data.Date.Year, GPS_Data.Date.Month, GPS_Data.Date.Date,
			GPS_Data.Time.Hours, GPS_Data.Time.Minutes, GPS_Data.Time.Seconds, GPS_Data.Time.Hundredths, 
			GPS_Data.Fix, GPS_Float_Alt.Integer, GPS_Float_Alt.Decimal); 
#endif
			sendGPSMSG();
		} else {
			/* GPS signal is not valid */
			TRACE("GPS:,Data received is not valid\n");
		}
	} else if (result == TM_GPS_Result_FirstDataWaiting && current != TM_GPS_Result_FirstDataWaiting) {
		current = TM_GPS_Result_FirstDataWaiting;
	} else if (result == TM_GPS_Result_OldData && current != TM_GPS_Result_OldData) {
		current = TM_GPS_Result_OldData;
	}
#endif
}

void sendGPSMSG(){
	gpsData.Validity = GPS_Data.Validity;
	gpsData.Latitude = GPS_Data.Latitude;
	gpsData.Longitude = GPS_Data.Longitude;
	newGPSData();
	
	message_t *messageSent;
	messageSent = (message_t*)osPoolAlloc(mpool);
	messageSent->msg_ID = GET_GPS_MSG;
	osMessagePut(guiQueue, (uint32_t)messageSent, 0);
	oldtilex=0;
	oldtiley=0;
}

void newGPSData(){
	int tilex;
	int tiley;
	int tilexOffset;
	int tileyOffset;

#ifdef SIMULATE_GPS
	gpsData.Validity = true;
	if(mapDisplayCount == 0){
		gpsData.Latitude = 45.384365;
		gpsData.Longitude = -75.698600;
	}else if(mapDisplayCount == 1){
		gpsData.Latitude = 45.424616;
		gpsData.Longitude = -75.699481;
	}else if(mapDisplayCount == 2){
		gpsData.Latitude = 45.416566;
		gpsData.Longitude = -75.717012;
	}else if(mapDisplayCount == 3){
		gpsData.Latitude = 45.398792;
		gpsData.Longitude = -75.685871;
	}
	mapDisplayCount++;
	if(mapDisplayCount == 4){
		mapDisplayCount = 0;
		zoom_level++;
		if(zoom_level == 17){
			zoom_level = 12;
		}
	}
#endif
	if(!gpsData.Validity){
			//
	}else{
			tilex = long2tilex(gpsData.Longitude, zoom_level, &tilexOffset);
			tiley = lat2tiley(gpsData.Latitude, zoom_level, &tileyOffset);
			TRACE("Zoom=%d,TileX=%d,TileY=%d\n", zoom_level, tilex, tiley);
			if((tilex != oldtilex) && (tiley != oldtiley)){
				drawTilePixmap(tilex, tiley, tilexOffset, tileyOffset);
				oldtilex=tilex;
				oldtiley=tiley;
				oldtilexOffset=tilexOffset;
				oldtileyOffset=tileyOffset;
			}
	}
}

void drawTilePixmap(int tilex, int tiley, int tilexOffset, int tileyOffset)
{
	osStatus status;
	status  = osMutexWait(traceMutex, 0);
	if (status != osOK){
		// handle failure code
	}
	
	coord_t leftX = 0;
	coord_t centerX = 247-tilexOffset;
	coord_t rightX = 247-tilexOffset+256;
	coord_t topY = 0;
	coord_t middleY = 240-tileyOffset;
	coord_t bottomY = 240-tileyOffset+256;
	coord_t xTileStartOffset = 256-247+tilexOffset;
	coord_t yTileStartOffset = 256-240+tileyOffset;
	coord_t xShowMap = 256 - xTileStartOffset;
	coord_t yShowMap = 256 - yTileStartOffset;
	TRACE("width = %d,height = %d,leftX = %d,centerX = %d,rightX = %d,topY = %d,middleY = %d,bottomY = %d,xTileStartOffset = %d,yTileStartOffset = %d,xShowMap = %d,yShowMap = %d\n",
	gdispGGetWidth(pixmap), gdispGGetHeight(pixmap), leftX, centerX, rightX, topY, middleY, bottomY, xTileStartOffset, yTileStartOffset, xShowMap, yShowMap);
	
	
	// X11
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex-1, tiley-1);
	if(gdispImageOpenFile(&myImage[0], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[0], leftX, topY, xShowMap, yShowMap, xTileStartOffset, yTileStartOffset);
		gdispImageClose(&myImage[0]);
		//gdispImageCache(&myImage[0]);
	}
	
	// X9
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex-1, tiley);
	if(gdispImageOpenFile(&myImage[1], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[1], leftX, middleY, xShowMap, 256, xTileStartOffset, 0);
		gdispImageClose(&myImage[1]);
		//gdispImageCache(&myImage[1]);
	}

	// X10
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex-1, tiley+1);
	if(gdispImageOpenFile(&myImage[2], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[2], leftX, bottomY, xShowMap, yTileStartOffset, xTileStartOffset, 0);
		gdispImageClose(&myImage[2]);
		//gdispImageCache(&myImage[2]);
	}
	
	// X5
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex, tiley-1);
	if(gdispImageOpenFile(&myImage[3], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[3], centerX, topY, 256, yShowMap, 0, yTileStartOffset);
		gdispImageClose(&myImage[3]);
		//gdispImageCache(&myImage[3]);
	}
	
	// X1
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex, tiley);
	if(gdispImageOpenFile(&myImage[4], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[4], centerX, middleY, 256, 256, 0, 0);
		gdispImageClose(&myImage[4]);
		//gdispImageCache(&myImage[4]);
	}
	
	// X2
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex, tiley+1);
	if(gdispImageOpenFile(&myImage[5], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[5], centerX, bottomY, 256, yTileStartOffset, 0, 0);
		gdispImageClose(&myImage[5]);
		//gdispImageCache(&myImage[5]);
	}
	
	// X7
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex+1, tiley-1);
	if(gdispImageOpenFile(&myImage[6], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[6], rightX, topY, xTileStartOffset, yShowMap, 0, yTileStartOffset);
		gdispImageClose(&myImage[6]);
		//gdispImageCache(&myImage[6]);
	}
	
	// X3
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex+1, tiley);
	if(gdispImageOpenFile(&myImage[7], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[7], rightX, middleY, xTileStartOffset, gdispGetHeight(), 0, 0);
		gdispImageClose(&myImage[7]);
		//gdispImageCache(&myImage[7]);
	}
	
	// X4
	formatString(gpsOutput, sizeof(gpsOutput), "Tiles/%d/%d/%d.png", zoom_level, tilex+1, tiley+1);
	if(gdispImageOpenFile(&myImage[8], gpsOutput) == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &myImage[8], rightX, bottomY, xTileStartOffset, yTileStartOffset, 0, 0);
		gdispImageClose(&myImage[8]);
		//gdispImageCache(&myImage[8]);
	}
	
	if(gdispImageOpenFile(&marker, "Tiles/marker32.png") == GDISP_IMAGE_ERR_OK){
		gdispGImageDraw(pixmap, &marker, 247-16, 240-32, gdispGetWidth(), gdispGetHeight(), 0, 0);
		gdispImageCache(&marker);
		//gdispImageClose(&marker);
	}
	
	surface = gdispPixmapGetBits(pixmap);
	if(surface != NULL){
		gdispBlitAreaEx(305,0,494,480,0,0,494,surface);
	}
	
	status = osMutexRelease(traceMutex);
	if (status != osOK)  {
		// handle failure code
	}
}