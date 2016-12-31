#include "tm_stm32_spi.h"
#include "spi.h"
#include "trace.h"
#include "tm_stm32_delay.h"
#include "msg.h"

struct SPI_data spi_Data;
char spiOutput[70];

bool nrfRequest(uint8_t *buffOut, uint32_t len);
bool nrfReceive(uint8_t *buffIn, uint32_t len);
bool nrfTransmit(uint8_t *buffOut, uint8_t *buffIn, uint32_t len);

void nrfGetAvailability();
void nrfGetSpeed();
void nrfGetCadence();
void nrfGetDistance();
void nrfGetHeartRate();
void nrfGetBattery();

uint8_t getSpeed();
uint8_t getCadence();
uint8_t getDistance();
uint8_t getHeartRate();
uint8_t getBattery();

void sendResponseMSG(uint8_t msg_ID, uint8_t value);
	
osMessageQDef(spiQueue, 16, message_t);
osMessageQId  spiQueue;

void runSPI(){
	nrfSetup();

	spiQueue = osMessageCreate(osMessageQ(spiQueue), NULL);
	
	char temp[10];
	message_t *messageReceived;
	while(1){
		osEvent evt = osMessageGet(spiQueue, osWaitForever);
		if (evt.status == osEventMessage) {
			messageReceived = (message_t*)evt.value.p;
			if(messageReceived->msg_ID == GET_SPEED_MSG){
				TRACE("SPI: GET_SPEED_MSG\n");
				sendResponseMSG(GET_SPEED_MSG, getSpeed());
			}else if(messageReceived->msg_ID == GET_CADENCE_MSG){
				TRACE("SPI: GET_CADENCE_MSG\n");
				sendResponseMSG(GET_CADENCE_MSG, getCadence());
			}else if(messageReceived->msg_ID == GET_DISTANCE_MSG){
				TRACE("SPI: GET_DISTANCE_MSG\n");
				sendResponseMSG(GET_DISTANCE_MSG, getDistance());
			}else if(messageReceived->msg_ID == GET_HEARTRATE_MSG){
				TRACE("SPI: GET_HEARTRATE_MSG\n");
				sendResponseMSG(GET_HEARTRATE_MSG, getHeartRate());
			}else if(messageReceived->msg_ID == GET_BATTERY_MSG){
				TRACE("SPI: GET_BATTERY_MSG\n");
				//sendResponseMSG(GET_BATTERY_MSG, getBattery());
			}
			osPoolFree(mpool, messageReceived);
		}
	}
}

void sendResponseMSG(uint8_t msg_ID, uint8_t value){
	message_t *messageSent;
	messageSent = (message_t*)osPoolAlloc(mpool);
	messageSent->msg_ID = msg_ID;
	messageSent->speed = value;
	osMessagePut(guiQueue, (uint32_t)messageSent, 0);
}

void nrfSetup(){
	// Init Chip Select Pin
	TM_GPIO_Init(GPIOH, GPIO_PIN_6, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_Low);
	//Set high (active low)
	TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);

	//Initialize Vars
	spi_Data.avail.key = 0xDA;
	spi_Data.speed.key = 0x01;
	spi_Data.cadence.key = 0x02;
	spi_Data.distance.key = 0x03;
	spi_Data.heartRate.key = 0x04;
	spi_Data.batt.key = 0x05;
	
	uint32_t time;
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	time = TM_RTC_GetUnixTimeStamp(&RTCD);
	spi_Data.avail.age = time;
	spi_Data.speed.age = time;
	spi_Data.cadence.age = time;
	spi_Data.distance.age = time;
	spi_Data.heartRate.age = time;
	spi_Data.batt.age = time;
	
	/* Init SPI */
	TM_SPI_Init(SPI2, TM_SPI_PinsPack_Custom);
}

/* Custom SPI initialization */
void TM_SPI_InitCustomPinsCallback(SPI_TypeDef* SPIx, uint16_t AlternateFunction) {
	TM_GPIO_InitAlternate(GPIOB, GPIO_PIN_14 | GPIO_PIN_15, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, AlternateFunction);
	TM_GPIO_InitAlternate(GPIOD, GPIO_PIN_3, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, AlternateFunction);
}

bool nrfRequest(uint8_t *buffOut, uint32_t len){
#ifdef DEBUG
	formatString(spiOutput, sizeof(spiOutput), "nrfRequest: %d\n", *buffOut);
	TM_USART_Puts(USART3, spiOutput);
#endif
	TRACE("nrfRequest: %d\n", *buffOut);
	uint8_t buffIn;
	
	nrfTransmit(buffOut, &buffIn, len);
	
	TRACE("nrfRequest Done\n");
#ifdef DEBUG
	TM_USART_Puts(USART3, "nrfRequest Done\n");
#endif
}

bool nrfReceive(uint8_t *buffIn, uint32_t len){
#ifdef DEBUG
	TM_USART_Puts(USART3, "nrfReceive Start\n");
#endif
	TRACE("nrfReceive Start\n");
	uint8_t buffOut = 0xFF;
	
	nrfTransmit(&buffOut, buffIn, len);
	
	TRACE("nrfReceive: %d\n", *buffIn);
#ifdef DEBUG
	formatString(spiOutput, sizeof(spiOutput), "nrfReceive: %d\n", *buffIn);
	TM_USART_Puts(USART3, spiOutput);
#endif
}

bool nrfTransmit(uint8_t *buffOut, uint8_t *buffIn, uint32_t len){
	while(1){
		TM_GPIO_SetPinLow(GPIOH, GPIO_PIN_6);
		TM_SPI_SendMulti(SPI2, buffOut, buffIn, len);
		if(*buffIn == 0xDF){
			TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
			Delayms(50);
		}else{
			break;
		}
	}
	TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
	return true;
}
	
