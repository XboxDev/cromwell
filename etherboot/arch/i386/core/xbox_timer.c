#include "boot.h"

unsigned int currticks(void)
{
	return IoInputDword(0x8008);
}

void ndelay(unsigned int nsecs)
{
	wait_us(nsecs * 1000);
}
void udelay(unsigned int usecs)
{
	wait_us(usecs);
}
