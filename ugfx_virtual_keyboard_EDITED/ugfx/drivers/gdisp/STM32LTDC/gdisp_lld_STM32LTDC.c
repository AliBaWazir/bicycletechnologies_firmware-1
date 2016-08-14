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

#define GDISP_DRIVER_VMT			GDISPVMT_STM32LTDC
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

// #include "stm32_ltdc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_ltdc.h"

#include "stm32469i_discovery_lcd.h"

#if GDISP_LLD_PIXELFORMAT != GDISP_PIXELFORMAT_RGB888
    #error Pixel format other than RGB888 not supported!
#endif

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

/**
 * @brief   Initialize the driver.
 * @return	TRUE if successful.
 * @param[in]	g					The driver structure
 * @param[out]	g->g				The driver must fill in the GDISPControl structure
 */
LLDSPEC bool_t gdisp_lld_init(GDisplay* g)
{
    if (LCD_OK != BSP_LCD_InitEx(LCD_ORIENTATION_LANDSCAPE))
    {
        return false;
    }        

    BSP_LCD_LayerDefaultInit(LTDC_ACTIVE_LAYER_BACKGROUND, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(LTDC_ACTIVE_LAYER_BACKGROUND);
    BSP_LCD_SetLayerVisible(LTDC_ACTIVE_LAYER_FOREGROUND, DISABLE);

	// Initialise the GDISP structure
	g->priv = 0;
	g->board = 0;
	g->g.Width = (coord_t)BSP_LCD_GetXSize();
	g->g.Height = (coord_t)BSP_LCD_GetYSize();;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;

	return TRUE;
}

/**
 * @brief   Draw a pixel
 * @pre		GDISP_HARDWARE_DRAWPIXEL is TRUE
 *
 * @param[in]	g				The driver structure
 * @param[in]	g->p.x,g->p.y	The pixel position
 * @param[in]	g->p.color		The color to set
 *
 * @note		The parameter variables must not be altered by the driver.
 */
LLDSPEC void gdisp_lld_draw_pixel(GDisplay* g)
{
	coord_t pos_x, pos_y;
    LLDCOLOR_TYPE color;

    #if GDISP_NEED_CONTROL
		switch(g->g.Orientation)
        {
            case GDISP_ROTATE_0:
            default:
                pos_x = g->p.x;
                pos_y = g->p.y;
                break;
            case GDISP_ROTATE_90:
                pos_x = g->p.y;
                pos_y = g->g.Width-g->p.x-1;
                break;
            case GDISP_ROTATE_180:
                pos_x = g->g.Width-g->p.x-1;
                pos_y = g->g.Height-g->p.y-1;
                break;
            case GDISP_ROTATE_270:
                pos_x = g->g.Height-g->p.y-1;
                pos_y = g->p.x;
                break;
		}
	#else
        pos_x = g->p.x;
        pos_y = g->p.y;
	#endif
    
    color = gdispColor2Native(g->p.color);
    BSP_LCD_DrawPixel(pos_x, pos_y, color);
}

/**
 * @brief   Read a pixel from the display
 * @return	The color at the defined position
 * @pre		GDISP_HARDWARE_PIXELREAD is TRUE (and the application needs it)
 *
 * @param[in]	g				The driver structure
 * @param[in]	g->p.x,g->p.y	The pixel position
 *
 * @note		The parameter variables must not be altered by the driver.
 */
LLDSPEC	color_t gdisp_lld_get_pixel_color(GDisplay* g)
{
    coord_t pos_x, pos_y;
    LLDCOLOR_TYPE color;

    #if GDISP_NEED_CONTROL
		switch(g->g.Orientation)
        {
            case GDISP_ROTATE_0:
            default:
                pos_x = g->p.x;
                pos_y = g->p.y;
                break;
            case GDISP_ROTATE_90:
                pos_x = g->p.y;
                pos_y = g->g.Width-g->p.x-1;
                break;
            case GDISP_ROTATE_180:
                pos_x = g->g.Width-g->p.x-1;
                pos_y = g->g.Height-g->p.y-1;
                break;
            case GDISP_ROTATE_270:
                pos_x = g->g.Height-g->p.y-1;
                pos_y = g->p.x;
                break;
		}
	#else
        pos_x = g->p.x;
        pos_y = g->p.y;
	#endif

    color = BSP_LCD_ReadPixel(pos_x, pos_y);
	return gdispNative2Color(color);
}

#if GDISP_NEED_CONTROL
/**
 * @brief   Control some feature of the hardware
 * @pre		GDISP_HARDWARE_CONTROL is TRUE (and the application needs it)
 *
 * @param[in]	g				The driver structure
 * @param[in]	g->p.x			The operation to perform
 * @param[in]	g->p.ptr		The operation parameter
 *
 * @note		The parameter variables must not be altered by the driver.
 */
LLDSPEC void gdisp_lld_control(GDisplay* g)
{
    switch(g->p.x)
    {
        case GDISP_CONTROL_POWER:
            if (g->g.Powermode == (powermode_t)g->p.ptr)
                return;
            switch((powermode_t)g->p.ptr)
            {
                case powerOff:
                    BSP_LCD_DisplayOff();
                    break;
                case powerOn:
                    BSP_LCD_DisplayOn();
                    break;
                default:
                    // TODO
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
            // Not supported!
            return;

        case GDISP_CONTROL_CONTRAST:
            // Not supported!
            return;
    }
}
#endif  /* GDISP_NEED_CONTROL */

/**
 * @brief   Fill an area with a single color
 * @pre		GDISP_HARDWARE_FILLS is TRUE
 *
 * @param[in]	g				The driver structure
 * @param[in]	g->p.x,g->p.y	The area position
 * @param[in]	g->p.cx,g->p.cy	The area size
 * @param[in]	g->p.color		The color to set
 *
 * @note		The parameter variables must not be altered by the driver.
 */
LLDSPEC void gdisp_lld_fill_area(GDisplay* g)
{
    coord_t pos_x, pos_y;
    
    #if GDISP_NEED_CONTROL
        switch(g->g.Orientation)
        {
            case GDISP_ROTATE_0:
            default:
                pos_x = g->p.x;
                pos_y = g->p.y;
                break;
            case GDISP_ROTATE_90:
                pos_x = g->p.y;
                pos_y = g->g.Width-g->p.x-g->p.cx;
                break;
            case GDISP_ROTATE_180:
                pos_x = g->g.Width-g->p.x-g->p.cx;
                pos_y = g->g.Height-g->p.y-g->p.cy;
                break;
            case GDISP_ROTATE_270:
                pos_x = g->g.Height-g->p.y-g->p.cy;
                pos_y = g->p.x;
                break;
        }
    #else
        pos_x = g->p.x;
        pos_y = g->p.y;
    #endif

    // set the color
    BSP_LCD_SetTextColor(gdispColor2Native(g->p.color | LCD_COLOR_TRANSPARENT));
    BSP_LCD_FillRect(pos_x, pos_y, g->p.cx, g->p.cy);
}

/**
 * @brief   Clear the screen using the defined color
 * @pre		GDISP_HARDWARE_CLEARS is TRUE
 *
 * @param[in]	g				The driver structure
 * @param[in]	g->p.color		The color to set
 *
 * @note		The parameter variables must not be altered by the driver.
 */
LLDSPEC	void gdisp_lld_clear(GDisplay *g)
{
    BSP_LCD_Clear(g->p.color | LCD_COLOR_TRANSPARENT);
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
    /**
     * @brief   Fill an area using a bitmap
     * @pre		GDISP_HARDWARE_BITFILLS is TRUE
     *
     * @param[in]	g				The driver structure
     * @param[in]	g->p.x,g->p.y	The area position
     * @param[in]	g->p.cx,g->p.cy	The area size
     * @param[in]	g->p.x1,g->p.y1	The starting position in the bitmap
     * @param[in]	g->p.x2			The width of a bitmap line
     * @param[in]	g->p.ptr		The pointer to the bitmap
     *
     * @note		The parameter variables must not be altered by the driver.
     */
    LLDSPEC void gdisp_lld_blit_area(GDisplay* g)
    {
        BSP_LCD_DrawBitmap(,,,g->p.ptr);
        
        


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
#endif /* GDISP_HARDWARE_BITFILLS */

#endif /* GFX_USE_GDISP */
