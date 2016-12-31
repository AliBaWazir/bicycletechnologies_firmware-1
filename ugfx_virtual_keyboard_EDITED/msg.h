#ifndef _MSG_H_
#define _MSG_H_

//#define DEBUG

#define GET_SPEED_MSG 		0x01
#define GET_CADENCE_MSG 	0x02
#define GET_DISTANCE_MSG	0x03
#define GET_HEARTRATE_MSG	0x04
#define GET_BATTERY_MSG		0x05

typedef struct {
  uint8_t msg_ID;
	uint8_t value;
} message_t;

extern osPoolId mpool;
extern osMessageQId  spiQueue;
extern osMessageQId  guiQueue;

#endif /* _MSG_H_ */
