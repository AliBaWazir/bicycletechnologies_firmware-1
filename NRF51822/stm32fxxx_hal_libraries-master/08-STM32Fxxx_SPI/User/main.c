/**
 * Keil project example for SPI communication
 *
 * Before you start, select your target, on the right of the "Load" button
 *
 * @author    Tilen Majerle
 * @email     tilen@majerle.eu
 * @website   http://stm32f4-discovery.net
 * @ide       Keil uVision 5
 * @conf      PLL parameters are set in "Options for Target" -> "C/C++" -> "Defines"
 * @packs     STM32F4xx/STM32F7xx Keil packs are requred with HAL driver support
 * @stdperiph STM32F4xx/STM32F7xx HAL drivers required
 */
/* Include core modules */
#include "stm32fxxx_hal.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32_disco.h"
#include "tm_stm32_spi.h"
#include "string.h"
#include "tm_stm32_delay.h"
#include <stdbool.h>
/* Data for send and receive */
uint8_t Transmit[15], Receive[15];
bool nrfTransmit(uint8_t*, uint8_t*, uint16_t);
bool nrfTransmitSingle(uint8_t*, uint8_t*);
bool nrfGetData(void);

//initialize the SPI data struct containing all the data coming from the NRF51822
struct SPI_data{
    struct Speed{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }speed;
    struct Cadence{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }cadence;
    struct HeartRate{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }heartRate;
    struct Batt{
        uint8_t value;
        uint32_t age;
        uint8_t key;
    }batt;
}spi_Data;


struct SPI_data spi_Data; //Create instance


int main(void) {

	
	/* Init system clock for maximum system speed */
	TM_RCC_InitSystem();
    
    /* Init HAL layer */
	HAL_Init();
	TM_DELAY_Init();
	/* Init leds */
	TM_DISCO_LedInit();
    // Init CS
    TM_GPIO_Init(GPIOD, GPIO_PIN_4, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_Low);
    //Set high (active low)
    TM_GPIO_SetPinHigh(GPIOD, GPIO_PIN_4);
    
    //Initialize Vars
	spi_Data.speed.key = 0x01;
    spi_Data.cadence.key = 0x02;
    spi_Data.heartRate.key = 0x04;
    spi_Data.batt.key = 0x08;
    
        
    
    Delay(100); // This delay needed for some reason, so the NRF has time.
    
    
	/* Init SPI */
	TM_SPI_Init(SPI2, TM_SPI_PinsPack_1);

    nrfGetData(); // Get struct data
    
    while(1){}; //Do nothing forever


}

bool nrfGetData(){
    uint8_t command = 0xDA; 
    uint8_t retVal = 0xFF;
    nrfTransmitSingle(&command, &retVal); // send command to retrieve bitfield list
    

    uint8_t bitField[4] = {0x00,0x00,0x00,0x00};
    uint8_t garbage[4] = {0x00,0x00,0x00,0x00};
    
    nrfTransmit(garbage, bitField, 4);
     
    
    
    //nrfTransmit(bitField, garbage, 4);//echo for debugging
    
    if(0x01 & bitField[0]){   // 0001  Get speed
        memset(garbage,0x00,sizeof(garbage));
        nrfTransmitSingle(&spi_Data.speed.key, &garbage[0]);// This transfer gives the NRF the instruction
        nrfTransmitSingle(&garbage[0], &spi_Data.speed.value);// This transfer allows the NRF to clock the data to us
    }
    
    if(0x02 & bitField[0]){  // 0010   Get Cadence
        memset(garbage,0x00,sizeof(garbage)); //clean up so the scope is cleaner
        nrfTransmitSingle(&spi_Data.cadence.key, &garbage[0]);
        nrfTransmitSingle(&garbage[0], &spi_Data.cadence.value);
    }
    
    if(0x04 & bitField[0]){  // 0100   Get 
        memset(garbage,0x00,sizeof(garbage));
        nrfTransmitSingle(&spi_Data.heartRate.key, &garbage[0]);
        nrfTransmitSingle(&garbage[0], &spi_Data.heartRate.value);
    }
    if(0x08 & bitField[0]){  // 1000
        memset(garbage,0x00,sizeof(garbage));
        nrfTransmitSingle(&spi_Data.batt.key, &garbage[0]);
        nrfTransmitSingle(&garbage[0], &spi_Data.batt.value);
    }
    
    
    return true;
}

bool nrfTransmitSingle(uint8_t* in, uint8_t* out){
    return nrfTransmit(in, out, 1);    
}


bool nrfTransmit(uint8_t *buffIn, uint8_t *buffOut, uint16_t len) {
    Delay(75);
    TM_GPIO_SetPinLow(GPIOD, GPIO_PIN_4);
    Delay(8);
    for(int i = 0; i<1; i++){
        TM_SPI_SendMulti(SPI2, buffIn, buffOut,len);
    }
    TM_GPIO_SetPinHigh(GPIOD, GPIO_PIN_4);
    return true;
}

/* Custom SPI initialization */
void TM_SPI_InitCustomPinsCallback(SPI_TypeDef* SPIx, uint16_t AlternateFunction) {
	/* SPI callback */
	if (SPIx == SPI2) {
		/* Pins on STM32F7-Discovery on Arduino header */
		TM_GPIO_InitAlternate(GPIOB, GPIO_PIN_14 | GPIO_PIN_15, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, AlternateFunction);
		TM_GPIO_InitAlternate(GPIOI, GPIO_PIN_1, TM_GPIO_OType_PP, TM_GPIO_PuPd_NOPULL, TM_GPIO_Speed_High, AlternateFunction);
	}
}
