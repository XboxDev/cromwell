/*
 * Sequences the necessary post-reset actions from as soon as we are able to run C
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-02-25 andy@warmcat.com  1024x576, and RGB AV cable support
	2003-01-13 andy@warmcat.com  Moved out interrupt stuff into BootInterrupts.c
	                             Dropped hardcoded GIMP tux, moved to icons stored
	                             after line 480 of the backdrop JPG
															 Moved out helper functions to BootLibrary.c
	2002-12-18 andy@warmcat.com  Added ISO9660 boot if no HDD MBR
	2002-11-25 andy@warmcat.com  Added memory size code in CMOS, LILO won't boot without it
	                              tidied
	2002-09-19 andy@warmcat.com  Added code to manage standard CMOS settings for Bochs
	                              Defeated call to VGA init for now
 */

#include "boot.h"
#include "BootEEPROM.h"
#include "BootFlash.h"
#include "BootFATX.h"
#include "xbox.h"

#ifdef XBE
#include "config-xbe.h"
#else
#include "config-rom.h"
#endif
#include "BootUsbOhci.h"



extern DWORD dwaTitleArea[1024*64];
JPEG jpegBackdrop;

int nTempCursorMbrX, nTempCursorMbrY;

volatile CURRENT_VIDEO_MODE_DETAILS currentvideomodedetails;
volatile ohci_t usbcontroller[2];

volatile AC97_DEVICE ac97device;

volatile AUDIO_ELEMENT_SINE aesTux;
volatile AUDIO_ELEMENT_SINE aesSong;
volatile AUDIO_ELEMENT_NOISE aenTux;

const KNOWN_FLASH_TYPE aknownflashtypesDefault[] = {
	{ 0xbf, 0x61, "SST49LF020", 0x40000 },  // default flash types
	{ 0x01, 0xd5, "Am29F080B", 0x100000 },  // default flash types
	{ 0x04, 0xd5, "Fujitsu MBM29F080A", 0x100000 },  // default flash types
	{ 0xad, 0xd5, "Hynix HY29F080", 0x100000 },  // default flash types
	{ 0x20, 0xf1, "ST M29F080A", 0x100000 },  // default flash types
	{ 0x89, 0xA6, "Sharp LHF08CH1", 0x100000 }, // default flash types
	{ 0, 0, "\0", 0 }
};


const SONG_NOTE songnoteaIntro[] = {
	{  370, 200, 207 },
	{  730, 200, 207 },
	{ 1080, 200, 207 },
	{ 1460, 200, 184 },
	{ 1630, 200, 207 },
	{ 2000, 200, 207 },
	{ 2150, 200, 207 },
	{ 2520, 200, 207 },
	{ 2680, 200, 184 },
	{ 2830, 200, 207 },
	{ 3190, 200, 207 },
	{ 3550, 200, 184 },
	{ 3720, 200, 207 },
	{ 3900, 200, 207 },
	{ 4250, 200, 207 },
	{ 4400, 200, 207 },
	{ 4760, 200, 184 },
	{ 4900, 200, 207 },
	{ 5260, 200, 207 },
	{ 6000, 0, 0 }
};

/*
Hints:

1) We are in a funny environment, executing out of ROM.  That means it is not possible to
 define filescope non-const variables with an initializer.  If you do so, the linker will oblige and you will end up with
 a 4MByte image instead of a 1MByte one, the linker has added the RAM init at 400000.
*/


// access to RTC CMOS memory

void BiosCmosWrite(BYTE bAds, BYTE bData) {
		IoOutputByte(0x72, bAds);
		IoOutputByte(0x73, bData);
}

BYTE BiosCmosRead(BYTE bAds)
{
		IoOutputByte(0x72, bAds);
		return IoInputByte(0x73);
}



//////////////////////////////////////////////////////////////////////
//
//  BootResetAction()

