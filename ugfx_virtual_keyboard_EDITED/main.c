#include <stdio.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "stm32469i_discovery_sdram.h"
#include "RTE_Components.h"
#include "gfx.h"
#include "gui.h"
#include "trace.h"
#include "tm_stm32_gps.h"
#include "tm_stm32_delay.h"
#include "gps.h"

#ifdef RTE_CMSIS_RTOS_RTX
extern uint32_t os_time;

uint32_t HAL_GetTick(void) { 
  return os_time; 
}

#endif

/*******************************************************************************
* FUNCTION:
*   SystemClock_Config
*
* DESCRIPTION:
*   System Clock Configuration
*     The system Clock is configured as follow : 
*     System Clock source            = PLL (HSE)
*     SYSCLK(Hz)                     = 180000000
*     HCLK(Hz)                       = 180000000
*     AHB Prescaler                  = 1
*     APB1 Prescaler                 = 4
*     APB2 Prescaler                 = 2
*     HSE Frequency(Hz)              = 8000000
*     PLL_M                          = 8
*     PLL_N                          = 360
*     PLL_P                          = 2
*     PLL_Q                          = 7
*     PLL_R                          = 6
*     VDD(V)                         = 3.3
*     Main regulator output voltage  = Scale1 mode
*     Flash Latency(WS)              = 5
*
* ARGUMENTS:
*   None
*
* RETURN VALUE:
*   None
*
*******************************************************************************/
void SystemClock_Config( void )
{
  RCC_ClkInitTypeDef        RCC_ClkInitStruct;
  RCC_OscInitTypeDef        RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
#if defined(USE_STM32469I_DISCO_REVA)
  RCC_OscInitStruct.PLL.PLLM       = 25;
#else
  RCC_OscInitStruct.PLL.PLLM       = 8;
#endif /* USE_STM32469I_DISCO_REVA */
  RCC_OscInitStruct.PLL.PLLN       = 360;
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ       = 7;
  RCC_OscInitStruct.PLL.PLLR       = 6;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Activate the Over-Drive mode */
  HAL_PWREx_EnableOverDrive();
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  /* LCD clock configuration */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 MHz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 417 MHz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 417 MHz / 5 = 83.4 MHz */
  /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 83.4 / 2 = 41.7 MHz */
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct = {0};
  HAL_RCCEx_GetPeriphCLKConfig( &PeriphClkInitStruct );

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 7;
  PeriphClkInitStruct.PLLSAIDivR     = RCC_PLLSAIDIVR_2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}
#include "stm32469i_discovery_lcd.h"

#if 0
void DMA2D_IRQHandler( void )
{  
  HAL_DMA2D_IRQHandler( &Accelerator );
}

void LTDC_IRQHandler( void )
{
  BSP_LCD_LTDC_IRQHandler();
}

void LTDC_ER_IRQHandler( void )
{
  BSP_LCD_LTDC_ER_IRQHandler();
}

void DSI_IRQHandler()
{
    BSP_LCD_DSI_IRQHandler();
}
#endif

void Thread_1 (void const *arg)
{
	guiEventLoop();
}		// function prototype for Thread_1
osThreadDef (Thread_1, osPriorityNormal, 1, 0);            // define Thread_1

void Thread_2 (void const *arg)
{
	runGPS();
}
osThreadDef (Thread_2, osPriorityNormal, 1, 0);            // define Thread_2

int main (void)
{			
  int i = 0;
	// Cached enabled in stm32f4xx_hal_conf.h
    // CPU_CACHE_Enable();			// Enable the CPU Cache
    
	HAL_Init();					// Initialize the HAL Library
	BSP_SDRAM_Init();			// Initialize BSP SDRAM
	memset((void*)0xC0000000, 0xA0, 800*480*4 + 1024);
	SystemClock_Config();		// Configure the System Clock
        
	osKernelInitialize();		// Initialize the KEIL RTX operating system
	osKernelStart();			// Start the scheduler
	gfxInit();					// Initialize the uGFX library

#ifdef DEBUG
	/* Initialize USART3 for debug */
	/* TX = PB10 */
	TM_USART_Init(USART3, TM_USART_PinsPack_1, 115200);
	TM_USART_Puts(USART3, "UART PC Output\n");
#endif
	
	/* Init RTC */
	if (TM_RTC_Init(TM_RTC_ClockSource_External)) {
			/* RTC was already initialized and time is running */
			/* No need to set clock now */
	} else {
			/* RTC was now initialized */
			/* If you need to set new time, now is the time to do it */
	}
		
	geventListenerInit(&glistener);
	gwinAttachListener(&glistener);

	guiCreate();

	osMutexDef (MutexIsr);
	traceMutex = osMutexCreate  (osMutex (MutexIsr));
  if (traceMutex != NULL)  {
    // Mutex object created
  }   
  //osThreadId guiThread;
  //osThreadId gpsThread;
  //guiThread = osThreadCreate (osThread (Thread_1), NULL);
	//gpsThread = osThreadCreate (osThread (Thread_2), NULL);
	
	guiEventLoop();
	
//	while(1){
//		i++;
//		if(i > 100){
//			i = 0;
//		}
//	}

}

