#ifndef _TRACE_H_
#define _TRACE_H_

#include "gfx.h"
#include "tm_stm32_rtc.h"

#define DEBUG

extern GFILE *myfile;

extern osMutexId traceMutex;

void closeTraceFile(void);
void openTraceFile(void);
void TRACE(const char *fmt, ...);

TM_RTC_Result_t updateRTC(TM_RTC_t* data, TM_RTC_Format_t format);
TM_RTC_Result_t getRTC(TM_RTC_t* data, TM_RTC_Format_t format);

#endif /* _TRACE_H_ */
