/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if GFX_USE_GDISP

#if defined(GDISP_SCREEN_HEIGHT)
	#warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
	#undef GISP_SCREEN_HEIGHT
#endif
#if defined(GDISP_SCREEN_WIDTH)
	#warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
	#undef GDISP_SCREEN_WIDTH
#endif

#if defined(LTDC_USE_DMA2D) && !LTDC_USE_DMA2D
 	#error You have to use DMA2D (I think..)!
#endif

#define GDISP_DRIVER_VMT			GDISPVMT_STM32LTDC
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

// #include "stm32_ltdc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_ltdc.h"

#include "stm32469i_discovery_lcd.h"

#if LTDC_USE_DMA2D
 	#include "stm32_dma2d.h"
#endif

#if GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB565
    #warning NOT 100% SURE IF THIS OPTION IS FULLY PORTED!
    #define OTM8009A_COLMOD         OTM8009A_COLMOD_RGB565
    #define FRAME_BUFFER_FORMAT     LTDC_PIXEL_FORMAT_RGB565
    #define FRAME_BUFFER_DEPTH      2
    #define DSI_COLORCODING         DSI_RGB565
    
#elif GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB888
    #define OTM8009A_COLMOD         OTM8009A_COLMOD_RGB888
    #define FRAME_BUFFER_FORMAT     LTDC_PIXEL_FORMAT_RGB888
    #define FRAME_BUFFER_DEPTH      3
    #define DSI_COLORCODING         DSI_RGB888
    
#else
	#error "GDISP: STM32LTDC - unsupported pixel format"
#endif

LTDC_LayerCfgTypeDef LayerConfig;

/* Screen is in landscape mode */
#define FRAME_BUFFER_WIDTH    OTM8009A_800X480_WIDTH
#define FRAME_BUFFER_HEIGHT   OTM8009A_800X480_HEIGHT

/* Calculated addresses for framebuffer(s) and memory manager */
#define FRAME_BUFFER_SIZE     FRAME_FRAME_BUFFER_WIDTH * FRAME_FRAME_BUFFER_HEIGHT * FRAME_BUFFER_DEPTH
#define FRAME_BUFFER_ADDR     (void*)(LCD_FB_START_ADDRESS)

#define LAYER_INDEX     0

#define VSYNC           1  
#define VBP             1 
#define VFP             1
#define HSYNC           1
#define HBP             1
#define HFP             1

#define LEFT_AREA       1
#define RIGHT_AREA      2
#define FINISHED_AREA   3

#define hltdc_handle    hltdc_eval
#define hdsi_handle     hdsi_eval

extern LTDC_HandleTypeDef hltdc_handle;
extern DSI_HandleTypeDef  hdsi_handle;

static int32_t pending_buffer = -1;
static int32_t active_area    = LEFT_AREA;

static uint8_t pColLeft[]    = {0x00, 0x00, 0x01, 0x8F}; /*   0 -> 399 */
static uint8_t pColRight[]   = {0x01, 0x90, 0x03, 0x1F}; /* 400 -> 799 */
static uint8_t pPage[]       = {0x00, 0x00, 0x01, 0xDF}; /*   0 -> 479 */
static uint8_t pScanCol[]    = {0x02, 0x15};             /* Scan @ 533 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	50
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/*******************************************************************************
* FUNCTION:
*   HAL_DSI_TearingEffectCallback
*
* DESCRIPTION:
*   Tearing Effect DSI callback.
*
* ARGUMENTS:
*   hdsi - Pointer to a DSI_HandleTypeDef structure that contains the 
*     configuration information for the DSI.
*
* RETURN VALUE:
*   None
*
*******************************************************************************/
void HAL_DSI_TearingEffectCallback( DSI_HandleTypeDef* hdsi )
{
  /* Mask the TE */
  HAL_DSI_ShortWrite( hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, OTM8009A_CMD_TEOFF, 0x00 );
  
  /* Refresh the right part of the display */
  HAL_DSI_Refresh( hdsi );   
}


