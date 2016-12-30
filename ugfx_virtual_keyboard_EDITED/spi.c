#include "tm_stm32_spi.h"
#include "spi.h"
#include "trace.h"
#include "tm_stm32_delay.h"

struct SPI_data spi_Data;
char spiOutput[70];

void nrfSetup(){
	
	TM_GPIO_Init(GPIOH, GPIO_PIN_6, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_Low);
	//Set high (active low)
	TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);

	//Initialize Vars
	spi_Data.speed.key = 0x01;
	spi_Data.cadence.key = 0x02;
	spi_Data.distance.key = 0x03;
	spi_Data.heartRate.key = 0x04;
	spi_Data.batt.key = 0x05;
}

/* Custom SPI initialization */
void TM_SPI_InitCustomPinsCallback(SPI_TypeDef* SPIx, uint16_t AlternateFunction) {
	TRACE("Custom SPI Pinback\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "Custom SPI Pinback\n");
#endif
	TM_GPIO_InitAlternate(GPIOB, GPIO_PIN_14 | GPIO_PIN_15, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, AlternateFunction);
	TM_GPIO_InitAlternate(GPIOD, GPIO_PIN_3, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, AlternateFunction);
}

bool nrfGetData(){
    /*uint8_t command = 0xDA; 
    uint8_t retVal = 0xFF;
    nrfTransmitSingle(&command, &retVal); // send command to retrieve bitfield list
    

    uint8_t bitField[4] = {0x00,0x00,0x00,0x00};
    uint8_t garbage[4] = {0x00,0x00,0x00,0x00};
    
    nrfTransmit(garbage, bitField, 4);
     
    
    
    //nrfTransmit(bitField, garbage, 4);//echo for debugging
    */
    //if(0x01 & bitField[0]){   // 0001  Get speed
				Delayms(100);
        runTestCase(&spi_Data.speed.key);
    //}
    
    //if(0x02 & bitField[0]){  // 0010   Get Cadence
				Delayms(100);
        runTestCase(&spi_Data.cadence.key);
    //}
    
    //if(0x04 & bitField[0]){  // 0100   Get HR
				Delayms(100);
        runTestCase(&spi_Data.distance.key);
    //}
    //if(0x08 & bitField[0]){  // 1000    Get Batt
				Delayms(100);
        runTestCase(&spi_Data.heartRate.key);
    //}
    
    
    return true;
}

void runTestCase(uint8_t* target){
#ifdef DEBUG
			formatString(spiOutput, sizeof(spiOutput), "Starting Test Case With Key: %d\n", *target);
			TM_USART_Puts(USART3, spiOutput);
#endif
    for(uint8_t i = 0; i<4; i++){
        uint8_t testGarbage[4] = {0x00,0x00,0x00,0x00};
        
        memset(testGarbage,0x00,sizeof(testGarbage));
				//TRACE("Test Garbage: %d,%d,%d,%d\n", testGarbage[0], testGarbage[1], testGarbage[2], testGarbage[3]);
        nrfTransmitSingle(target, &testGarbage[0]);// This transfer gives the NRF the instruction
        nrfTransmitSingle(&testGarbage[0], &spi_Data.speed.value);// This transfer allows the NRF to clock the data to us
				TRACE("Target: %d, Test Garbage: %d,%d,%d,%d, Value: %d\n", *target, testGarbage[0], testGarbage[1], testGarbage[2], testGarbage[3], spi_Data.speed.value);
#ifdef DEBUG
				formatString(spiOutput, sizeof(spiOutput), "Target: %d, Test Garbage: %d,%d,%d,%d, Value: %d\n", *target, testGarbage[0], testGarbage[1], testGarbage[2], testGarbage[3], spi_Data.speed.value);
				TM_USART_Puts(USART3, spiOutput);
#endif
				//TRACE("Target: %d, Test Garbage: %d,%d,%d,%d\n", *target, testGarbage[0], testGarbage[1], testGarbage[2], testGarbage[3]);
    }
}



bool nrfTransmitSingle(uint8_t* in, uint8_t* out){
    return nrfTransmit(in, out, 1);    
}


bool nrfTransmit(uint8_t *buffIn, uint8_t *buffOut, uint32_t len) {
    while(nrfTransmit2(buffIn, buffOut, len)==false){
        //if we go here not working
			TRACE("SPI NOT WORKING\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "SPI NOT WORKING\n");
#endif
    }
    return true;
}   

bool nrfTransmit2(uint8_t *buffIn, uint8_t *buffOut, uint32_t len) {
    Delayms(7);
    TRACE("Transmit NRF\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "Transmit NRF\n");
#endif
		TM_GPIO_SetPinLow(GPIOH, GPIO_PIN_6);
    Delayms(1);
    for(int i = 0; i<1; i++){
        
        TM_SPI_SendMulti(SPI2, buffIn, buffOut,len);
        if(buffOut[0] == 0xDF){
						TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
            Delayms(1);
            return false;
        }
            
    }
		TM_GPIO_SetPinHigh(GPIOH, GPIO_PIN_6);
    TRACE("Finished Transmit NRF\n");
#ifdef DEBUG
			TM_USART_Puts(USART3, "Finished Transmit NRF\n");
#endif
    return true;
}