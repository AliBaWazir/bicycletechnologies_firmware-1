#include "tm_stm32_spi.h"
#include "spi.h"
#include "trace.h"
#include "tm_stm32_delay.h"
#include "msg.h"

#include "tm_stm32_exti.h"

bool connectionStatus;

struct SPI_data spi_Data;
char spiOutput[70];

TM_RTC_t RTCD_SPI;

bool nrfSend(uint8_t *buffOut, uint32_t len);
bool nrfReceive(uint8_t *buffIn, uint32_t len);
bool nrfTransmit(uint8_t *buffOut, uint8_t *buffIn, uint32_t len);

void nrfGetAvailability();
void nrfGetDeviceName();
bool nrfGetSpeed();
bool nrfGetCadence();
bool nrfGetDistance();
bool nrfGetHeartRate();
bool nrfGetCadenceSetPoint();
bool nrfGetBattery();
bool nrfGetWheelDiameter();
void nrfGetGearSettings();

void nrfSetGearSettings();
void nrfSetWheelDiameter();
void nrfSetCadenceSetPoint();

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
	
	connectionStatus = true;
	
	spiQueue = osMessageCreate(osMessageQ(spiQueue), NULL);
	
	char temp[10];
	message_t *messageReceived;
	while(1){
		osEvent evt = osMessageGet(spiQueue, osWaitForever);
		if (evt.status == osEventMessage) {
			//nrfGetDeviceName();
			messageReceived = (message_t*)evt.value.p;
			if(messageReceived->msg_ID == GET_AVAILABILITY_MSG){
				TRACE("SPI:,GET_AVAILABILITY_MSG\n");
				nrfGetAvailability();
			}else if(messageReceived->msg_ID == GET_SPEED_MSG){
				TRACE("SPI:,GET_SPEED_MSG\n");
				sendResponseMSG(GET_SPEED_MSG, getSpeed());
			}else if(messageReceived->msg_ID == GET_CADENCE_MSG){
				TRACE("SPI:,GET_CADENCE_MSG\n");
				sendResponseMSG(GET_CADENCE_MSG, getCadence());
			}else if(messageReceived->msg_ID == GET_DISTANCE_MSG){
				TRACE("SPI:,GET_DISTANCE_MSG\n");
				sendResponseMSG(GET_DISTANCE_MSG, getDistance());
			}else if(messageReceived->msg_ID == GET_HEARTRATE_MSG){
				TRACE("SPI:,GET_HEARTRATE_MSG\n");
				sendResponseMSG(GET_HEARTRATE_MSG, getHeartRate());
			}else if(messageReceived->msg_ID == GET_CADENCE_SETPOINT_MSG){
				TRACE("SPI:,GET_CADENCE_SETPOINT_MSG\n");
				sendResponseMSG(GET_CADENCE_SETPOINT_MSG, getCadenceSetPoint());
			}else if(messageReceived->msg_ID == GET_BATTERY_MSG){
				TRACE("SPI:,GET_BATTERY_MSG\n");
				sendResponseMSG(GET_BATTERY_MSG, getBattery());
			}else if(messageReceived->msg_ID == GET_GEAR_COUNT_MSG){
				TRACE("SPI:,GET_GEAR_COUNT_MSG\n");
				for(int count = 0; count <= MAXIMUM_FRONT_GEARS; count++){
					spi_Data.gears.frontGears[count] = 0;
				}
				for(int count = 0; count <= MAXIMUM_BACK_GEARS; count++){
					spi_Data.gears.backGears[count] = 0;
				}
				nrfGetGearSettings();
				sendGearSettingsMSG();
			}else if(messageReceived->msg_ID == SET_GEAR_COUNT_MSG){
				TRACE("SPI:,SET_GEAR_COUNT_MSG\n");
				for(int count = 0; count <= messageReceived->frontGears[0]; count++){
					spi_Data.gears.frontGears[count] = messageReceived->frontGears[count];
				}
				for(int count = 0; count <= messageReceived->backGears[0]; count++){
					spi_Data.gears.backGears[count] = messageReceived->backGears[count];
				}
				nrfSetGearSettings();
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
		TRACE("SPI:,INTERRUPTED\n");
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

bool nrfSend(uint8_t *buffOut, uint32_t len){
	uint8_t buffIn;
	nrfTransmit(buffOut, &buffIn, len);
}

bool nrfReceive(uint8_t *buffIn, uint32_t len){
	uint8_t buffOut = DUMMY_VALUE;
	nrfTransmit(&buffOut, buffIn, len);
}

bool nrfTransmit(uint8_t *buffOut, uint8_t *buffIn, uint32_t len){
	while(1){
		TM_GPIO_SetPinLow(GPIOH, GPIO_PIN_6);
		TM_SPI_SendMulti(SPI2, buffOut, buffIn, len);
		TRACE("SPI:,buffOut = %d [%02hhX],buffIn = %d [%02hhX]\n", *buffOut, *buffOut, *buffIn, *buffIn);
		if(*buffIn == DUMMY_VALUE){
			TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
			Delayms(50);
		}else{
			break;
		}
	}
	TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
	return true;
}
	
/*
* ======================== GETTERS ========================
*/

void nrfGetAvailability(){
	uint8_t key = GET_AVAILABILITY_MSG;
	nrfSend(&key, 1);
	nrfReceive(&spi_Data.avail.value[0], 4);
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.avail.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	if((spi_Data.avail.value[0] & FLAG_SPEED) != 0){
		TRACE("SPI:,Speed Data is available\n");
		nrfGetSpeed();
	}
	if((spi_Data.avail.value[0] & FLAG_CADENCE) != 0){
		TRACE("SPI:,Cadence Data is available\n");
		nrfGetCadence();
	}
	if((spi_Data.avail.value[0] & FLAG_DISTANCE) != 0){
		TRACE("SPI:,Distance Data is available\n");
		nrfGetDistance();
	}
	if((spi_Data.avail.value[0] & FLAG_HEARTRATE) != 0){
		TRACE("SPI:,Heart Rate Data is available\n");
		nrfGetHeartRate();
	}
	if((spi_Data.avail.value[0] & FLAG_BATTERY) != 0){
		TRACE("SPI:,Battery Data is available\n");
		nrfGetBattery();
	}
}

// TODO UNDER DEVELOPMENT
void nrfGetDeviceName(){
	char array[4];
	uint8_t key = GET_DEVICE_NAME_MSG;
	nrfSend(&key, 1);
	uint8_t value[3];
	nrfReceive(&value[0], 3);
	if((value[0] == 0x4E) && (value[1] == 0x52) && (value[2] == 0x46)){
		connectionStatus = true;
		for(int x = 0; x<3; x++){
			array[x] = value[x];
		}
		array[3] = '\0';
		TRACE("SPI:,Connection to %c%c%c established\n", value[0],value[1],value[2]);
		TRACE("SPI:,Connection to %s finished\n", array[0]);
	}else{
		connectionStatus = false;
		TRACE("SPI:,Connection not established");
	}
	
}

bool nrfGetSpeed(){
	uint8_t key = GET_SPEED_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_SPEED)){
		return false;
	}
	spi_Data.speed.value = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.speed.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

bool nrfGetCadence(){
	uint8_t key = GET_CADENCE_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_CADENCE)){
		return false;
	}
	spi_Data.cadence.value = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.cadence.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

bool nrfGetDistance(){
	uint8_t key = GET_DISTANCE_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_DISTANCE)){
		return false;
	}
	spi_Data.distance.value = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.distance.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

