/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if GFX_USE_GINPUT && GINPUT_NEED_MOUSE

#define GMOUSE_DRIVER_VMT		GMOUSEVMT_FT6x06
#include "../../../../src/ginput/ginput_driver_mouse.h"

// Get the hardware interface
#include "stm32469i_discovery_ts.h"
#include "stm32469i_discovery_lcd.h"

static bool_t MouseInit(GMouse* m, unsigned driverinstance)
{
    // We only support one of these on this board
	if (driverinstance)
		return FALSE;
    
	return TS_OK == BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
}

static bool_t read_xyz(GMouse* m, GMouseReading* pdr)
{
    TS_StateTypeDef ts_state;

    if (TS_OK != BSP_TS_GetState(&ts_state))
    {
        return FALSE;
    }
    
    if (ts_state.touchDetected)
    {
        pdr->x = (coord_t)ts_state.touchX[0];
		pdr->y = (coord_t)ts_state.touchY[0];
		pdr->z = 1;
        pdr->buttons = GINPUT_TOUCH_PRESSED;  
    }
    else
    {
        pdr->buttons = 0;
        pdr->z = 0;
    }
    
    return TRUE;
}

const GMouseVMT GMOUSE_DRIVER_VMT[1] = {{
	{
		GDRIVER_TYPE_TOUCH,
        #warning This driver could be interrupt driven
		GMOUSE_VFLG_TOUCH | GMOUSE_VFLG_ONLY_DOWN | GMOUSE_VFLG_POORUPDOWN | GMOUSE_VFLG_DEFAULTFINGER, // | GMOUSE_VFLG_CALIBRATE | GMOUSE_VFLG_CAL_TEST,
		sizeof(GMouse),
		_gmouseInitDriver,
		_gmousePostInitDriver,
		_gmouseDeInitDriver
	},
	1,				// z_max - (currently?) not supported
	0,				// z_min - (currently?) not supported
	1,				// z_touchon
	0,				// z_touchoff
    #warning Look into (self-)calibration and error values
	{				// pen_jitter
		0, // GMOUSE_FT6x06_PEN_CALIBRATE_ERROR,			// calibrate
		0, // GMOUSE_FT6x06_PEN_CLICK_ERROR,				// click
		0, // GMOUSE_FT6x06_PEN_MOVE_ERROR				// move
	},
	{				// finger_jitter
		0, // GMOUSE_FT6x06_FINGER_CALIBRATE_ERROR,		// calibrate
		0, // GMOUSE_FT6x06_FINGER_CLICK_ERROR,			// click
		0, // GMOUSE_FT6x06_FINGER_MOVE_ERROR             // move
	},
	MouseInit, 		// init
	NULL,           // deinit
	read_xyz,		// get
	NULL,           // calsave
	NULL            // calload
}};

#endif /* GFX_USE_GINPUT && GINPUT_NEED_MOUSE */

