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

void nrfScan();
void nrfConnect(uint8_t device);
void nrfForget(uint8_t device);
bool nrfGetAdvertisingCount();
void nrfGetMacAddress();

uint8_t getSpeed();
uint8_t getCadence();
uint8_t getDistance();
uint8_t getHeartRate();
uint8_t getCadenceSetPoint();
uint8_t getBattery();

void getBluetooth();

void sendResponseMSG(uint8_t msg_ID, uint8_t value);
void sendGearSettingsMSG();
void sendBluetoothScanMSG();

osMessageQDef(spiQueue, 32, message_t);
osMessageQId  spiQueue;

void runSPI(){
	nrfSetup();
	uint8_t batt = 0;
	uint8_t count = 0;
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
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_AVAILABILITY_MSG\n");
#endif
				TRACE("SPI:,GET_AVAILABILITY_MSG\n");
				nrfGetAvailability();
			}else if(messageReceived->msg_ID == GET_SPEED_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_SPEED_MSG\n");
#endif
				TRACE("SPI:,GET_SPEED_MSG\n");
				sendResponseMSG(GET_SPEED_MSG, getSpeed());
			}else if(messageReceived->msg_ID == GET_CADENCE_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_CADENCE_MSG\n");
#endif
				TRACE("SPI:,GET_CADENCE_MSG\n");
				sendResponseMSG(GET_CADENCE_MSG, getCadence());
			}else if(messageReceived->msg_ID == GET_DISTANCE_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_DISTANCE_MSG\n");
#endif
				TRACE("SPI:,GET_DISTANCE_MSG\n");
				sendResponseMSG(GET_DISTANCE_MSG, getDistance());
			}else if(messageReceived->msg_ID == GET_HEARTRATE_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_HEARTRATE_MSG\n");
#endif
				TRACE("SPI:,GET_HEARTRATE_MSG\n");
				sendResponseMSG(GET_HEARTRATE_MSG, getHeartRate());
			}else if(messageReceived->msg_ID == GET_CADENCE_SETPOINT_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_CADENCE_SETPOINT_MSG\n");
#endif
				TRACE("SPI:,GET_CADENCE_SETPOINT_MSG\n");
				sendResponseMSG(GET_CADENCE_SETPOINT_MSG, getCadenceSetPoint());
			}else if(messageReceived->msg_ID == GET_BATTERY_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_BATTERY_MSG\n");
#endif
				TRACE("SPI:,GET_BATTERY_MSG\n");
				sendResponseMSG(GET_BATTERY_MSG, getBattery());
			}else if(messageReceived->msg_ID == GET_GEAR_COUNT_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,GET_GEAR_COUNT_MSG\n");
#endif
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
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,SET_GEAR_COUNT_MSG\n");
#endif
				TRACE("SPI:,SET_GEAR_COUNT_MSG\n");
				for(int count = 0; count <= messageReceived->frontGears[0]; count++){
					spi_Data.gears.frontGears[count] = messageReceived->frontGears[count];
				}
				for(int count = 0; count <= messageReceived->backGears[0]; count++){
					spi_Data.gears.backGears[count] = messageReceived->backGears[count];
				}
				nrfSetGearSettings();
			}else if(messageReceived->msg_ID == NRF_SCAN_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,NRF_SCAN_MSG\n");
#endif
				TRACE("SPI:,NRF_SCAN_MSG\n");
				getBluetooth();
			}else if(messageReceived->msg_ID == NRF_CONNECT_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,NRF_CONNECT_MSG\n");
#endif
				TRACE("SPI:,NRF_CONNECT_MSG\n");
				nrfConnect(messageReceived->value);
			}else if(messageReceived->msg_ID == NRF_FORGET_MSG){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,NRF_FORGET_MSG\n");
#endif
				TRACE("SPI:,NRF_FORGET_MSG\n");
				nrfForget(messageReceived->value);
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
	for(uint8_t count = 0; count <= spi_Data.gears.frontGears[0]; count++){
		messageSent->frontGears[count] = spi_Data.gears.frontGears[count];
	}
	for(uint8_t count = 0; count <= spi_Data.gears.backGears[0]; count++){
		messageSent->backGears[count] = spi_Data.gears.backGears[count];
	}
	osMessagePut(guiQueue, (uint32_t)messageSent, 0);
}