/*******************************************************************************
* FUNCTION:
*   HAL_DSI_EndOfRefreshCallback
*
* DESCRIPTION:
*   End of Refresh DSI callback.
*
* ARGUMENTS:
*   hdsi - Pointer to a DSI_HandleTypeDef structure that contains the 
*     configuration information for the DSI.
*
* RETURN VALUE:
*   None
*
*******************************************************************************/
void HAL_DSI_EndOfRefreshCallback( DSI_HandleTypeDef* hdsi )
{
  if ( pending_buffer >= 0 )
  {
    if ( active_area == LEFT_AREA )
    { 
      /* determine the number of bytes per pixel */
      int bpp;
      switch ( LayerConfig.PixelFormat )
      {
        case LTDC_PIXEL_FORMAT_ARGB8888:
          bpp = 4; 
          break;
        case  LTDC_PIXEL_FORMAT_RGB888:
          bpp = 3;
          break;
        case  LTDC_PIXEL_FORMAT_RGB565:
        case  LTDC_PIXEL_FORMAT_ARGB4444:
          bpp = 2;
          break;
        default:
          bpp = 1;  
      }
          
      /* Disable DSI Wrapper */
      __HAL_DSI_WRAPPER_DISABLE( hdsi );

      /* Update LTDC configuaration */
      LTDC_LAYER( &hltdc_handle, LAYER_INDEX )->CFBAR = LayerConfig.FBStartAdress + LayerConfig.ImageWidth * bpp;
      __HAL_LTDC_RELOAD_CONFIG( &hltdc_handle );

      /* Enable DSI Wrapper */
      __HAL_DSI_WRAPPER_ENABLE( hdsi );
      
      HAL_DSI_LongWrite( hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColRight );

      /* Refresh the display */
      HAL_DSI_Refresh( hdsi );    
      
      active_area = RIGHT_AREA; 
    }
    else if ( active_area == RIGHT_AREA )
    {
      /* Disable DSI Wrapper */
      __HAL_DSI_WRAPPER_DISABLE( hdsi );
      
      /* Update LTDC configuaration */
      LTDC_LAYER( &hltdc_handle, LAYER_INDEX )->CFBAR = LayerConfig.FBStartAdress;
      __HAL_LTDC_RELOAD_CONFIG( &hltdc_handle );
      
      /* Enable DSI Wrapper */
      __HAL_DSI_WRAPPER_ENABLE( hdsi );
      
      HAL_DSI_LongWrite( hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColLeft ); 

      /* Refresh the display */
      HAL_DSI_Refresh( hdsi );    
      
      active_area = FINISHED_AREA; 
    }
    else
    {
      active_area = LEFT_AREA; 
      pending_buffer = -1;     
    }
  }
}