bool nrfGetHeartRate(){
	uint8_t key = GET_HEARTRATE_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_HEART_RATE)){
		return false;
	}
	spi_Data.heartRate.value = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.heartRate.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

bool nrfGetCadenceSetPoint(){
	uint8_t key = GET_CADENCE_SETPOINT_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_CADENCE_SET_POINT)){
		return false;
	}
	spi_Data.cadenceSetPoint.value = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.cadenceSetPoint.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

bool nrfGetBattery(){
	uint8_t key = GET_BATTERY_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_BATTERY)){
		return false;
	}
	spi_Data.batt.value = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.batt.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

bool nrfGetWheelDiameter(){
	uint8_t key = GET_WHEEL_DIAMETER_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_WHEEL_DIAMETER)){
		return false;
	}
	spi_Data.wheelDiameter.value = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.wheelDiameter.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

void nrfGetGearSettings(){
	uint8_t key = GET_GEAR_COUNT_MSG;
	uint8_t temp[2];
	nrfSend(&key, 1);
	nrfReceive(&temp[0], 2);
	if((temp[0] > MAXIMUM_FRONT_GEARS) || (temp[1] > MAXIMUM_BACK_GEARS)){
		return;
	}
	spi_Data.gears.frontGears[0] = temp[0];
	spi_Data.gears.backGears[0] = temp[1];
	TRACE("SPI:,Gear Count Received, Front = %d, Back = %d\n", temp[0], temp[1]);
	
	uint8_t command[3];
	command[0] = GET_TEETH_COUNT_MSG;
	// Front Gears
	command[1] = GEAR_COMMAND_FRONT;
	for(int count = 1; count <= spi_Data.gears.frontGears[0]; count++){
		command[2] = count-1;
		nrfSend(&command[0], 3);
		nrfReceive(&temp[0], 1);
		spi_Data.gears.frontGears[count] = temp[0];
		TRACE("SPI:,Front Gear = %d,Teeth Count = %d\n", count, temp[0]);
	}
	
	// Back Gears
	command[1] = GEAR_COMMAND_BACK;
	for(int count = 1; count <= spi_Data.gears.backGears[0]; count++){
		command[2] = count-1;
		nrfSend(&command[0], 3);
		nrfReceive(&temp[0], 1);
		spi_Data.gears.backGears[count] = temp[0];
		TRACE("SPI:,Back Gear = %d,Teeth Count = %d\n", count, temp[0]);
	}
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.gears.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
}


