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

/////////////////////////////////
// configuration


#include <stdarg.h>
#include <stdio.h>
#include "consts.h"
#include "jpeg-6b/jpeglib.h"

// filtror is a debugging device designed to make code available over LPC and allow a debug shell
// details are at http://warmcat.com/milksop
// if you don't have one, or are building a final ROM image, keep this at zero

#define INCLUDE_FILTROR 0


/////////////////////////////////
// some typedefs to make for easy sizing

  typedef unsigned int DWORD;
  typedef unsigned short WORD;
  typedef unsigned char BYTE;
  typedef int bool;
	typedef unsigned long RGBA; // LSB=R -> MSB = A
	typedef long long __int64;

#define guint int
#define guint8 unsigned char

#define true 1
#define false 0

#ifndef NULL
#define NULL ((void *)0)
#endif

#define WATCHDOG __asm__ __volatile__ ( "push %eax; push %edx; movw $0x80cf, %dx ; mov $5, %al ; out %al, %dx ; mov $10000, %eax ; 0: dec %eax ; cmp $0, %eax ; jnz 0b ; mov $4, %al ; out %al, %dx ; pop %edx ; pop %eax " );

#define FRAMEBUFFER_START ( 0xf0000000 + /*(0x04000000-(640*480*4) -(640*4*4)-(256*4))*/ (*((DWORD *)0xfd600800)) )

#define VIDEO_CURSOR_POSX (*((DWORD *)0x430))
#define VIDEO_CURSOR_POSY (*((DWORD *)0x434))
#define VIDEO_ATTR (*((DWORD *)0x438))
#define VIDEO_LUMASCALING (*((DWORD *)0x43c))
#define VIDEO_RSCALING (*((DWORD *)0x440))
#define VIDEO_BSCALING (*((DWORD *)0x444))
#define MALLOC_BASE (*((DWORD *)0x448))
#define VIDEO_HEIGHT (*((DWORD *)0x44c))
#define VIDEO_MARGINX (*((DWORD *)0x450))
#define VIDEO_MARGINY (*((DWORD *)0x454))

/////////////////////////////////
// Superfunky i386 internal structures

typedef struct gdt_t {
        unsigned short       m_wSize __attribute__ ((packed));
        unsigned long m_dwBase32 __attribute__ ((packed));
        unsigned short       m_wDummy __attribute__ ((packed));
} ts_descriptor_pointer;

typedef struct {  // inside an 8-byte protected mode interrupt vector
	WORD m_wHandlerHighAddressLow16;
	WORD m_wSelector;
	WORD m_wType;
	WORD m_wHandlerLinearAddressHigh16;
} ts_pm_interrupt;

/* the protected mode part of the kernel has to reside at 1 MB in RAM */
#define PM_KERNEL_DEST 0x100000
/* parameters for the kernel have to be here */
#define KERNEL_SETUP   0x90000
/* the GDT must not be overwritten, so we place it into an unused
   area within KERNEL_SETUP */
#define GDT_LOC (KERNEL_SETUP+0x800)
/* same with the command line */
#define CMD_LINE_LOC (KERNEL_SETUP+0x1000)

/* a retail Xbox has 64 MB of RAM */
#define RAMSIZE (64 * 1024*1024)
/* let's reserve 1 MB at the top for the framebuffer */
#define RAMSIZE_USE (RAMSIZE - 4096*1024)
/* the initrd resides at 1 MB from the top of RAM */
#define INITRD_DEST (RAMSIZE_USE - 1024*1024)


#define PDW_BIOS_TICK_PTR ((DWORD *)0x46c)

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

/////////////////////////////////
// some Boot API prototypes

//////// BootPerformPicChallengeResponseAction.c

/* ----------------------------  IO primitives -----------------------------------------------------------
*/

static __inline void IoOutputByte(WORD wAds, BYTE bValue) {
//	__asm__  ("			     out	%%al,%%dx" : : "edx" (dwAds), "al" (bValue)  );
    __asm__ __volatile__ ("outb %b0,%w1": :"a" (bValue), "Nd" (wAds));
}

static __inline void IoOutputWord(WORD wAds, WORD wValue) {
//	__asm__  ("	 out	%%ax,%%dx	" : : "edx" (dwAds), "ax" (wValue)  );
    __asm__ __volatile__ ("outw %0,%w1": :"a" (wValue), "Nd" (wAds));
	}

static __inline void IoOutputDword(WORD wAds, DWORD dwValue) {
//	__asm__  ("	 out	%%eax,%%dx	" : : "edx" (dwAds), "ax" (wValue)  );
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

#define BootPciInterruptGlobalStackStateAndDisable() {	__asm__ __volatile__ (  "pushf; cli" ); }
#define BootPciInterruptGlobalPopState()  {	__asm__ __volatile__  (  "popf" ); }

	// main I2C traffic functions
int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite);
int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite, bool fMode);
//#define I2CTransmit2Bytes(bPicAddressI2cFormat, bCommand, bData) I2CTransmitWord(bPicAddressI2cFormat, (((WORD)bCommand)<<8)|(WORD)bData, false)

	// boot process
int BootPerformPicChallengeResponseAction();
	// LED control (see associated enum above)
int I2cSetFrontpanelLed(BYTE b);

//////// filtror.c

