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

#include "consts.h"

// filtror is a debugging device designed to make code available over LPC and allow a debug shell
// details are at http://warmcat.com/milksop
// if you don't have one, or are building a final ROM image, keep this at zero

#define INCLUDE_FILTROR 1

/////////////////////////////////
// some typedefs to make for easy sizing

  typedef unsigned int DWORD;
  typedef unsigned short WORD;
  typedef unsigned char BYTE;
  typedef int bool;
	typedef unsigned long RGBA; // LSB=R -> MSB = A
	//typedef unsigned int size_t;

#define true 1
#define false 0

#define NULL ((void *)0)

/////////////////////////////////
// Superfunky i386 internal structures

typedef struct gdt_t {
        unsigned short       m_wSize __attribute__ ((packed));
        unsigned long m_dwBase32 __attribute__ ((packed));
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
#define RAMSIZE_USE (RAMSIZE - 1024*1024)
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
//static int I2CTransmit2Bytes(BYTE bPicAddressI2cFormat, BYTE bCommand, BYTE bData);
#define I2CTransmit2Bytes(bPicAddressI2cFormat, bCommand, bData) I2CTransmitWord(bPicAddressI2cFormat, (((WORD)bCommand)<<8)|(WORD)bData, false)
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

////////// BootPerformXCodeActions.c

int BootPerformXCodeActions();

////////// BootLinux.c

void BootLinux();

////////// BootResetActions.c

void BootResetAction(void);
void BootCpuCache(bool fEnable) ;
int printk(const char *szFormat, ...);

////////// BootPciPeripheralInitialization.c

void BootPciPeripheralInitialization();
extern void	ReadPCIByte(unsigned int bus, unsigned int dev, unsigned intfunc, 	unsigned int reg_off, unsigned char *pbyteval);
extern void	WritePCIByte(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned char byteval);
extern void	ReadPCIDword(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned int *pdwordval);
extern void	WritePCIDword(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned int dwordval);
extern void	ReadPCIBlock(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned char *buf,	unsigned int nbytes);
extern void	WritePCIBlock(unsigned int bus, unsigned int dev, unsigned int func, 	unsigned int reg_off, unsigned char *buf, unsigned int nbytes);

///////// BootVgaInitialization.c

void BootVgaInitialization() ;

///////// BootIde.c

int BootIdeInit(void);
int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes);
int BootIdeBootSectorHddOrElTorito(int nDriveIndex, BYTE * pbaResult);
int BootIdeAtapiAdditionalSenseCode(int nDrive, BYTE * pba, int nLengthMaxReturn);
