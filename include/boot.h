#ifndef _Boot_H_
#define _Boot_H_

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
#include "jpeglib.h"


unsigned int cromwell_config;
unsigned int cromwell_retryload;
unsigned int cromwell_loadbank;
unsigned int cromwell_Biostype;
unsigned int xbox_ram;


#define XROMWELL	0
#define CROMWELL	1

#define ICON_WIDTH 64
#define ICON_HEIGH 64

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

#define INCLUDE_FILTROR 0

// enable logging to serial port.  You probably don't have this.

#define INCLUDE_SERIAL 0

// enable trace message printing for debugging - with filtror or serial only
#define PRINT_TRACE 0
#if PRINT_TRACE
#define TRACE bprintf(__FILE__ " :%d\n\r",__LINE__);
#else
#define TRACE
#endif

#define INITRD_POS 0x02000000

/////////////////////////////////
// some typedefs to make for easy sizing

typedef unsigned long ULONG;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#ifndef bool_already_defined_
	typedef int bool;
#endif
typedef unsigned long RGBA; // LSB=R -> MSB = A
typedef long long __int64;

#define guint int
#define guint8 unsigned char

#define true 1
#define false 0

#ifndef NULL
#define NULL ((void *)0)
#endif

#define ASSERT(exp) { if(!(exp)) { bprintf("Assert failed file " __FILE__ " line %d\n", __LINE__); } }

#include "BootFilesystemIso9660.h"


extern void WATCHDOG(void);

#include "BootVideo.h"

extern volatile CURRENT_VIDEO_MODE_DETAILS currentvideomodedetails;

volatile DWORD VIDEO_CURSOR_POSX;
volatile DWORD VIDEO_CURSOR_POSY;
volatile DWORD VIDEO_ATTR;
volatile DWORD VIDEO_LUMASCALING;
volatile DWORD VIDEO_RSCALING;
volatile DWORD VIDEO_BSCALING;
volatile DWORD VIDEO_VSYNC_COUNT;
volatile DWORD BIOS_TICK_COUNT;
volatile DWORD VIDEO_VSYNC_POSITION;
volatile DWORD VIDEO_VSYNC_DIR;
volatile DWORD DVD_TRAY_STATE;

BYTE VIDEO_AV_MODE ;

#define DVD_CLOSED 		0
#define DVD_CLOSING 		1
#define DVD_OPEN   		2
#define DVD_OPENING   		3

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

typedef enum {
	EDT_UNKNOWN= 0,
	EDT_XBOXFS
} enumDriveType;

typedef struct tsHarddiskInfo {  // this is the retained knowledge about an IDE device after init
    unsigned short m_fwPortBase;
    unsigned short m_wCountHeads;
    unsigned short m_wCountCylinders;
    unsigned short m_wCountSectorsPerTrack;
    unsigned long m_dwCountSectorsTotal; /* total */
    unsigned char m_bLbaMode;	/* am i lba (0x40) or chs (0x00) */
    unsigned char m_szIdentityModelNumber[40];
    unsigned char term_space_1[2];
    unsigned char m_szSerial[20];
    unsigned char term_space_2[2];
    char m_szFirmware[8];
    unsigned char term_space_3[2];
    unsigned char m_fDriveExists;
    unsigned char m_fAtapi;  // true if a CDROM, etc
    enumDriveType m_enumDriveType;
    unsigned char m_bCableConductors;  // valid for device 0 if present
    unsigned short m_wAtaRevisionSupported;
    unsigned char s_length;
    unsigned char m_length;
    unsigned char m_fHasMbr;
} tsHarddiskInfo;


/* a retail Xbox has 64 MB of RAM */
#define RAMSIZE (64 * 1024*1024)
/* the size of the framebuffer (defaults to 4 MB) */
#define FRAMEBUFFER_SIZE 0x00400000
/* the start of the framebuffer */
#define FRAMEBUFFER_START (0xf0000000 | (RAMSIZE - FRAMEBUFFER_SIZE))
/* the protected mode part of the kernel has to reside at 1 MB in RAM */
#define PM_KERNEL_DEST 0x100000
/* parameters for the kernel have to be here */
#define KERNEL_SETUP   0x90000
/* the GDT must not be overwritten, so we place it into an unused
   area within KERNEL_SETUP */