LLDSPEC bool_t gdisp_lld_init(GDisplay* g) {
    GPIO_InitTypeDef GPIO_Init_Structure = {0};
	DSI_CmdCfgTypeDef CmdCfg        = {0};
    DSI_LPCmdTypeDef LPCmd          = {0};
    DSI_PLLInitTypeDef dsiPllInit   = {0};

    /* Toggle Hardware Reset of the DSI LCD using its XRES signal (active low) */
    BSP_LCD_Reset();

    /* Call first MSP Initialize only in case of first initialization
    * This will set IP blocks LTDC, DSI and DMA2D
    * - out of reset
    * - clocked
    * - NVIC IRQ related to IP blocks enabled
    */
    BSP_LCD_MspInit();

    /* Base address of DSI Host/Wrapper registers to be set before calling De-Init */
    hdsi_handle.Instance = DSI;

    HAL_DSI_DeInit(&(hdsi_handle));

    #if defined(USE_STM32469I_DISCO_REVA)  
    dsiPllInit.PLLNDIV  = 100;
    dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV5;
    #else
    dsiPllInit.PLLNDIV  = 125;
    dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV2;  
    #endif  /* USE_STM32469I_DISCO_REVA */
    dsiPllInit.PLLODF  = DSI_PLL_OUT_DIV1;   

    hdsi_handle.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
    hdsi_handle.Init.TXEscapeCkdiv = 0x4;
    HAL_DSI_Init(&(hdsi_handle), &(dsiPllInit));

    /* Configure the DSI for Command mode */
    CmdCfg.VirtualChannelID      = 0;
    CmdCfg.HSPolarity            = DSI_HSYNC_ACTIVE_HIGH;
    CmdCfg.VSPolarity            = DSI_VSYNC_ACTIVE_HIGH;
    CmdCfg.DEPolarity            = DSI_DATA_ENABLE_ACTIVE_HIGH;
    CmdCfg.ColorCoding           = DSI_COLORCODING;
    CmdCfg.CommandSize           = FRAME_BUFFER_WIDTH / 2; /* screen is diveded into 2 areas! */
    CmdCfg.TearingEffectSource   = DSI_TE_DSILINK;
    CmdCfg.TearingEffectPolarity = DSI_TE_RISING_EDGE;
    CmdCfg.VSyncPol              = DSI_VSYNC_FALLING;
    CmdCfg.AutomaticRefresh      = DSI_AR_DISABLE;
    CmdCfg.TEAcknowledgeRequest  = DSI_TE_ACKNOWLEDGE_ENABLE;
    HAL_DSI_ConfigAdaptedCommandMode(&hdsi_handle, &CmdCfg);

    LPCmd.LPGenShortWriteNoP     = DSI_LP_GSW0P_ENABLE;
    LPCmd.LPGenShortWriteOneP    = DSI_LP_GSW1P_ENABLE;
    LPCmd.LPGenShortWriteTwoP    = DSI_LP_GSW2P_ENABLE;
    LPCmd.LPGenShortReadNoP      = DSI_LP_GSR0P_ENABLE;
    LPCmd.LPGenShortReadOneP     = DSI_LP_GSR1P_ENABLE;
    LPCmd.LPGenShortReadTwoP     = DSI_LP_GSR2P_ENABLE;
    LPCmd.LPGenLongWrite         = DSI_LP_GLW_ENABLE;
    LPCmd.LPDcsShortWriteNoP     = DSI_LP_DSW0P_ENABLE;
    LPCmd.LPDcsShortWriteOneP    = DSI_LP_DSW1P_ENABLE;
    LPCmd.LPDcsShortReadNoP      = DSI_LP_DSR0P_ENABLE;
    LPCmd.LPDcsLongWrite         = DSI_LP_DLW_ENABLE;
    HAL_DSI_ConfigCommand(&hdsi_handle, &LPCmd);

    /* De-Initialize LTDC */
    HAL_LTDC_DeInit(&hltdc_handle);

    /* Configure LTDC */
    hltdc_handle.Init.HorizontalSync     = HSYNC;
    hltdc_handle.Init.VerticalSync       = VSYNC;
    hltdc_handle.Init.AccumulatedHBP     = HSYNC + HBP;
    hltdc_handle.Init.AccumulatedVBP     = VSYNC + VBP;
    hltdc_handle.Init.AccumulatedActiveH = VSYNC + VBP + FRAME_BUFFER_HEIGHT;
    hltdc_handle.Init.AccumulatedActiveW = HSYNC + HBP + FRAME_BUFFER_WIDTH / 2; /* screen is diveded into 2 areas! */
    hltdc_handle.Init.TotalHeigh         = VSYNC + VBP + FRAME_BUFFER_HEIGHT + VFP;
    hltdc_handle.Init.TotalWidth         = HSYNC + HBP + FRAME_BUFFER_WIDTH / 2 + HFP; /* screen is diveded into 2 areas! */
    hltdc_handle.Init.Backcolor.Blue     = 0;
    hltdc_handle.Init.Backcolor.Green    = 0;
    hltdc_handle.Init.Backcolor.Red      = 0;
    hltdc_handle.Init.HSPolarity         = LTDC_HSPOLARITY_AL;
    hltdc_handle.Init.VSPolarity         = LTDC_VSPOLARITY_AL;
    hltdc_handle.Init.DEPolarity         = LTDC_DEPOLARITY_AL;
    hltdc_handle.Init.PCPolarity         = LTDC_PCPOLARITY_IPC;
    hltdc_handle.Instance                = LTDC;

    /* Initialize LTDC */
    HAL_LTDC_Init(&hltdc_handle);

    /* Start DSI */
    HAL_DSI_Start(&(hdsi_handle));

    /* Initialize the OTM8009A LCD Display IC Driver (KoD LCD IC Driver)
    *  depending on configuration set in 'hdsivideo_handle'.
    */
    OTM8009A_Init(OTM8009A_COLMOD, LCD_ORIENTATION_LANDSCAPE);

    LPCmd.LPGenShortWriteNoP    = DSI_LP_GSW0P_DISABLE;
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
    HAL_DSI_ConfigCommand(&hdsi_handle, &LPCmd);

    HAL_DSI_ConfigFlowControl(&hdsi_handle, DSI_FLOW_CONTROL_BTA);

    /* Refresh the display */
    HAL_DSI_Refresh(&hdsi_handle);

    /* Layer Init */
    LayerConfig.WindowX0 = 0;
    LayerConfig.WindowX1 = FRAME_BUFFER_WIDTH / 2;
    LayerConfig.WindowY0 = 0;
    LayerConfig.WindowY1 = FRAME_BUFFER_HEIGHT; 
    LayerConfig.PixelFormat = FRAME_BUFFER_FORMAT;
    LayerConfig.FBStartAdress = (uint32_t)FRAME_BUFFER_ADDR;
    LayerConfig.Alpha = 255;
    LayerConfig.Alpha0 = 0;
    LayerConfig.Backcolor.Blue = 0;
    LayerConfig.Backcolor.Green = 0;
    LayerConfig.Backcolor.Red = 0;
    LayerConfig.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    LayerConfig.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    LayerConfig.ImageWidth = FRAME_BUFFER_WIDTH / 2;
    LayerConfig.ImageHeight = FRAME_BUFFER_HEIGHT;
HAL_LTDC_ConfigCLUT
    HAL_LTDC_ConfigLayer(&hltdc_handle, &LayerConfig, LAYER_INDEX ); 

    BSP_LCD_SelectLayer( LAYER_INDEX ); 

    HAL_DSI_LongWrite( &hdsi_handle, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColLeft );
    HAL_DSI_LongWrite( &hdsi_handle, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_PASET, pPage );

    /* Update pitch : the draw is done on the whole physical X Size */
    HAL_LTDC_SetPitch( &hltdc_handle, FRAME_BUFFER_WIDTH, LAYER_INDEX );

    pending_buffer = 0;
    active_area = LEFT_AREA;

    HAL_DSI_LongWrite(&hdsi_handle, 0, DSI_DCS_LONG_PKT_WRITE, 2, OTM8009A_CMD_WRTESCN, pScanCol);

    /* short delay necessary to ensure proper DSI update... */
    HAL_Delay( 100 );

#warning TODO set_backlight

	// Initialise the GDISP structure
	g->priv = 0;
	g->board = 0;
	g->g.Width = FRAME_BUFFER_WIDTH;
	g->g.Height = FRAME_BUFFER_HEIGHT;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;

	return TRUE;
}

