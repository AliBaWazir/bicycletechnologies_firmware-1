/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

#include "tm_stm32_i2c.h"

// Resolution and Accuracy Settings
#define GMOUSE_FT6x06_PEN_CALIBRATE_ERROR		8
#define GMOUSE_FT6x06_PEN_CLICK_ERROR			6
#define GMOUSE_FT6x06_PEN_MOVE_ERROR			4
#define GMOUSE_FT6x06_FINGER_CALIBRATE_ERROR	14
#define GMOUSE_FT6x06_FINGER_CLICK_ERROR		18
#define GMOUSE_FT6x06_FINGER_MOVE_ERROR			14

// How much extra data to allocate at the end of the GMouse structure for the board's use
#define GMOUSE_FT6x06_BOARD_DATA_SIZE			0

// Set this to TRUE if you want self-calibration.
//	NOTE:	This is not as accurate as real calibration.
//			It requires the orientation of the touch panel to match the display.
//			It requires the active area of the touch panel to exactly match the display size.
#define GMOUSE_FT6x06_SELF_CALIBRATE			TRUE

#define FT6x06_SLAVE_ADDR 0x54

static bool_t init_board(GMouse* m, unsigned driverinstance) {
	(void)m;

	// Initialize the I2C1 peripheral
	if (TM_I2C_Init(I2C1, TM_I2C_PinsPack_2, 100000)) {
		return FALSE;
	}

	return TRUE;
}
/*
static GFXINLINE void aquire_bus(GMouse* m) {
}

static GFXINLINE void release_bus(GMouse* m) {
}
*/
static void write_reg(GMouse* m, uint8_t reg, uint8_t val) {
	(void)m;

	TM_I2C_Write(I2C1, FT6x06_SLAVE_ADDR, reg, val);
}

static uint8_t read_byte(GMouse* m, uint8_t reg) {
	(void)m;
	
  uint8_t data;
	TM_I2C_Read(I2C1, FT6x06_SLAVE_ADDR, reg, &data);
	return data;
}

static uint16_t read_word(GMouse* m, uint8_t reg) {
	(void)m;
  
	uint8_t data[2];
	TM_I2C_ReadMulti(I2C1, FT6x06_SLAVE_ADDR, reg, data, 2);
	return (data[0]<<8 | data[1]);
}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