#define GDT_LOC (KERNEL_SETUP+0x800)
/* same with the command line */
#define CMD_LINE_LOC (KERNEL_SETUP+0x1000)

/* let's reserve 4 MB at the top for the framebuffer */
#define RAMSIZE_USE (RAMSIZE - FRAMEBUFFER_SIZE)
/* the initrd resides at 1 MB from the top of RAM */
#define INITRD_DEST (RAMSIZE_USE - 1024*1024)

//#define LPCFlashadress 0xFFF00000
#define LPCFlashadress 0xFF000000

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


void BootPciInterruptEnable(void);

	// boot process
int BootPerformPicChallengeResponseAction(void);
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
	int BootFiltrorGetIncomingMessageLength(void);
	void BootFiltrorMarkIncomingMessageAsHavingBeenRead(void) ;
	bool BootFiltrorDoesPcHaveAMessageWaitingForItToRead(void);
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
#if INCLUDE_SERIAL
#define bprintf serialprint
#else
#define bprintf(...)
#endif
#endif

#if INCLUDE_SERIAL
	int serialprint(const char *szFormat, ...);
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


typedef struct _LIST_ENTRY {
	struct _LIST_ENTRY *m_plistentryNext;
	struct _LIST_ENTRY *m_plistentryPrevious;
} LIST_ENTRY;

void ListEntryInsertAfterCurrent(LIST_ENTRY *plistentryCurrent, LIST_ENTRY *plistentryNew);
void ListEntryRemove(LIST_ENTRY *plistentryCurrent);

////////// BootPerformXCodeActions.c

int BootPerformXCodeActions(void);

#include "BootEEPROM.h"
#include "BootParser.h"

////////// BootStartBios.c

void StartBios(CONFIGENTRY *config,int nActivePartition, int nFATXPresent,int bootfrom);
int BootMenue(CONFIGENTRY *config,int nDrive,int nActivePartition, int nFATXPresent);

////////// BootResetActions.c
void ClearIDT (void);
void BootResetAction(void);
void BootCpuCache(bool fEnable) ;
int printk(const char *szFormat, ...);
void BiosCmosWrite(BYTE bAds, BYTE bData);
BYTE BiosCmosRead(BYTE bAds);


///////// BootPciPeripheralInitialization.c
void BootPciPeripheralInitialization(void);
void BootAGPBUSInitialization(void);
void BootDetectMemorySize(void);
extern void	ReadPCIByte(unsigned int bus, unsigned int dev, unsigned intfunc, 	unsigned int reg_off, unsigned char *pbyteval);
extern void	WritePCIByte(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned char byteval);
extern void	ReadPCIDword(unsigned int bus, unsigned int dev, unsigned int func,	unsigned int reg_off, unsigned int *pdwordval);
extern void	WritePCIDword(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned int dwordval);
extern void	ReadPCIBlock(unsigned int bus, unsigned int dev, unsigned int func,		unsigned int reg_off, unsigned char *buf,	unsigned int nbytes);
extern void	WritePCIBlock(unsigned int bus, unsigned int dev, unsigned int func, 	unsigned int reg_off, unsigned char *buf, unsigned int nbytes);

void PciWriteByte (unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg_off, unsigned char byteval);
BYTE PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
DWORD PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, DWORD dw);
DWORD PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);

///////// BootPerformPicChallengeResponseAction.c

int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite);
int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite);
bool I2CGetTemperature(int *, int *);
void I2CModifyBits(BYTE bAds, BYTE bReg, BYTE bData, BYTE bMask);

///////// BootIde.c

extern tsHarddiskInfo tsaHarddiskInfo[];  // static struct stores data about attached drives
int BootIdeInit(void);
int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes);
int BootIdeBootSectorHddOrElTorito(int nDriveIndex, BYTE * pbaResult);
int BootIdeAtapiAdditionalSenseCode(int nDrive, BYTE * pba, int nLengthMaxReturn);
int BootIdeSetTransferMode(int nIndexDrive, int nMode);
int BootIdeWaitNotBusy(unsigned uIoBase);
bool BootIdeAtapiReportFriendlyError(int nDriveIndex, char * szErrorReturn, int nMaxLengthError);
void BootIdeAtapiPrintkFriendlyError(int nDriveIndex);

