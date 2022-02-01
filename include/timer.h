#ifndef __TIMER_H__
#define __TIMER_H__

#include "cromwell_types.h"

void wait_ms(u32 ticks);
void wait_us(u32 ticks);
void wait_smalldelay(void);
u32 GetTimerTicks(void);

#endif