void nrfGetAvailability(){
	nrfRequest(&spi_Data.avail.key, 1);
	nrfReceive(&spi_Data.avail.value[0], 4);
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	spi_Data.avail.age = TM_RTC_GetUnixTimeStamp(&RTCD);
}

void nrfGetSpeed(){
	nrfRequest(&spi_Data.speed.key, 1);
	nrfReceive(&spi_Data.speed.value, 1);
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	spi_Data.speed.age = TM_RTC_GetUnixTimeStamp(&RTCD);
}

void nrfGetCadence(){
	nrfRequest(&spi_Data.cadence.key, 1);
	nrfReceive(&spi_Data.cadence.value, 1);
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	spi_Data.cadence.age = TM_RTC_GetUnixTimeStamp(&RTCD);
}

void nrfGetDistance(){
	nrfRequest(&spi_Data.distance.key, 1);
	nrfReceive(&spi_Data.distance.value, 1);
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	spi_Data.distance.age = TM_RTC_GetUnixTimeStamp(&RTCD);
}

void nrfGetHeartRate(){
	nrfRequest(&spi_Data.heartRate.key, 1);
	nrfReceive(&spi_Data.heartRate.value, 1);
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	spi_Data.heartRate.age = TM_RTC_GetUnixTimeStamp(&RTCD);
}

void nrfGetBattery(){
	nrfRequest(&spi_Data.batt.key, 1);
	nrfReceive(&spi_Data.batt.value, 1);
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	spi_Data.batt.age = TM_RTC_GetUnixTimeStamp(&RTCD);
}

uint8_t getSpeed(){
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&RTCD)- 120) > spi_Data.speed.age){
		nrfGetSpeed();
	}
	return spi_Data.speed.value;
}

uint8_t getCadence(){
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&RTCD)- 120) > spi_Data.cadence.age){
		nrfGetSpeed();
	}
	return spi_Data.cadence.value;
}

uint8_t getDistance(){
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&RTCD)- 120) > spi_Data.distance.age){
		nrfGetSpeed();
	}
	return spi_Data.distance.value;
}

uint8_t getHeartRate(){
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&RTCD)- 120) > spi_Data.heartRate.age){
		nrfGetSpeed();
	}
	return spi_Data.heartRate.value;
}

uint8_t getBattery(){
	TM_RTC_t RTCD;
	getRTC(&RTCD, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&RTCD)- 120) > spi_Data.batt.age){
		nrfGetSpeed();
	}
	return spi_Data.batt.value;
}

/*

bool nrfGetData(){
		Delayms(100);
		runTestCase(&spi_Data.speed.key);
		Delayms(100);
		runTestCase(&spi_Data.cadence.key);
		Delayms(100);
		runTestCase(&spi_Data.distance.key);
		Delayms(100);
		runTestCase(&spi_Data.heartRate.key);
    return true;
}

void runTestCase(uint8_t* target){
#ifdef DEBUG
		formatString(spiOutput, sizeof(spiOutput), "Starting Test Case With Key: %d\n", *target);
		TM_USART_Puts(USART3, spiOutput);
#endif
		uint8_t testGarbage[4] = {0x00,0x00,0x00,0x00};
    for(uint8_t i = 0; i<4; i++){        
        memset(testGarbage,0x00,sizeof(testGarbage));
        nrfTransmitSingle(target, &testGarbage[0]);// This transfer gives the NRF the instruction
        nrfTransmitSingle(&testGarbage[0], &spi_Data.speed.value);// This transfer allows the NRF to clock the data to us
				//TRACE("Target: %d, Test Garbage: %d,%d,%d,%d, Value: %d\n", *target, testGarbage[0], testGarbage[1], testGarbage[2], testGarbage[3], spi_Data.speed.value);
#ifdef DEBUG
				formatString(spiOutput, sizeof(spiOutput), "Target: %d, Test Garbage: %d,%d,%d,%d, Value: %d\n", *target, testGarbage[0], testGarbage[1], testGarbage[2], testGarbage[3], spi_Data.speed.value);
				TM_USART_Puts(USART3, spiOutput);
#endif
				Delayms(1000);
    }
}

bool nrfTransmitSingle(uint8_t* out, uint8_t* in){
    return nrfTransmit(out, in, 1);
}


bool nrfTransmit(uint8_t *buffOut, uint8_t *buffIn, uint32_t len) {
    while(nrfTransmit2(buffOut, buffIn, len)==false){
        //if we go here not working
			TRACE("SPI NOT WORKING\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "SPI NOT WORKING\n");
#endif
    }
    return true;
}   

bool nrfTransmit2(uint8_t *buffOut, uint8_t *buffIn, uint32_t len) {
    bool transmitSuccess = false;
		Delayms(7);
    TRACE("Transmit NRF\n");
#ifdef DEBUG
		TM_USART_Puts(USART3, "Transmit NRF\n");
#endif
		Delayms(1);
    for(int i = 0; i<10 && (!transmitSuccess); i++){
				TM_GPIO_SetPinLow(GPIOH, GPIO_PIN_6);
        TM_SPI_SendMulti(SPI2, buffOut, buffIn,len);
        if(buffOut[0] == 0xDF){
					TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
          Delayms(1000);
          transmitSuccess= false;
        }else{
					transmitSuccess= true;
				}
    }
		TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
    TRACE("Finished Transmit NRF\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "Finished Transmit NRF\n");
#endif
    return transmitSuccess;
}

*/