///////// BootUSB.c

void BootStopUSB(void);
void BootStartUSB(void);
void USBGetEvents(void);

#include "xpad.h"

extern struct xpad_data XPAD_current[4];
extern struct xpad_data XPAD_last[4];

///////// BootEthernet.c

int BootStartUpEthernet(void);

///////// BootAudio.c

typedef struct {
	WORD m_wTimeStartmS;
	WORD m_wTimeDurationmS;
	DWORD m_dwFrequency;
} SONG_NOTE;


typedef enum {
	AET_SINE=0,
	AET_NOISE
} AUDIO_ELEMENT_TYPES;

typedef void (*CALLBACK_AFTER_BLOCK)(void * pvoidAc97Device, void * pvoidaudioelement);


typedef struct _AUDIO_ELEMENT {  // represents a kind of generated sound, needs a derived object to be usable
	struct _AUDIO_ELEMENT * m_paudioelementNext;
	short m_sPanZeroIsAllLeft7FFFIsAllRight;
	AUDIO_ELEMENT_TYPES m_aetType;
	DWORD m_dwVolumeElementMaster7fff0000Max;
	DWORD m_dwVolumeAttackRate;
	DWORD m_dwVolumeAttackLimit;
	DWORD m_dwVolumeSustainRate;
	DWORD m_dwVolumeSustainLimit;
	DWORD m_dwVolumeDecayRate;
	BYTE m_bStageZeroIsAttack;
	void * m_pvPayload;
	DWORD m_dwPayload2;
	CALLBACK_AFTER_BLOCK m_callbackEveryBufferFill;
	DWORD m_dwCount48kHzSamplesRendered;
} AUDIO_ELEMENT;

typedef struct _AUDIO_ELEMENT_SINE {  // derived object composed from base object AUDIO_ELEMENT represents a sine wave with harmonics
	AUDIO_ELEMENT m_paudioelement;
	DWORD m_dwComputedFundamentalPhaseIncrementFor48kHzSamples;
	short m_saVolumePerHarmonicZeroIsNone7FFFIsFull[5]; // first entry is fundamental, then 2nd harmonic, etc
	DWORD m_dwPhaseAccumilator[5];
} AUDIO_ELEMENT_SINE;

typedef struct _AUDIO_ELEMENT_NOISE {  // derived object composed from base object AUDIO_ELEMENT represents a whitish noise source
	AUDIO_ELEMENT m_paudioelement;
	short m_sVolumeZeroIsNone7FFFIsFull;
	union {
		DWORD m_dwShifter;
		short m_saSamples[2];
	} shifter;
	short m_sLastSample;
} AUDIO_ELEMENT_NOISE;

typedef struct {  // descriptor from AC97 specification
	DWORD m_dwBufferStartAddress;
	WORD m_wBufferLengthInSamples;  // 0=no smaples
	WORD m_wBufferControl;  // b15=1=issue IRQ on completion, b14=1=last in stream
} AC97_DESCRIPTOR __attribute__ ((aligned (8)));


typedef struct {
	AC97_DESCRIPTOR m_aac97descriptorPcmSpdif[32];
	AC97_DESCRIPTOR m_aac97descriptorPcmOut[32];
	volatile DWORD * m_pdwMMIO;
	volatile DWORD m_dwCount48kHzSamplesRendered;
	volatile DWORD m_dwNextDescriptorMod31;
	AUDIO_ELEMENT * m_paudioelementFirst;
} AC97_DEVICE  __attribute__ ((aligned (8))) ;

