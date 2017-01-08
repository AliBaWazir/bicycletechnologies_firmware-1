#ifndef _MSG_H_
#define _MSG_H_

//#define DEBUG

#define INVALID_DATA 0xFF

#define GET_AVAILABILITY_MSG			0xDA
// Group 0: Shifting Algorithm Parameters
#define GET_SPEED_MSG 						0x01
#define GET_CADENCE_MSG 					0x02
#define GET_DISTANCE_MSG					0x03
#define GET_HEARTRATE_MSG					0x04
#define GET_CADENCE_SETPOINT_MSG 	0x05
#define GET_BATTERY_MSG						0x06

#define SET_CADENCE_SETPOINT_MSG	0x08
#define LOCK_GEAR_LEVEL_MSG				0x09

// Group 1:Bike Configuration
#define GET_WHEEL_DIAMETER_MSG		0x10
#define GET_GEAR_COUNT_MSG				0x11
#define GET_TEETH_COUNT_MSG				0x12

#define SET_WHEEL_DIAMETER_MSG		0x18
#define	SET_GEAR_COUNT_MSG				0x19
#define SET_TEETH_COUNT_MSG				0x1A

// Group 2: Bluetooth Device Configuration


typedef struct {
  uint8_t msg_ID;
	uint8_t value;
	uint8_t frontGears[MAXIMUM_FRONT_GEARS+1];
	uint8_t backGears[MAXIMUM_BACK_GEARS+1];
} message_t;

extern osPoolId mpool;
extern osMessageQId  spiQueue;
extern osMessageQId  guiQueue;

#endif /* _MSG_H_ */
