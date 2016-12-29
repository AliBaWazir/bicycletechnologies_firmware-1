/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_ltdc.h"

#include "stm32469i_discovery_lcd.h"

#define VSYNC           12
#define VBP             1
#define VFP             1
#define VACT            480
#define HSYNC           120
#define HBP             1
#define HFP             1
#define HACT            800      /* Note: 16bpp: Screen divided in 4 areas of 200 pixels to avoid DSI tearing */
#define DSI_COLOR_CODING	DSI_RGB565
#define OTM_COLOR_FORMAT 	OTM8009A_FORMAT_RBG565

#define __DSI_MASK_TE()       (GPIOJ->AFR[0] &= (0xFFFFF0FFU))   /* Mask DSI TearingEffect Pin*/
#define __DSI_UNMASK_TE()     (GPIOJ->AFR[0] |= ((uint32_t)(GPIO_AF13_DSI) << 8)) /* UnMask DSI TearingEffect Pin*/

// 16 bpp screen map: 4 areas of 200 pixels
uint8_t pCols[4][4] =
{
    {0x00, 0x00, 0x00, 0xC7}, /*   0 -> 199 */
    {0x00, 0xC8, 0x01, 0x8F}, /* 200 -> 399 */
    {0x01, 0x90, 0x02, 0x57}, /* 400 -> 599 */
    {0x02, 0x58, 0x03, 0x1F}, /* 600 -> 799 */
};

// 24 bpp screen map: 2 areas of 400 pixels
uint8_t pColLeft[]    = {0x00, 0x00, 0x01, 0x8F}; /*   0 -> 399 */
uint8_t pColRight[]   = {0x01, 0x90, 0x03, 0x1F}; /* 400 -> 799 */

uint8_t pPage[]       = {0x00, 0x00, 0x01, 0xDF}; /*   0 -> 479 */
//uint8_t pScanCol[]    = {0x02, 0x15};             /* Scan @ 533 */

#ifdef USE_DOUBLE_BUFFERING
  static uint8_t pScanCol[] = {0x01, 0xB0};             /* Scan @ 432 */
#else
  static uint8_t pScanCol[] = {0x02, 0x40};             /* Scan @ 576 */
#endif
	
static DSI_CmdCfgTypeDef CmdCfg;
static DSI_LPCmdTypeDef LPCmd;
static DSI_PLLInitTypeDef dsiPllInit;
static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

static const ltdcConfig driverCfg = {
	HACT, VACT,								// Width, Height (pixels)
	HSYNC, VSYNC,									// Horizontal, Vertical sync (pixels)
	HBP, VBP,									// Horizontal, Vertical back porch (pixels)
	HFP, VFP,									// Horizontal, Vertical front porch (pixels)
	0,										// Sync flags
	0x000000,								// Clear color (RGB888)

	{										// Background layer config
		(LLDCOLOR_TYPE *)(LCD_FB_START_ADDRESS),	// Frame buffer address
		HACT, VACT,							// Width, Height (pixels)
		HACT * LTDC_PIXELBYTES,				// Line pitch (bytes)
		LTDC_PIXELFORMAT,					// Pixel format
		0, 0,								// Start pixel position (x, y)
		HACT, VACT,							// Size of virtual layer (cx, cy)
		LTDC_COLOR_FUCHSIA,					// Default color (ARGB8888)
		DSI_COLOR_CODING,							// Color key (RGB888)
		LTDC_BLEND_FIX1_FIX2,				// Blending factors
		0,									// Palette (RGB888, can be NULL)
		0,									// Palette length
		0xFF,								// Constant alpha factor
		LTDC_LEF_ENABLE						// Layer configuration flags
	},

	LTDC_UNUSED_LAYER_CONFIG				// Foreground layer config
};

