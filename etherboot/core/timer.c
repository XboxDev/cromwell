/* A couple of routines to implement a low-overhead timer for drivers */

 /*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include	"etherboot.h"
#include	"timer.h"

/* Machine Independant timer helper functions */

void mdelay(unsigned int msecs)
{
	unsigned int i;
	for(i = 0; i < msecs; i++) {
		udelay(1000);
		poll_interruptions();
	}
}