extern void BootResetAction ( void ) {
//	__int64 i64Timestamp;
	bool fMbrPresent=false;
	bool fSeenActive=false;
	int nActivePartitionIndex=0;
	int nFATXPresent=false;
	int nTempCursorX, nTempCursorY;
       
        
#if INCLUDE_FILTROR
	// clear down channel quality stats
	bfcqs.m_dwBlocksFromPc=0;
	bfcqs.m_dwCountChecksumErrorsSeenFromPc=0;
	bfcqs.m_dwBlocksToPc=0;
	bfcqs.m_dwCountTimeoutErrorsSeenToPc=0;
#endif

		// prep our BIOS console print state

	VIDEO_ATTR=0xffffffff;

#if INCLUDE_FILTROR
	BootFiltrorSendArrayToPc("\nBOOT: starting BootResetAction()\n\r", 34);
#endif

	// init malloc() and free() structures

	MemoryManagementInitialization((void *)MEMORYMANAGERSTART, MEMORYMANAGERSIZE);
	

	bprintf("BOOT: starting PCI init\n\r");
	BootPciPeripheralInitialization();
	bprintf("BOOT: done with PCI initialization\n\r");

	BootInterruptsWriteIdt();	// Save Mode, not all fully Setup
	bprintf("BOOT: done interrupts\n\r");

	// if we don't make the PIC happy within 200mS, the damn thing will reset us

	BootPerformPicChallengeResponseAction();
	bprintf("BOOT: done with PIC challenge\n\r");

	memset((ohci_t *)&usbcontroller[0],0,sizeof(ohci_t));
	memset((ohci_t *)&usbcontroller[1],0,sizeof(ohci_t));

	// initialize the PCI devices

	
	BootEepromReadEntireEEPROM();
	bprintf("BOOT: Read EEPROM\n\r");
//	DumpAddressAndData(0, (BYTE *)&eeprom, 256);





	if(((BYTE *)&eeprom)[0x96]&0x01) { // 16:9 widescreen TV
		currentvideomodedetails.m_nVideoModeIndex=VIDEO_MODE_1024x576;
	} else { // 4:3 TV
		currentvideomodedetails.m_nVideoModeIndex=VIDEO_PREFERRED_MODE;
	}

	currentvideomodedetails.m_pbBaseAddressVideo=(BYTE *)0xfd000000;
	currentvideomodedetails.m_fForceEncoderLumaAndChromaToZeroInitially=1;
        
        // Load and Init the Background image

	BootVgaInitializationKernel((CURRENT_VIDEO_MODE_DETAILS *)&currentvideomodedetails);

	bprintf("BOOT: kern VGA init done\n\r");

	{ // decode and malloc backdrop bitmap
		extern int _start_backdrop, _end_backdrop;
		BootVideoJpegUnpackAsRgb(
			(BYTE *)&_start_backdrop,
			((DWORD)(&_end_backdrop)-(DWORD)(&_start_backdrop)),
			&jpegBackdrop
		);
	}
	bprintf("BOOT: backdrop unpacked\n\r");

		// paint the backdrop
	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
	bprintf("BOOT: done backdrop\n\r");

	

//#ifndef XBE
		
	{
	DWORD dw=0;
	BootPciInterruptGlobalStackStateAndDisable(&dw);
//	IoOutputByte(0x80c0, 0x08);  // from 2bl
	IoOutputByte(0x8004, IoInputByte(0x8004)|1);  // KERN: SCI enable == SCI interrupt generated
	IoOutputByte(0x8022, IoInputByte(0x8022)|2);  // KERN: Interrupt enable register, b1 RESERVED in AMD docs
	IoOutputByte(0x8002, IoInputByte(0x8002)|1);  // KERN: Enable SCI interrupt when timer status goes high
	IoOutputByte(0x8028, IoInputByte(0x8028)|1);  // KERN: setting readonly trap event???

	I2CTransmitWord(0x10, 0x0b00); // Allow audio
//	I2CTransmitWord(0x10, 0x0b01); // GAH!!!  Audio Mute!
	I2CTransmitWord(0x10, 0x1a01); // unknown, done immediately after reading out eeprom data
	I2CTransmitWord(0x10, 0x1b04); // unknown
	BootPciInterruptGlobalPopState(dw);
	}
//#endif





	// Audio Section
	
	BootAudioInit(&ac97device);
	ConstructAUDIO_ELEMENT_SINE(&aesTux, 1000);  // constructed silent, manipulated in video IRQ that moves tux
	BootAudioAttachAudioElement(&ac97device, (AUDIO_ELEMENT *)&aesTux);
	ConstructAUDIO_ELEMENT_SINE(&aesSong, 1000);  // constructed silent, manipulated in video IRQ that moves tux
	aesSong.m_paudioelement.m_pvPayload=(SONG_NOTE *)&songnoteaIntro[0];
	aesSong.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[0]=0x1fff;
	aesSong.m_paudioelement.m_bStageZeroIsAttack=3; // silenced initially until first note
	aesSong.m_paudioelement.m_bStageZeroIsAttack=3; // silenced initially until first note
	BootAudioAttachAudioElement(&ac97device, (AUDIO_ELEMENT *)&aesSong);
	ConstructAUDIO_ELEMENT_NOISE(&aenTux);  // constructed silent, manipulated in video IRQ that moves tux
	BootAudioAttachAudioElement(&ac97device, (AUDIO_ELEMENT *)&aenTux);
	BootAudioPlayDescriptors(&ac97device);


	

	// display solid red frontpanel LED while we start up
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );

	
        {
	DWORD dw=0;
	BootPciInterruptGlobalStackStateAndDisable(&dw);

	I2CTransmitWord(0x10, 0x1901); // no reset on eject

#ifdef IS_XBE_CDLOADER
	I2CTransmitWord(0x10, 0x0c01); // close DVD tray
#else
	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
#endif
	BootPciInterruptGlobalPopState(dw);
	}
     
         
	VIDEO_CURSOR_POSY=currentvideomodedetails.m_dwMarginYInLinesRecommended;
	VIDEO_CURSOR_POSX=(currentvideomodedetails.m_dwMarginXInPixelsRecommended/*+64*/)*4;