void wait_ms(DWORD ticks);
void wait_smalldelay(void);
void BootAudioInit(volatile AC97_DEVICE * pac97device);
void BootAudioInterrupt(volatile AC97_DEVICE * pac97device);
void BootAudioSilence(volatile AC97_DEVICE * pac97device);
void BootAudioOutBufferToDescriptor(volatile AC97_DEVICE * pac97device, DWORD * pdwBuffer, WORD wLengthInSamples, bool fFinal);
short BootAudioInterpolatedSine(DWORD dwPhase4GIs2PiRadians);
void BootAudioPlayDescriptors(volatile AC97_DEVICE * pac97device);
void BootAudioAttachAudioElement(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT * paudioelement);
void BootAudioDetachAudioElement(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT * paudioelement);
void ConstructAUDIO_ELEMENT_SINE(volatile AUDIO_ELEMENT_SINE *, int nFrequencyFundamental);
void DestructAUDIO_ELEMENT_SINE(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT_SINE * paes);
void ConstructAUDIO_ELEMENT_NOISE(volatile AUDIO_ELEMENT_NOISE *);
void DestructAUDIO_ELEMENT_NOISE(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT_NOISE * paen);
WORD BootAudioGetMixerSetting(volatile AC97_DEVICE * pac97device, int nSettingIndex);
void BootAudioSetMixerSetting(volatile AC97_DEVICE * pac97device, int nSettingIndex, WORD wValue);


	// video helpers

void BootVideoBlit(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	DWORD * pdwTopLeftSource,
	DWORD dwCountBytesPerLineSource,
	DWORD dwCountLines
);

void BootVideoVignette(
	DWORD * pdwaTopLeftDestination,
	DWORD m_dwCountBytesPerLineDestination,
	DWORD m_dwCountLines,
	RGBA rgbaColour1,
	RGBA rgbaColour2,
	DWORD dwStartLine,
	DWORD dwEndLine
);


typedef struct {
	BYTE * m_pBitmapData;
	int m_nWidth;
	int m_nHeight;
	int m_nBytesPerPixel;
} JPEG;

int BootVideoOverlayString(DWORD * pdwaTopLeftDestination, DWORD m_dwCountBytesPerLineDestination, RGBA rgbaOpaqueness, const char * szString);
void BootVideoChunkedPrint(char * szBuffer, WORD wLength);
int VideoDumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable);
unsigned int BootVideoGetStringTotalWidth(const char * szc);
void BootVideoClearScreen(JPEG * pJpeg, int nStartLine, int nEndLine);
void BootVideoJpegBlitBlend(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	JPEG * pJpeg,
	DWORD * pdwTopLeftInJpegBitmap,
	RGBA m_rgbaTransparent,
	DWORD * pdwTopLeftBackground,
	DWORD dwCountBytesPerLineBackground,
	DWORD dwCountBytesPerPixelBackground,
	int x,
	int y
);

bool BootVideoJpegUnpackAsRgb(
	BYTE *pbaJpegFileImage,
	int nFileLength,
	JPEG * pJpeg
);

void BootVideoEnableOutput(BYTE bAvPack);
BYTE * BootVideoGetPointerToEffectiveJpegTopLeft(JPEG * pJpeg);

void * memcpy(void *dest, const void *src,  size_t size);
void * memset(void *dest, int data,  size_t size);
int _memcmp(const BYTE *pb, const BYTE *pb1, int n);
int _strncmp(const char *sz1, const char *sz2, int nMax);
char * strcpy(char *sz, const char *szc);
char * _strncpy(char *sz, const char *szc, int nLenMax);

void MemoryManagementInitialization(void * pvStartAddress, DWORD dwTotalMemoryAllocLength);
void * malloc(size_t size);
void free(void *);

void Sleep(int nMicroseconds);

extern volatile int nCountI2cinterrupts, nCountUnusedInterrupts, nCountUnusedInterruptsPic2, nCountInterruptsSmc, nCountInterruptsIde;
extern volatile bool fSeenPowerdown;
extern BYTE baBackdrop[60*72*4];
extern JPEG jpegBackdrop;
typedef enum {
	ETS_OPEN_OR_OPENING=0,
	ETS_CLOSING,
	ETS_CLOSED
} TRAY_STATE;
extern volatile TRAY_STATE traystate;


extern void BootInterruptsWriteIdt(void);

int copy_swap_trim(unsigned char *dst, unsigned char *src, int len);
void HMAC_SHA1( unsigned char *result,
                unsigned char *key, int key_length,
                unsigned char *text1, int text1_length,
                unsigned char *text2, int text2_length );

char *HelpGetToken(char *ptr,char token);
void HelpGetParm(char *szBuffer, char *szOrig);
char *strrchr0(char *string, char ch);

#endif // _Boot_H_