#if INCLUDE_FILTROR

	typedef struct {
		DWORD m_dwBlocksFromPc;
		DWORD m_dwCountChecksumErrorsSeenFromPc;
		DWORD m_dwBlocksToPc;
		DWORD m_dwCountTimeoutErrorsSeenToPc; // this member should be incremented by the higher-level protocol when expected response does not happen
	} BOOTFILTROR_CHANNEL_QUALITY_STATS;

	extern BOOTFILTROR_CHANNEL_QUALITY_STATS bfcqs;

		// helpers
	int BootFiltrorGetIncomingMessageLength();
	void BootFiltrorMarkIncomingMessageAsHavingBeenRead() ;
	bool BootFiltrorDoesPcHaveAMessageWaitingForItToRead();
	void BootFiltrorSetMessageLengthForPcToRead(WORD wChecksum, WORD wLength) ;
	int DumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable);
		// main functions
	int BootFiltrorSendArrayToPc(const BYTE * pba, WORD wLength);
	int BootFiltrorGetArrayFromPc( BYTE * pba, WORD wLengthMax);
	int BootFiltrorSendArrayToPcModal(const BYTE * pba, WORD wLength);
	int BootFiltrorSendStringToPcModal(const char *szFormat, ...);
		// alias
#define bprintf BootFiltrorSendStringToPcModal
#else
#define bprintf(...)
#endif

#define SPAMI2C() 				__asm__ __volatile__ (\
	"retry: ; "\
	"movb $0x55, %al ;"\
	"movl $0xc004, %edx ;"\
	"shl	$1, %al ;"\
	"out %al, %dx ;"\
\
	"movb $0xaa, %al ;"\
	"add $4, %edx ;"\
	"out %al, %dx ;"\
\
	"movb $0xbb, %al ;"\
	"sub $0x2, %edx ;"\
	"out %al, %dx ;"\
\
	"sub $6, %edx ;"\
	"in %dx, %ax ;"\
	"out %ax, %dx ;"\
	"add $2, %edx ;"\
	"movb $0xa, %al ;"\
	"out %al, %dx ;"\
	"sub $0x2, %dx ;"\
"spin: ;"\
	"in %dx, %al ;"\
	"test $8, %al ;"\
	"jnz spin ;"\
	"test $8, %al ;"\
	"jnz retry ;"\
	"test $0x24, %al ;"\
\
	"jmp retry"\
);


////////// BootPerformXCodeActions.c

int BootPerformXCodeActions();

////////// BootStartBios.c

void StartBios();

////////// BootResetActions.c

void BootResetAction(void);
void BootCpuCache(bool fEnable) ;
int printk(const char *szFormat, ...);
void BiosCmosWrite(BYTE bAds, BYTE bData);
BYTE BiosCmosRead(BYTE bAds);

////////// BootPciPeripheralInitialization.c

void BootPciPeripheralInitialization();
extern void	ReadPCIByte(unsigned int bus, unsigned int dev, unsigned intfunc, 	unsigned int reg_off, unsigned char *pbyteval);
extern void	WritePCIByte(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned char byteval);
extern void	ReadPCIDword(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned int *pdwordval);
extern void	WritePCIDword(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned int dwordval);
extern void	ReadPCIBlock(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned char *buf,	unsigned int nbytes);
extern void	WritePCIBlock(unsigned int bus, unsigned int dev, unsigned int func, 	unsigned int reg_off, unsigned char *buf, unsigned int nbytes);

///////// BootPerformPicChallengeResponseAction.c

int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite, bool fMode);
int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite);

///////// BootVgaInitialization.c

void BootVgaInitialization() ;
BYTE BootVgaInitializationKernel(int nLinesPref);  // returns AV pack index, call with 480 or 576

///////// BootIde.c

int BootIdeInit(void);
int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes);
int BootIdeBootSectorHddOrElTorito(int nDriveIndex, BYTE * pbaResult);
int BootIdeAtapiAdditionalSenseCode(int nDrive, BYTE * pba, int nLengthMaxReturn);
BYTE BootIdeGetTrayState();
int BootIdeSetTransferMode(int nIndexDrive, int nMode);

	// video helpers

void BootVideoBlit(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	DWORD * pdwTopLeftSource,
	DWORD dwCountBytesPerLineSource,
	DWORD dwCountLines
);

void BootGimpVideoBlitBlend(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	void * pgimpstruct,
	RGBA m_rgbaTransparent,
	DWORD * pdwTopLeftBackground,
	DWORD dwCountBytesPerLineBackground
);

void BootGimpVideoBlit(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	void * pgimpstruct,
	RGBA m_rgbaTransparent
);

void BootVideoVignette(
	DWORD * pdwaTopLeftDestination,
	DWORD m_dwCountBytesPerLineDestination,
	DWORD m_dwCountLines,
	RGBA rgbaColour1,
	RGBA rgbaColour2
);

#define VIDEO_CURSOR_POSX (*((DWORD *)0x430))
#define VIDEO_CURSOR_POSY (*((DWORD *)0x434))
#define VIDEO_ATTR (*((DWORD *)0x438))
#define VIDEO_LUMASCALING (*((DWORD *)0x43c))
#define VIDEO_RSCALING (*((DWORD *)0x440))
#define VIDEO_BSCALING (*((DWORD *)0x444))


int BootVideoOverlayString(DWORD * pdwaTopLeftDestination, DWORD m_dwCountBytesPerLineDestination, RGBA rgbaOpaqueness, const char * szString);
void BootVideoChunkedPrint(char * szBuffer, WORD wLength);
int VideoDumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable);
unsigned int BootVideoGetStringTotalWidth(const char * szc);

BYTE * BootVideoJpegUnpackAsRgb(
	BYTE *pbaJpegFileImage,
	int nFileLength,
	int *nWidth,
	int *nHeight,
	int *nBytesPerPixel
);

void BootVideoEnableOutput(BYTE bAvPack);


void * memcpy(void *dest, const void *src,  size_t size);
void * memset(void *dest, int data,  size_t size);

void * malloc(size_t size);
void free(void *);