#ifdef XBE
	printk("\2Xbox Linux XROMWELL  " VERSION "\2\n" );
#else
	printk("\2Xbox Linux Clean BIOS  " VERSION "\2\n" );
#endif
	VIDEO_CURSOR_POSY=currentvideomodedetails.m_dwMarginYInLinesRecommended+32;
	VIDEO_CURSOR_POSX=(currentvideomodedetails.m_dwMarginXInPixelsRecommended/*+64*/)*4;
	printk( __DATE__ " -  http://xbox-linux.sf.net\n");
	VIDEO_CURSOR_POSX=(currentvideomodedetails.m_dwMarginXInPixelsRecommended/*+64*/)*4;
	printk("(C)2002-2003 Xbox Linux Team - Licensed under the GPL\n");
	printk("\n");

		// capture title area
	BootVideoBlit(&dwaTitleArea[0], currentvideomodedetails.m_dwWidthInPixels*4, ((DWORD *)FRAMEBUFFER_START)+(currentvideomodedetails.m_dwMarginYInLinesRecommended*currentvideomodedetails.m_dwWidthInPixels), currentvideomodedetails.m_dwWidthInPixels*4, 64);

	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=VIDEO_CURSOR_POSY;

	VIDEO_ATTR=0xffc8c8c8;
	printk("Xbox Rev: ");
	VIDEO_ATTR=0xffc8c800;
	if((PciReadDword(BUS_0, DEV_1, 0, 0x08)&0xff)<=0xb2) {
		printk("v1.0  ");
	} else {
		printk("v1.1  ");
//		printk("v1.1  %x", PciReadDword(BUS_0, DEV_1, 0, 0x08)&0xff);
	}


	BootPciInterruptEnable();

/*
	{
		DWORD dw1, dw2;
		rdmsr(0x2a, dw1, dw2);
//		printk("%x %x  ", dw1, dw2);

	}
*/
	{
		int n, nx;
		I2CGetTemperature(&n, &nx);
		VIDEO_ATTR=0xffc8c8c8;
		printk("CPU Temp: ");
		VIDEO_ATTR=0xffc8c800;
		printk("%doC  ", n);
		VIDEO_ATTR=0xffc8c8c8;
		printk("M/b Temp: ");
		VIDEO_ATTR=0xffc8c800;
		printk("%doC  ", nx);
	}