LLDSPEC void gdisp_lld_draw_pixel(GDisplay* g) {
	unsigned	pos;

	#if GDISP_NEED_CONTROL
    switch(g->g.Orientation) {
		case GDISP_ROTATE_0:
		default:
			pos = PIXIL_POS(g, g->p.x, g->p.y);
			break;
		case GDISP_ROTATE_90:
			pos = PIXIL_POS(g, g->p.y, g->g.Width-g->p.x-1);
			break;
		case GDISP_ROTATE_180:
			pos = PIXIL_POS(g, g->g.Width-g->p.x-1, g->g.Height-g->p.y-1);
			break;
		case GDISP_ROTATE_270:
			pos = PIXIL_POS(g, g->g.Height-g->p.y-1, g->p.x);
			break;
		}
	#else
		pos = PIXIL_POS(g, g->p.x, g->p.y);
	#endif

	#if LTDC_USE_DMA2D
		while(DMA2D->CR & DMA2D_CR_START);
	#endif

	PIXEL_ADDR(g, pos)[0] = gdispColor2Native(g->p.color);
}

LLDSPEC	color_t gdisp_lld_get_pixel_color(GDisplay* g) {
	unsigned		pos;
	LLDCOLOR_TYPE	color;

	#if GDISP_NEED_CONTROL
		switch(g->g.Orientation) {
		case GDISP_ROTATE_0:
		default:
			pos = PIXIL_POS(g, g->p.x, g->p.y);
			break;
		case GDISP_ROTATE_90:
			pos = PIXIL_POS(g, g->p.y, g->g.Width-g->p.x-1);
			break;
		case GDISP_ROTATE_180:
			pos = PIXIL_POS(g, g->g.Width-g->p.x-1, g->g.Height-g->p.y-1);
			break;
		case GDISP_ROTATE_270:
			pos = PIXIL_POS(g, g->g.Height-g->p.y-1, g->p.x);
			break;
		}
	#else
		pos = PIXIL_POS(g, g->p.x, g->p.y);
	#endif

	#if LTDC_USE_DMA2D
		while(DMA2D->CR & DMA2D_CR_START);
	#endif

	color = PIXEL_ADDR(g, pos)[0];
	
	return gdispNative2Color(color);
}

