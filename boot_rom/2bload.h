/***************************************************************************
      Includes used by XBox boot code
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#include "2bconsts.h"


static inline double min (double a, double b)
{
	if (a < b) return a; else return b;
}

static inline double max (double a, double b)
{
	if (a > b) return a; else return b;
}

// filtror is a debugging device designed to make code available over LPC and allow a debug shell
// details are at http://warmcat.com/milksop
// if you don't have one, or are building a final ROM image, keep this at zero


/////////////////////////////////
// some typedefs to make for easy sizing

typedef unsigned long ULONG;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int bool;
typedef unsigned int size_t;

typedef unsigned long RGBA; // LSB=R -> MSB = A
typedef long long __int64;

#define guint int
#define guint8 unsigned char

#define true 1
#define false 0

#ifndef NULL
#define NULL ((void *)0)
#endif




/////////////////////////////////
// LED-flashing codes
// or these together as argument to I2cSetFrontpanelLed

enum {
	I2C_LED_RED0 = 0x80,
	I2C_LED_RED1 = 0x40,
	I2C_LED_RED2 = 0x20,
	I2C_LED_RED3 = 0x10,
	I2C_LED_GREEN0 = 0x08,
	I2C_LED_GREEN1 = 0x04,
	I2C_LED_GREEN2 = 0x02,
	I2C_LED_GREEN3 = 0x01
};

///////////////////////////////
/* BIOS-wide error codes		all have b31 set  */

enum {
	ERR_SUCCESS = 0,  // completed without error

	ERR_I2C_ERROR_TIMEOUT = 0x80000001,  // I2C action failed because it did not complete in a reasonable time
	ERR_I2C_ERROR_BUS = 0x80000002, // I2C action failed due to non retryable bus error

	ERR_BOOT_PIC_ALG_BROKEN = 0x80000101 // PIC algorithm did not pass its self-test
};

//////// BootPerformPicChallengeResponseAction.c

/* ----------------------------  IO primitives -----------------------------------------------------------
*/

static __inline void IoOutputByte(WORD wAds, BYTE bValue) {
    __asm__ __volatile__ ("outb %b0,%w1": :"a" (bValue), "Nd" (wAds));
}

static __inline void IoOutputWord(WORD wAds, WORD wValue) {
    __asm__ __volatile__ ("outw %0,%w1": :"a" (wValue), "Nd" (wAds));
	}

static __inline void IoOutputDword(WORD wAds, DWORD dwValue) {
    __asm__ __volatile__ ("outl %0,%w1": :"a" (dwValue), "Nd" (wAds));
}


static __inline BYTE IoInputByte(WORD wAds) {
  unsigned char _v;
  __asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

static __inline WORD IoInputWord(WORD wAds) {
  WORD _v;
  __asm__ __volatile__ ("inw %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

static __inline DWORD IoInputDword(WORD wAds) {
  DWORD _v;
  __asm__ __volatile__ ("inl %w1,%0":"=a" (_v):"Nd" (wAds));
  return _v;
}

#define rdmsr(msr,val1,val2) \
       __asm__ __volatile__("rdmsr" \
			    : "=a" (val1), "=d" (val2) \
			    : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))


	// boot process
int BootPerformPicChallengeResponseAction(void);
	// LED control (see associated enum above)
int I2cSetFrontpanelLed(BYTE b);


////////// BootResetActions.c

void BootStartBiosLoader(void);



///////// BootPerformPicChallengeResponseAction.c

int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite);
int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite);
bool I2CGetTemperature(int *, int *);
void I2CModifyBits(BYTE bAds, BYTE bReg, BYTE bData, BYTE bMask);



void * memcpy(void *dest, const void *src,  size_t size);
void * memset(void *dest, int data,  size_t size);
int _memcmp(const BYTE *pb, const BYTE *pb1, int n);
int _strncmp(const char *sz1, const char *sz2, int nMax);
char * strcpy(char *sz, const char *szc);
char * _strncpy(char *sz, const char *szc, int nLenMax);

unsigned int free_mem_ptr;
void *malloc(size_t size);
void free(void *where);

unsigned char *BufferIN;
int BufferINlen;
unsigned char *BufferOUT;
int BufferOUTPos;
void compressinit(void);
void Decode(void);
