#include "tm_stm32_spi.h"
#include "spi.h"
#include "trace.h"
#include "tm_stm32_delay.h"
#include "msg.h"

#include "tm_stm32_exti.h"

struct SPI_data spi_Data;
char spiOutput[70];

TM_RTC_t RTCD_SPI;

bool nrfRequest(uint8_t *buffOut, uint32_t len);
bool nrfReceive(uint8_t *buffIn, uint32_t len);
bool nrfTransmit(uint8_t *buffOut, uint8_t *buffIn, uint32_t len);

void nrfGetAvailability();
void nrfGetSpeed();
void nrfGetCadence();
void nrfGetDistance();
void nrfGetHeartRate();
void nrfGetCadenceSetPoint();
void nrfGetBattery();
void nrfGetGearSettings();
void nrfSetGearSettings();

uint8_t getSpeed();
uint8_t getCadence();
uint8_t getDistance();
uint8_t getHeartRate();
uint8_t getCadenceSetPoint();
uint8_t getBattery();

void sendResponseMSG(uint8_t msg_ID, uint8_t value);
void sendGearSettingsMSG();

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
			if(messageReceived->msg_ID == GET_AVAILABILITY_MSG){
				TRACE("SPI: GET_AVAILABILITY_MSG\n");
				nrfGetAvailability();
			}else if(messageReceived->msg_ID == GET_SPEED_MSG){
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
			}else if(messageReceived->msg_ID == GET_CADENCE_SETPOINT_MSG){
				TRACE("SPI: GET_CADENCE_SETPOINT_MSG\n");
				sendResponseMSG(GET_CADENCE_SETPOINT_MSG, getCadenceSetPoint());
			}else if(messageReceived->msg_ID == GET_BATTERY_MSG){
				TRACE("SPI: GET_BATTERY_MSG\n");
				sendResponseMSG(GET_BATTERY_MSG, getBattery());
			}else if(messageReceived->msg_ID == GET_GEAR_COUNT_MSG){
				TRACE("SPI: GET_GEAR_COUNT_MSG\n");
				//nrfGetGearSettings();
				/*spi_Data.gears.frontGears[0] = 4;
				spi_Data.gears.backGears[0] = 9;
				for(int x = 1; x <= 4; x++){
					spi_Data.gears.frontGears[x] = x;
				}
				for(int x = 1; x <= 9; x++){
					spi_Data.gears.backGears[x] = x;
				}*/
				sendGearSettingsMSG();
			}else if(messageReceived->msg_ID == SET_GEAR_COUNT_MSG){
				TRACE("SPI: SET_GEAR_COUNT_MSG\n");
				for(int count = 0; count <= messageReceived->frontGears[0]; count++){
					spi_Data.gears.frontGears[count] = messageReceived->frontGears[count];
				}
				for(int count = 0; count <= messageReceived->backGears[0]; count++){
					spi_Data.gears.backGears[count] = messageReceived->backGears[count];
				}
				//nrfSetGearSettings();
			}
			osPoolFree(mpool, messageReceived);
		}
	}
}

void sendResponseMSG(uint8_t msg_ID, uint8_t value){
	message_t *messageSent;
	messageSent = (message_t*)osPoolAlloc(mpool);
	messageSent->msg_ID = msg_ID;
	messageSent->value = value;
	osMessagePut(guiQueue, (uint32_t)messageSent, 0);
}

void sendGearSettingsMSG(){
	message_t *messageSent;
	messageSent = (message_t*)osPoolAlloc(mpool);
	messageSent->msg_ID = GET_GEAR_COUNT_MSG;
	for(int count = 0; count <= spi_Data.gears.frontGears[0]; count++){
		messageSent->frontGears[count] = spi_Data.gears.frontGears[count];
	}
	for(int count = 0; count <= spi_Data.gears.backGears[0]; count++){
		messageSent->backGears[count] = spi_Data.gears.backGears[count];
	}
	osMessagePut(guiQueue, (uint32_t)messageSent, 0);
}

void nrfSetup(){
	// Init Chip Select Pin
	TM_GPIO_Init(GPIOH, GPIO_PIN_6, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_Low);
	//Set high (active low)
	TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);

	uint32_t time;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	time = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	spi_Data.avail.age = time;
	spi_Data.speed.age = time;
	spi_Data.cadence.age = time;
	spi_Data.distance.age = time;
	spi_Data.heartRate.age = time;
	spi_Data.cadenceSetPoint.age = time;
	spi_Data.batt.age = time;
	
	/* Init SPI */
	TM_SPI_Init(SPI2, TM_SPI_PinsPack_Custom);
	
	// Enabling Interrupts from NRF
	if (TM_EXTI_Attach(GPIOA, GPIO_Pin_7, TM_EXTI_Trigger_Rising) == TM_EXTI_Result_Ok) {
		TRACE("NRF Interrupts Are Enabled\n");
	}
}