#if GDISP_NEED_CONTROL
LLDSPEC void gdisp_lld_control(GDisplay* g) {
    switch(g->p.x) {
    case GDISP_CONTROL_POWER:
        if (g->g.Powermode == (powermode_t)g->p.ptr)
            return;
        switch((powermode_t)g->p.ptr) {
        case powerOff: case powerOn: case powerSleep: case powerDeepSleep:
            // TODO
            break;
        default:
            return;
        }
        g->g.Powermode = (powermode_t)g->p.ptr;
        return;

    case GDISP_CONTROL_ORIENTATION:
        if (g->g.Orientation == (orientation_t)g->p.ptr)
            return;
        switch((orientation_t)g->p.ptr) {
            case GDISP_ROTATE_0:
            case GDISP_ROTATE_180:
                if (g->g.Orientation == GDISP_ROTATE_90 || g->g.Orientation == GDISP_ROTATE_270) {
                    coord_t		tmp;

                    tmp = g->g.Width;
                    g->g.Width = g->g.Height;
                    g->g.Height = tmp;
                }
                break;
            case GDISP_ROTATE_90:
            case GDISP_ROTATE_270:
                if (g->g.Orientation == GDISP_ROTATE_0 || g->g.Orientation == GDISP_ROTATE_180) {
                    coord_t		tmp;

                    tmp = g->g.Width;
                    g->g.Width = g->g.Height;
                    g->g.Height = tmp;
                }
                break;
            default:
                return;
        }
        g->g.Orientation = (orientation_t)g->p.ptr;
        return;

    case GDISP_CONTROL_BACKLIGHT:
        if ((unsigned)g->p.ptr > 100) g->p.ptr = (void *)100;
        set_backlight(g, (unsigned)g->p.ptr);
        g->g.Backlight = (unsigned)g->p.ptr;
        return;

    case GDISP_CONTROL_CONTRAST:
        if ((unsigned)g->p.ptr > 100) g->p.ptr = (void *)100;
        // TODO
        g->g.Contrast = (unsigned)g->p.ptr;
        return;
    }
}
#endif  /* GDISP_NEED_CONTROL */