#ifdef REPORT_VIDEO_MODE

	VIDEO_ATTR=0xffc8c8c8;
	printk("AV: ");
	VIDEO_ATTR=0xffc8c800;

	switch(currentvideomodedetails.m_bAvPack) {
		case 0:
			printk("Scart");
			break;
		case 1:
			printk("HDTV");
			break;
		case 2:
			printk("VGA");
			break;
		case 3:
			printk("Type 3");
			break;
		case 4:
			printk("SVideo");
			break;
		case 5:
			printk("Type 5");
			break;
		case 6:
			printk("Composite");
			break;
	}

//	printk("  - Time=0x%08X/0x%08X", (DWORD)(i64Timestamp>>32), (DWORD)i64Timestamp);
	printk("\n");
#endif



		I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );

		VIDEO_ATTR=0xffffffff;

				// restore EEPROM to my original contents
#if 0
		{
			int nAds=0, n;
			static const BYTE baMyEEProm[] = {
				0x6D, 0xAB, 0x59, 0xA2, 0xB8, 0x82, 0x09, 0xAB, 0x21, 0x84, 0xB2, 0x50, 0x8A, 0x7F, 0x4F, 0x43,
				0x54, 0x01, 0x1E, 0x52, 0xD3, 0xB6, 0x3A, 0x5C, 0x32, 0xA6, 0x11, 0x28, 0x72, 0x07, 0xAE, 0x3C,
				0x36, 0xD4, 0x83, 0xFB, 0xE0, 0x29, 0xEE, 0xA8, 0x1C, 0x9D, 0x14, 0xEF, 0x44, 0x39, 0x65, 0x37,
				0xC3, 0xA3, 0x94, 0xF3, 0x32, 0x31, 0x36, 0x34, 0x30, 0x35, 0x33, 0x32, 0x31, 0x31, 0x30, 0x33,
				0x00, 0x50, 0xF2, 0x62, 0xDD, 0x3B, 0x00, 0x00, 0x41, 0x99, 0x81, 0xB5, 0xBF, 0xCC, 0x91, 0x7C,
				0xE9, 0xE5, 0x19, 0xF5, 0xDF, 0xE9, 0x31, 0xE8, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
				0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00
			};
/* my v1.1 EEPROM in case of crisis
00000000: 1D BF 03 AF 54 48 56 FF : 1E 1D 53 81 41 22 32 00    ....THV...S.A"2.
00000010: 83 AA AC F0 B6 63 5A 7F : 76 D9 93 08 34 98 80 53    .....cZ.v...4..S
00000020: D1 E5 BE 3D A8 3E 85 63 : 87 3A C6 B2 ED E3 52 00    ...=.>.c.:....R.
00000030: 1B B2 A0 B5 33 30 30 36 : 36 39 34 32 33 32 30 35    ....300669423205
00000040: 00 50 F2 82 7F 01 00 00 : B2 53 65 EB A0 37 E3 6C    .P.......Se..7.l
00000050: 79 01 F0 5B FB D0 1F 75 : 00 03 80 00 00 00 00 00    y..[...u........
00000060: A1 55 47 FC 00 00 00 00 : 47 4D 54 00 42 53 54 00    .UG.....GMT.BST.
00000070: 00 00 00 00 00 00 00 00 : 0A 05 00 02 03 05 00 01    ................
00000080: 00 00 00 00 00 00 00 00 : 00 00 00 00 C4 FF FF FF    ................
00000090: 01 00 00 00 00 00 10 00 : 02 00 00 00 00 00 00 00    ................
000000A0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000000B0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000000C0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000000D0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000000E0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000000F0: 00 00 00 00 00 00 00 00 : 00 00 00 00 40 00 00 00    ............@...

*/
			bprintf("Rewriting EEPROM, please wait...\n");

			while(nAds<0x100) {
				I2CTransmitWord( 0x54, (nAds<<8)|baMyEEProm[nAds], true);
				for(n=0;n<1000;n++) { IoInputByte(I2C_IO_BASE+0); }
				nAds++;
			}
	}
#endif


			// now do the main BIOS action