void sendBluetoothScanMSG(){
	message_t *messageSent;
	messageSent = (message_t*)osPoolAlloc(mpool);
	messageSent->msg_ID = NRF_SCAN_MSG;
	messageSent->value = spi_Data.bluetooth.deviceCount;
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
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,buffOut = %d [%02hhX],buffIn = %d [%02hhX]\n", *buffOut, *buffOut, *buffIn, *buffIn);
				TM_USART_Puts(USART3, buff);
#endif
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
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,Speed Data is available\n");
#endif
		TRACE("SPI:,Speed Data is available\n");
		nrfGetSpeed();
	}
	if((spi_Data.avail.value[0] & FLAG_CADENCE) != 0){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,Cadence Data is available\n");
#endif
		TRACE("SPI:,Cadence Data is available\n");
		nrfGetCadence();
	}
	if((spi_Data.avail.value[0] & FLAG_DISTANCE) != 0){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,Distance Data is available\n");
#endif
		TRACE("SPI:,Distance Data is available\n");
		nrfGetDistance();
	}
	if((spi_Data.avail.value[0] & FLAG_HEARTRATE) != 0){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,Heart Rate Data is available\n");
#endif
		TRACE("SPI:,Heart Rate Data is available\n");
		nrfGetHeartRate();
	}
	if((spi_Data.avail.value[0] & FLAG_BATTERY) != 0){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,Battery Data is available\n");
#endif
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
		for(uint8_t x = 0; x<3; x++){
			array[x] = value[x];
		}
		array[3] = '\0';
		TRACE("SPI:,Connection to %c%c%c established\n", value[0],value[1],value[2]);
		TRACE("SPI:,Connection to %s finished\n", array[0]);
	}else{
		connectionStatus = false;
		TRACE("SPI:,Connection not established\n");
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

bool nrfGetAdvertisingCount(){
	uint8_t key = GET_ADVERTISING_COUNT_MSG;
	nrfSend(&key, 1);
	uint8_t value;
	nrfReceive(&value, 1);
	if((value == INVALID_DATA) || (value > MAXIMUM_BLUETOOTH)){
		return false;
	}
	spi_Data.bluetooth.deviceCount = value;
	getRTC(&RTCD_SPI, TM_RTC_Format_BIN);
	spi_Data.bluetooth.age = TM_RTC_GetUnixTimeStamp(&RTCD_SPI);
	return true;
}

void nrfGetMacAddress(){
	uint8_t command[3];
	command[0] = GET_MAC_ADDRESS_MSG;
	for(uint8_t count = 0; count < spi_Data.bluetooth.deviceCount; count++){
		command[1] = count;
		nrfSend(&command[0], 2);
		nrfReceive(&devicesMAC[count][0], 6);
		TRACE("SPI:,Device %d,MAC Address; %02X:%02X:%02X:%02X:%02X:%02X\n", count, devicesMAC[count][0], 
																																		devicesMAC[count][1], 
																																		devicesMAC[count][2], 
																																		devicesMAC[count][3], 
																																		devicesMAC[count][4], 
																																		devicesMAC[count][5]);
	}
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
	for(uint8_t count = 1; count <= spi_Data.gears.frontGears[0]; count++){
		command[2] = count-1;
		command[3] = spi_Data.gears.frontGears[count];
		nrfSend(&command[0], 4);
	}
	command[1] = GEAR_COMMAND_BACK;
	for(uint8_t count = 1; count <= spi_Data.gears.backGears[0]; count++){
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

void nrfScan(){
	uint8_t command[2];
	command[0] = NRF_SCAN_MSG;
	command[1] = NRF_SCAN_PERIOD;
	nrfSend(&command[0], 2);
}

void nrfConnect(uint8_t device){
	uint8_t command[2];
	command[0] = NRF_CONNECT_MSG;
	command[1] = device;
	nrfSend(&command[0], 2);
}

void nrfForget(uint8_t device){
	uint8_t command[2];
	command[0] = NRF_FORGET_MSG;
	command[1] = device;
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
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,INVALID SPEED: Returning INVALID_DATA\n");
#endif
		TRACE("SPI:,INVALID SPEED: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,VALID SPEED: %d\n", spi_Data.speed.value);
				TM_USART_Puts(USART3, buff);
#endif
	TRACE("SPI:,VALID SPEED: %d\n", spi_Data.speed.value);
	return spi_Data.speed.value;
}

uint8_t getCadence(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.cadence.age;
	if(((!connectionStatus) || (!nrfGetCadence())) && (timeDiff > DATA_INVALID_TIME)){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,INVALID CADENCE: Returning INVALID_DATA\n");
#endif
		TRACE("SPI:,INVALID CADENCE: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,VALID CADENCE: %d\n", spi_Data.cadence.value);
				TM_USART_Puts(USART3, buff);
#endif
	TRACE("SPI:,VALID CADENCE: %d\n", spi_Data.cadence.value);
	return spi_Data.cadence.value;
}

uint8_t getDistance(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.distance.age;
	if(((!connectionStatus) || (!nrfGetDistance())) && (timeDiff > DATA_INVALID_TIME)){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,INVALID DISTANCE: Returning INVALID_DATA\n");
#endif
		TRACE("SPI:,INVALID DISTANCE: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,VALID DISTANCE: %d\n", spi_Data.distance.value);
				TM_USART_Puts(USART3, buff);
#endif
	TRACE("SPI:,VALID DISTANCE: %d\n", spi_Data.distance.value);
	return spi_Data.distance.value;
}

uint8_t getHeartRate(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.heartRate.age;
	if(((!connectionStatus) || (!nrfGetHeartRate())) && (timeDiff > DATA_INVALID_TIME)){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,INVALID HEART RATE: Returning INVALID_DATA\n");
#endif
		TRACE("SPI:,INVALID HEART RATE: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,VALID HEART RATE: %d\n", spi_Data.heartRate.value);
				TM_USART_Puts(USART3, buff);
#endif
	TRACE("SPI:,VALID HEART RATE: %d\n", spi_Data.heartRate.value);
	return spi_Data.heartRate.value;
}

uint8_t getCadenceSetPoint(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.cadenceSetPoint.age;
	if(((!connectionStatus) || (!nrfGetCadenceSetPoint())) && (timeDiff > DATA_INVALID_TIME)){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,INVALID CADENCE SET POINT: Returning INVALID_DATA\n");
#endif
		TRACE("SPI:,INVALID CADENCE SET POINT: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,VALID CADENCE SET POINT: %d\n", spi_Data.cadenceSetPoint.value);
				TM_USART_Puts(USART3, buff);
#endif
	TRACE("SPI:,VALID CADENCE SET POINT: %d\n", spi_Data.cadenceSetPoint.value);
	return spi_Data.cadenceSetPoint.value;
}

uint8_t getBattery(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.batt.age;
	if(((!connectionStatus) || (!nrfGetBattery())) && (timeDiff > DATA_INVALID_TIME)){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,INVALID BATTERY: Returning INVALID_DATA\n");
#endif
		TRACE("SPI:,INVALID BATTERY: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,VALID BATTERY: %d\n", spi_Data.batt.value);
				TM_USART_Puts(USART3, buff);
#endif
	TRACE("SPI:,VALID BATTERY: %d\n", spi_Data.batt.value);
	if(spi_Data.batt.value > 95){
		return 100;
	}else if(spi_Data.batt.value > 85){
		return 90;
	}else if(spi_Data.batt.value > 75){
		return 80;
	}else if(spi_Data.batt.value > 65){
		return 70;
	}else if(spi_Data.batt.value > 55){
		return 60;
	}else if(spi_Data.batt.value > 45){
		return 50;
	}else if(spi_Data.batt.value > 35){
		return 40;
	}else if(spi_Data.batt.value > 25){
		return 30;
	}else if(spi_Data.batt.value > 15){
		return 20;
	}else if(spi_Data.batt.value > 5){
		return 10;
	}else{
		return 0;
	}
}

uint8_t getWheelDiameter(){
	TM_RTC_t rtcd;
	getRTC(&rtcd, TM_RTC_Format_BIN);
	uint32_t timeDiff = TM_RTC_GetUnixTimeStamp(&rtcd) - spi_Data.wheelDiameter.age;
	if(((!connectionStatus) || (!nrfGetWheelDiameter())) && (timeDiff > DATA_INVALID_TIME)){
#ifdef DEBUG
				TM_USART_Puts(USART3, "SPI:,INVALID WHEEL DIAMETER: Returning INVALID_DATA\n");
#endif
		TRACE("SPI:,INVALID WHEEL DIAMETER: Returning INVALID_DATA\n");
		return INVALID_DATA;
	}
#ifdef DEBUG
				char buff[256];
				formatString(buff, sizeof(buff), "SPI:,VALID WHEEL DIAMETER: %d\n", spi_Data.wheelDiameter.value);
				TM_USART_Puts(USART3, buff);
#endif
	TRACE("SPI:,VALID WHEEL DIAMETER: %d\n", spi_Data.wheelDiameter.value);
	return spi_Data.wheelDiameter.value;
}

void getBluetooth(){
	nrfScan();
	Delayms(1000*NRF_SCAN_PERIOD);
	if(nrfGetAdvertisingCount()){
		nrfGetMacAddress();
		sendBluetoothScanMSG();
	}
}