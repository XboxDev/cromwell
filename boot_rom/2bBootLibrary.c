
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/
// 20040924 - Updated by dmp to include more str functions, and use ASM
// where possible. ASM shamelessly stolen from linux-2.6.8.1

#include "stdint.h"

void * memcpy(void * to, const void * from, size_t n)
{
	int d0, d1, d2;
	__asm__ __volatile__(
       		"rep ; movsl\n\t"
       		"testb $2,%b4\n\t"
      		"je 1f\n\t"
      	 	"movsw\n"
      		"1:\ttestb $1,%b4\n\t"
     		"je 2f\n\t"
     		"movsb\n"
     	  	"2:"
     		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		:"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
	       	: "memory");
	return (to);
}

void * memset(void *s, int c,  size_t count)
{
  	int d0, d1;
	__asm__ __volatile__(
	        "rep\n\t"
	        "stosb"
	        : "=&c" (d0), "=&D" (d1)
	        :"a" (c),"1" (s),"0" (count)
	        :"memory");
	return s;
}
                

int memcmp(const void * cs,const void * ct,size_t count)
{
        const unsigned char *su1, *su2;
        int res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0) break;
	return res;
}