#ifdef DO_CMOS_BIOS_DATA
		{  			// standard BIOS settings
			int n;
			for(n=0x0e;n<0x40;n++) BiosCmosWrite(n, 0);

//			BiosCmosWrite(0x0e, 0); // say that CMOS, HDD and RTC are all valid
//			BiosCmosWrite(0x0f, 0); // say that we are coming up from a normal reset
//			BiosCmosWrite(0x10, 0); // no floppies on this thing
//			BiosCmosWrite(0x11, 0); // no standard BIOS options
//			BiosCmosWrite(0x12, 0); // no HDD yet
			BiosCmosWrite(0x14, 9); // misc equip list, VGA, display present, b0 fixed to 1
//			BiosCmosWrite(0x2d, 0); // misc BIOS settings
			BiosCmosWrite(0x30, 0x78); // 1K blocks of RAM, - first 1M
			BiosCmosWrite(0x31, 0xe6); // high part (set to 59MB)
			BiosCmosWrite(0x32, 20); // century
//			BiosCmosWrite(0x3d, 0); // boot method: default=0=do not attempt boot (set in BootIde.c)
		}
#else
		{
				BiosCmosWrite(0x32, 20); // century
		}
#endif
			// gggb while waiting for Ethernet & Hdd

		I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2);


#ifdef DO_ETHERNET
				// init Ethernet
		printk("Initializing Ethernet... ");
		{
			int n=BootStartUpEthernet();
			if(n) { printk("Error %d\n", n); } else { printk("OK\n"); }
		}
#endif

		// set Ethernet MAC address from EEPROM
	        
	        {
		volatile BYTE * pb=(BYTE *)0xfef000a8;  // Ethernet MMIO base + MAC register offset (<--thanks to Anders Gustafsson)
		int n;
		for(n=5;n>=0;n--) { *pb++=	eeprom.MACAddress[n]; } // send it in backwards, its reversed by the driver
	        }

		BootEepromPrintInfo();

		
#if 0
		{
		OBJECT_FLASH of;
		memset(&of,0x00,sizeof(of));
		of.m_pbMemoryMappedStartAddress=(BYTE *)0xff000000;
		BootFlashCopyCodeToRam();
		BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0]);
		VIDEO_ATTR=0xffc8c8c8;
		printk("Flash type: ");
		VIDEO_ATTR=0xffc8c800;
		printk("%s\n", of.m_szFlashDescription);
		}
#endif
	
	// init USB
#ifdef DO_USB
	
		printk("BOOT: start USB init\n");

		BootUsbInit((ohci_t *)&usbcontroller[0], "USB1", (void *)0xfed00000);
//		BootUsbInit((ohci_t *)&usbcontroller[1], "USB1", (void *)0xfed08000);
		printk("BOOT: done USB init\n");
	

		
		VIDEO_ATTR=0xffc8c8c8;
		printk("USB: ");
		VIDEO_ATTR=0xffc8c800;

//		BootUsbPrintStatus((USB_CONTROLLER_OBJECT *)&usbcontroller[0]);
//		BootUsbPrintStatus((USB_CONTROLLER_OBJECT *)&usbcontroller[1]);
//		BootUsbDump((USB_CONTROLLER_OBJECT *)&usbcontroller[0]);
//		BootUsbDump((USB_CONTROLLER_OBJECT *)&usbcontroller[1]);
		