#if LTDC_USE_DMA2D
	static void dma2d_init(void) {
		// Enable DMA2D clock
		RCC->AHB1ENR |= RCC_AHB1ENR_DMA2DEN;
	
		// Output color format
		#if GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB565
			DMA2D->OPFCCR = OPFCCR_RGB565;
		#elif GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB888
			DMA2D->OPFCCR = OPFCCR_OPFCCR_RGB888;
		#endif
	
		// Foreground color format
		#if GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB565
			DMA2D->FGPFCCR = FGPFCCR_CM_RGB565;
		#elif GDISP_LLD_PIXELFORMAT == GDISP_PIXELFORMAT_RGB888
			DMA2D->FGPFCCR = FGPFCCR_CM_RGB888;
		#endif
	}

	// Uses p.x,p.y  p.cx,p.cy  p.color
	LLDSPEC void gdisp_lld_fill_area(GDisplay* g)
	{	
		uint32_t pos;
		uint32_t lineadd;
		uint32_t shape;

		// Wait until DMA2D is ready
		while(DMA2D->CR & DMA2D_CR_START);

		#if GDISP_NEED_CONTROL
			switch(g->g.Orientation) {
			case GDISP_ROTATE_0:
			default:
				pos = PIXIL_POS(g, g->p.x, g->p.y);
				lineadd = g->g.Width - g->p.cx;
				shape = (g->p.cx << 16) | (g->p.cy);
				break;
			case GDISP_ROTATE_90:
				pos = PIXIL_POS(g, g->p.y, g->g.Width-g->p.x-g->p.cx);
				lineadd = g->g.Height - g->p.cy;
				shape = (g->p.cy << 16) | (g->p.cx);
				break;
			case GDISP_ROTATE_180:
				pos = PIXIL_POS(g, g->g.Width-g->p.x-g->p.cx, g->g.Height-g->p.y-g->p.cy);
				lineadd = g->g.Width - g->p.cx;
				shape = (g->p.cx << 16) | (g->p.cy);
				break;
			case GDISP_ROTATE_270:
				pos = PIXIL_POS(g, g->g.Height-g->p.y-g->p.cy, g->p.x);
				lineadd = g->g.Height - g->p.cy;
				shape = (g->p.cy << 16) | (g->p.cx);
				break;
			}
		#else
			pos = PIXIL_POS(g, g->p.x, g->p.y);
			lineadd = g->g.Width - g->p.cx;
			shape = (g->p.cx << 16) | (g->p.cy);
		#endif
		
		// Start the DMA2D
		DMA2D->OMAR = (uint32_t)PIXEL_ADDR(g, pos);
		DMA2D->OOR = lineadd;
		DMA2D->NLR = shape;
		DMA2D->OCOLR = (uint32_t)(gdispColor2Native(g->p.color));
		DMA2D->CR = DMA2D_CR_MODE_R2M | DMA2D_CR_START;	
	}

	/* Oops - the DMA2D only supports GDISP_ROTATE_0.
	 *
	 * Where the width is 1 we can trick it for other orientations.
	 * That is worthwhile as a width of 1 is common. For other
	 * situations we need to fall back to pixel pushing.
	 *
	 * Additionally, although DMA2D can translate color formats
	 * it can only do it for a small range of formats. For any
	 * other formats we also need to fall back to pixel pushing.
	 *
	 * As the code to actually do all that for other than the
	 * simplest case (orientation == GDISP_ROTATE_0 and
	 * GDISP_PIXELFORMAT == GDISP_LLD_PIXELFORMAT) is very complex
	 * we will always pixel push for now. In practice that is OK as
	 * access to the framebuffer is fast - probably faster than DMA2D.
	 * It just uses more CPU.
	 */
	#if GDISP_HARDWARE_BITFILLS 
		// Uses p.x,p.y  p.cx,p.cy  p.x1,p.y1 (=srcx,srcy)  p.x2 (=srccx), p.ptr (=buffer)
		LLDSPEC void gdisp_lld_blit_area(GDisplay* g) {
			// Wait until DMA2D is ready
			while(DMA2D->CR & DMA2D_CR_START);

			// Source setup
			DMA2D->FGMAR = LTDC_PIXELBYTES * (g->p.y1 * g->p.x2 + g->p.x1) + (uint32_t)g->p.ptr;
			DMA2D->FGOR = g->p.x2 - g->p.cx;
		
			// Output setup
			DMA2D->OMAR = (uint32_t)PIXEL_ADDR(g, PIXIL_POS(g, g->p.x, g->p.y));
			DMA2D->OOR = g->g.Width - g->p.cx;
			DMA2D->NLR = (g->p.cx << 16) | (g->p.cy);

			// Set MODE to M2M and Start the process
			DMA2D->CR = DMA2D_CR_MODE_M2M | DMA2D_CR_START;
		}
	#endif

#endif /* LTDC_USE_DMA2D */

#endif /* GFX_USE_GDISP */