/*
* ======================== SETTERS ========================
*/

void nrfSetGearSettings(){
	uint8_t command[4];
	command[0] = SET_GEAR_COUNT_MSG;
	command[1] = spi_Data.gears.frontGears[0];
	command[2] = spi_Data.gears.backGears[0];
	nrfSend(&command[0], 3);
	
	command[0] = SET_TEETH_COUNT_MSG;
	command[1] = GEAR_COMMAND_FRONT;
	for(int count = 1; count <= spi_Data.gears.frontGears[0]; count++){
		command[2] = count-1;
		command[3] = spi_Data.gears.frontGears[count];
		nrfSend(&command[0], 4);
	}
	command[1] = GEAR_COMMAND_BACK;
	for(int count = 1; count <= spi_Data.gears.backGears[0]; count++){
		command[2] = count-1;
		command[3] = spi_Data.gears.backGears[count];
		nrfSend(&command[0], 4);
	}
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.gears.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
}

void nrfSetWheelDiameter(){
	uint8_t command[2];
	command[0] = SET_WHEEL_DIAMETER_MSG;
	command[1] = spi_Data.wheelDiameter.value;
	nrfSend(&command[0], 2);
}

void nrfSetCadenceSetPoint(){
	uint8_t command[2];
	command[0] = SET_CADENCE_SETPOINT_MSG;
	command[1] = spi_Data.cadenceSetPoint.value;
	nrfSend(&command[0], 2);
}

/*
* ======================== OTHERS ========================
*/

uint8_t getSpeed(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.speed.age;
  if(((!connectionStatus) || (!nrfGetSpeed())) && (timeDiff > DATA_INVALID_TIME)){
		TRACE("SPI:,INVALID SPEED: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
	TRACE("SPI:,VALID SPEED: %d\n", spi_Data.speed.value);
	return spi_Data.speed.value;
}

uint8_t getCadence(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.cadence.age;
	if(((!connectionStatus) || (!nrfGetCadence())) && (timeDiff > DATA_INVALID_TIME)){
		TRACE("SPI:,INVALID CADENCE: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
	TRACE("SPI:,VALID CADENCE: %d\n", spi_Data.cadence.value);
	return spi_Data.cadence.value;
}

uint8_t getDistance(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.distance.age;
	if(((!connectionStatus) || (!nrfGetDistance())) && (timeDiff > DATA_INVALID_TIME)){
		TRACE("SPI:,INVALID DISTANCE: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
	TRACE("SPI:,VALID DISTANCE: %d\n", spi_Data.distance.value);
	return spi_Data.distance.value;
}

uint8_t getHeartRate(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.heartRate.age;
	if(((!connectionStatus) || (!nrfGetHeartRate())) && (timeDiff > DATA_INVALID_TIME)){
		TRACE("SPI:,INVALID HEART RATE: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
	TRACE("SPI:,VALID HEART RATE: %d\n", spi_Data.heartRate.value);
	return spi_Data.heartRate.value;
}

uint8_t getCadenceSetPoint(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.cadenceSetPoint.age;
	if(((!connectionStatus) || (!nrfGetCadenceSetPoint())) && (timeDiff > DATA_INVALID_TIME)){
		TRACE("SPI:,INVALID CADENCE SET POINT: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
	TRACE("SPI:,VALID CADENCE SET POINT: %d\n", spi_Data.cadenceSetPoint.value);
	return spi_Data.cadenceSetPoint.value;
}

uint8_t getBattery(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.batt.age;
	if(((!connectionStatus) || (!nrfGetBattery())) && (timeDiff > DATA_INVALID_TIME)){
		TRACE("SPI:,INVALID BATTERY: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
	TRACE("SPI:,VALID BATTERY: %d\n", spi_Data.batt.value);
	return spi_Data.batt.value;
}

uint8_t getWheelDiameter(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.wheelDiameter.age;
	if(((!connectionStatus) || (!nrfGetWheelDiameter())) && (timeDiff > DATA_INVALID_TIME)){
		TRACE("SPI:,INVALID WHEEL DIAMETER: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
	TRACE("SPI:,VALID WHEEL DIAMETER: %d\n", spi_Data.wheelDiameter.value);
	return spi_Data.wheelDiameter.value;
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