#endif

			// init the HDD and DVD
		VIDEO_ATTR=0xffc8c8c8;
		printk("Initializing IDE Controller\n");

			// wait around for HDD to become ready

		
		{
		DWORD dw=BIOS_TICK_COUNT;
		BootIdeWaitNotBusy(0x1f0);
		while((BIOS_TICK_COUNT-dw)<30) ;  // wait minimum ~1.8 second
		}
		printk("Ready\n");

					// reuse BIOS status area

		BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);  // blank out volatile data area
		VIDEO_CURSOR_POSX=nTempCursorX;
		VIDEO_CURSOR_POSY=nTempCursorY;

		bprintf("Entering BootIdeInit()\n");
		BootIdeInit();
		printk("\n");

		nTempCursorMbrX=VIDEO_CURSOR_POSX;
		nTempCursorMbrY=VIDEO_CURSOR_POSY;

			// HDD Partition Table dump

		{
			BYTE ba[512];
			memset(&ba,0x00,sizeof(ba));
			if(BootIdeReadSector(0, &ba[0], 0, 0, 512)) {
				printk("Unable to read HDD first sector getting partition table\n");
			} else {

				if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) fMbrPresent=true;

				if(fMbrPresent) {
					volatile BYTE * pb;
					int n=0, nPos=0;
#ifdef DISPLAY_MBR_INFO
					char sz[512];
					memset(&sz,0x00,sizeof(sz));

					VIDEO_ATTR=0xffe8e8e8;
					printk("MBR Partition Table:\n");
#endif
					(volatile BYTE *)pb=&ba[0x1be];
					n=0; nPos=0;
					while(n<4) {
#ifdef DISPLAY_MBR_INFO
						nPos=sprintf(sz, " hda%d: ", n+1);
#endif
						if(pb[0]&0x80) {
							nActivePartitionIndex=n;
							fSeenActive=true;
#ifdef DISPLAY_MBR_INFO
							nPos+=sprintf(&sz[nPos], "boot\t");
#endif
						} else {
#ifdef DISPLAY_MBR_INFO
							nPos+=sprintf(&sz[nPos], "   \t");
#endif
						}

#ifdef DISPLAY_MBR_INFO
	//					nPos+=sprintf(&sz[nPos],"type:%02X\tStart: %02X/%02X/%02X\tEnd: %02X/%02X/%02X  ", pb[4], pb[1],pb[2],pb[3],pb[5],pb[6],pb[7]);
						switch(pb[4]) {
							case 0x00:
								nPos+=sprintf(&sz[nPos],"Empty\t");
								break;
							case 0x82:
								nPos+=sprintf(&sz[nPos],"Swap\t");
								break;
							case 0x83:
								nPos+=sprintf(&sz[nPos],"Ext2 \t");
								break;
							case 0x1e:
								nPos+=sprintf(&sz[nPos],"FATX\t");
								break;
							default:
								nPos+=sprintf(&sz[nPos],"0x%02X\t", pb[4]);
								break;
						}
						if(pb[4]) {
							VIDEO_ATTR=0xffc8c8c8;
							if(pb[0]&0x80) VIDEO_ATTR=0xffd8d8d8;
							nPos+=sprintf(&sz[nPos],"Start: 0x%08x \tTotal: 0x%08x\t(%dMB)\n", *((DWORD *)&pb[8]), *((DWORD *)&pb[0xc]), (*((DWORD*)&pb[0xc]))/2048 );
						} else {
							VIDEO_ATTR=0xffa8a8a8;
							sz[nPos++]='\n'; sz[nPos]='\0';
						}
						printk(sz);
#endif
						n++; pb+=16;
					}

					printk("\n");

					VIDEO_ATTR=0xffffffff;
				} else { // no mbr signature
					;
				}
				BootIdeReadSector(0, &ba[0], 3, 0, 512);
				if(ba[0] == 'B' && ba[1] == 'R' && ba[2] == 'F' && ba[3] == 'R') nFATXPresent = true;
			}
		}

			// if we made it this far, lets have a solid green LED to celebrate
		I2cSetFrontpanelLed(
			I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3
		);

//	printk("i2C=%d SMC=%d, IDE=%d, tick=%d una=%d unb=%d\n", nCountI2cinterrupts, nCountInterruptsSmc, nCountInterruptsIde, BIOS_TICK_COUNT, nCountUnusedInterrupts, nCountUnusedInterruptsPic2);


	// enable below for stress testing
	// continually restarts _romwell without hard-resetting the hardware

#if 0
		{
			void bootloader2();
			printk("rebooting\n");
			bootloader2();
		}
#endif

	// Used to start Bochs; now a misnomer as it runs vmlinux
	// argument 0 for hdd and 1 for from CDROM
#ifdef FORCE_CD_BOOT
	StartBios(1, 0, 0);
#else
		if(fMbrPresent && fSeenActive) { // if there's an MBR and active partition , try to boot from HDD
			StartBios(0, nActivePartitionIndex, nFATXPresent);
		} else { // otherwise prompt for CDROM
			StartBios(1, 0, nFATXPresent);
		}
#endif
	}