void TM_EXTI_Handler(uint16_t GPIO_Pin) {
	/* Handle external line 7 interrupts */
	if (GPIO_Pin == GPIO_Pin_7) {
		TRACE("SPI INTERRUPT\n");
		message_t *messageSent;
		messageSent = (message_t*)osPoolAlloc(mpool);
		messageSent->msg_ID = GET_AVAILABILITY_MSG;
		osMessagePut(spiQueue, (uint32_t)messageSent, 0);
	}
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
	uint8_t key = GET_AVAILABILITY_MSG;
	nrfRequest(&key, 1);
	nrfReceive(&spi_Data.avail.value[0], 4);
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.avail.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	if((spi_Data.avail.value[0] & FLAG_SPEED) != 0){
		nrfGetSpeed();
	}
	if((spi_Data.avail.value[0] & FLAG_CADENCE) != 0){
		nrfGetCadence();
	}
	if((spi_Data.avail.value[0] & FLAG_DISTANCE) != 0){
		nrfGetDistance();
	}
	if((spi_Data.avail.value[0] & FLAG_HEARTRATE) != 0){
		nrfGetHeartRate();
	}
	if((spi_Data.avail.value[0] & FLAG_BATTERY) != 0){
		nrfGetBattery();
	}
}

void nrfGetSpeed(){
	uint8_t key = GET_SPEED_MSG;
	nrfRequest(&key, 1);
	nrfReceive(&spi_Data.speed.value, 1);
	spi_Data.speed.age = spi_Data.avail.age;
}

void nrfGetCadence(){
	uint8_t key = GET_CADENCE_MSG;
	nrfRequest(&key, 1);
	nrfReceive(&spi_Data.cadence.value, 1);
	spi_Data.cadence.age = spi_Data.avail.age;
}

void nrfGetDistance(){
	uint8_t key = GET_DISTANCE_MSG;
	nrfRequest(&key, 1);
	nrfReceive(&spi_Data.distance.value, 1);
	spi_Data.distance.age = spi_Data.avail.age;
}

void nrfGetHeartRate(){
	uint8_t key = GET_HEARTRATE_MSG;
	nrfRequest(&key, 1);
	nrfReceive(&spi_Data.heartRate.value, 1);
	spi_Data.heartRate.age = spi_Data.avail.age;
}

void nrfGetCadenceSetPoint(){
	uint8_t key = GET_CADENCE_SETPOINT_MSG;
	nrfRequest(&key, 1);
	nrfReceive(&spi_Data.cadenceSetPoint.value, 1);
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.batt.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
}

void nrfGetBattery(){
	uint8_t key = GET_BATTERY_MSG;
	nrfRequest(&key, 1);
	nrfReceive(&spi_Data.batt.value, 1);
	spi_Data.batt.age = spi_Data.avail.age;
}


void nrfGetGearSettings(){
	uint8_t key = GET_GEAR_COUNT_MSG;
	uint8_t temp[2];
	nrfRequest(&key, 1);
	nrfReceive(&temp[0], 2);
	if((temp[0] > MAXIMUM_FRONT_GEARS) || (temp[1] > MAXIMUM_BACK_GEARS)){
		return;
	}
	spi_Data.gears.frontGears[0] = temp[0];
	spi_Data.gears.backGears[0] = temp[1];
	
	uint8_t command[3];
	command[0] = GET_TEETH_COUNT_MSG;
	// Front Gears
	command[1] = 0xCA;
	for(int count = 1; count <= spi_Data.gears.frontGears[0]; count++){
		command[2] = count-1;
		nrfRequest(&command[0], 3);
		nrfReceive(&temp[0], 1);
		spi_Data.gears.frontGears[count] = temp[0];
	}
	
	// Back Gears
	command[1] = 0xEE;
	for(int count = 1; count <= spi_Data.gears.backGears[0]; count++){
		command[2] = count-1;
		nrfRequest(&command[0], 3);
		nrfReceive(&temp[0], 1);
		spi_Data.gears.backGears[count] = temp[0];
	}
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.gears.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
}

void nrfSetGearSettings(){
	uint8_t command[4];
	command[0] = SET_GEAR_COUNT_MSG;
	command[1] = spi_Data.gears.frontGears[0];
	command[2] = spi_Data.gears.backGears[0];
	nrfRequest(&command[0], 3);
	
	command[0] = SET_TEETH_COUNT_MSG;
	command[1] = 0xCA;
	for(int count = 1; count <= spi_Data.gears.frontGears[0]; count++){
		command[2] = count-1;
		command[3] = spi_Data.gears.frontGears[count];
		nrfRequest(&command[0], 4);
	}
	command[1] = 0xEE;
	for(int count = 1; count <= spi_Data.gears.backGears[0]; count++){
		command[2] = count-1;
		command[3] = spi_Data.gears.backGears[count];
		nrfRequest(&command[0], 4);
	}
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.gears.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
}



uint8_t getSpeed(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&rtcd)- 120) < spi_Data.speed.age){
		return spi_Data.speed.value;
	}else{
		return NULL;
	}
}

uint8_t getCadence(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&rtcd)- 120) < spi_Data.cadence.age){
		return spi_Data.cadence.value;
	}else{
		return NULL;
	}
}

uint8_t getDistance(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&rtcd)- 120) < spi_Data.distance.age){
		return spi_Data.distance.value;
	}else{
		return NULL;
	}
}

uint8_t getHeartRate(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&rtcd)- 120) < spi_Data.heartRate.age){
		return spi_Data.heartRate.value;
	}else{
		return NULL;
	}
}

uint8_t getCadenceSetPoint(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&rtcd)- 120) > spi_Data.cadenceSetPoint.age){
		nrfGetCadenceSetPoint();
	}
	return spi_Data.batt.value;
}

uint8_t getBattery(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	if((TM_RTC_GetUnixTimeStamp(&rtcd)- 120) < spi_Data.batt.age){
		return spi_Data.batt.value;
	}else{
		return NULL;
	}
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