static GFXINLINE void init_board(GDisplay* g) {
	g->board = 0;
  

    /* Toggle Hardware Reset of the DSI LCD using
    * its XRES signal (active low) */
    BSP_LCD_Reset();

    /* Call first MSP Initialize only in case of first initialization
    * This will set IP blocks LTDC, DSI and DMA2D
    * - out of reset
    * - clocked
    * - NVIC IRQ related to IP blocks enabled
    */
    BSP_LCD_MspInit();

	/* LCD clock configuration */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
#ifdef USE_DOUBLE_BUFFERING
    /* Double buffering use 384/7/2 = 27.4MHz */
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 364;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 7;
#else
    /* Single buffering use 375/3/2 = 62.5MHz */
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 417;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
#endif
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
		
    /* Base address of DSI Host/Wrapper registers to be set before calling De-Init */
    hdsi_eval.Instance = DSI;

    HAL_DSI_DeInit(&(hdsi_eval));

		#if defined(USE_STM32469I_DISCO_REVA)
			dsiPllInit.PLLNDIV  = 100;
			dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
		#else
			dsiPllInit.PLLNDIV  = 125;
			dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV2;
		#endif  /* USE_STM32469I_DISCO_REVA */

		dsiPllInit.PLLODF  = DSI_PLL_OUT_DIV1;

    hdsi_eval.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
    hdsi_eval.Init.TXEscapeCkdiv = 0x4;
    HAL_DSI_Init(&(hdsi_eval), &(dsiPllInit));

    /* Configure the DSI for Command mode */
    CmdCfg.VirtualChannelID      = 0;
    CmdCfg.HSPolarity            = DSI_HSYNC_ACTIVE_HIGH;
    CmdCfg.VSPolarity            = DSI_VSYNC_ACTIVE_HIGH;
    CmdCfg.DEPolarity            = DSI_DATA_ENABLE_ACTIVE_HIGH;
    CmdCfg.CommandSize           = HACT;
    CmdCfg.TearingEffectSource   = DSI_TE_EXTERNAL;
    CmdCfg.TearingEffectPolarity = DSI_TE_RISING_EDGE;
    CmdCfg.VSyncPol              = DSI_VSYNC_FALLING;
    CmdCfg.AutomaticRefresh      = DSI_AR_DISABLE;
    CmdCfg.TEAcknowledgeRequest  = DSI_TE_ACKNOWLEDGE_ENABLE;
		CmdCfg.ColorCoding           = DSI_COLOR_CODING;

    HAL_DSI_ConfigAdaptedCommandMode(&hdsi_eval, &CmdCfg);

    LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_ENABLE;
    LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_ENABLE;
    LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_ENABLE;
    LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_ENABLE;
    LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_ENABLE;
    LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_ENABLE;
    LPCmd.LPGenLongWrite        = DSI_LP_GLW_ENABLE;
    LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_ENABLE;
    LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_ENABLE;
    LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_ENABLE;
    LPCmd.LPDcsLongWrite        = DSI_LP_DLW_ENABLE;
    HAL_DSI_ConfigCommand(&hdsi_eval, &LPCmd);
}

static GFXINLINE void post_init_board(GDisplay* g)
{
	(void)g;
	GPIO_InitTypeDef GPIO_Init_Structure;
	
	/* Start DSI */
    HAL_DSI_Start(&(hdsi_eval));

    /* Initialize the OTM8009A LCD Display IC Driver (KoD LCD IC Driver)
    *  depending on configuration set in 'hdsivideo_handle'.
    */
    OTM8009A_Init(OTM_COLOR_FORMAT, LCD_ORIENTATION_LANDSCAPE);

    /*LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_DISABLE;
    LPCmd.LPGenShortWriteOneP   = DSI_LP_GSW1P_DISABLE;
    LPCmd.LPGenShortWriteTwoP   = DSI_LP_GSW2P_DISABLE;
    LPCmd.LPGenShortReadNoP     = DSI_LP_GSR0P_DISABLE;
    LPCmd.LPGenShortReadOneP    = DSI_LP_GSR1P_DISABLE;
    LPCmd.LPGenShortReadTwoP    = DSI_LP_GSR2P_DISABLE;
    LPCmd.LPGenLongWrite        = DSI_LP_GLW_DISABLE;
    LPCmd.LPDcsShortWriteNoP    = DSI_LP_DSW0P_DISABLE;
    LPCmd.LPDcsShortWriteOneP   = DSI_LP_DSW1P_DISABLE;
    LPCmd.LPDcsShortReadNoP     = DSI_LP_DSR0P_DISABLE;
    LPCmd.LPDcsLongWrite        = DSI_LP_DLW_DISABLE;
    HAL_DSI_ConfigCommand(&hdsi_eval, &LPCmd);*/

    HAL_DSI_ConfigFlowControl(&hdsi_eval, DSI_FLOW_CONTROL_BTA);


    /* Enable GPIOJ clock */
    __HAL_RCC_GPIOJ_CLK_ENABLE();

    /* Configure DSI_TE pin from MB1166 : Tearing effect on separated GPIO from KoD LCD */
    /* that is mapped on GPIOJ2 as alternate DSI function (DSI_TE)                      */
    /* This pin is used only when the LCD and DSI link is configured in command mode    */
    /* Not used in DSI Video mode.                                                      */

    GPIO_Init_Structure.Pin       = GPIO_PIN_2;
    GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init_Structure.Pull      = GPIO_NOPULL;
    GPIO_Init_Structure.Speed     = GPIO_SPEED_HIGH;
    GPIO_Init_Structure.Alternate = GPIO_AF13_DSI;
    HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);

    /* Refresh the display */
    HAL_DSI_Refresh(&hdsi_eval);

    /* Update pitch : the draw is done on the whole physical X Size */
    HAL_LTDC_SetPitch(&hltdc_eval, BSP_LCD_GetXSize(), 0);
}

static GFXINLINE void set_backlight(GDisplay* g, uint8_t percent)
{
	(void)g;
	(void)percent;
}

#endif /* _GDISP_LLD_BOARD_H */
