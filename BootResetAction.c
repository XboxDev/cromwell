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

#ifdef XBE
#include "config-xbe.h"
#else
#include "config-rom.h"
#endif

#ifdef memcpy
#undef memcpy
#endif
#ifdef memset
#undef memset
#endif

BYTE baBackdrop[60*72*4];
JPEG jpegBackdrop;

const KNOWN_FLASH_TYPE aknownflashtypesDefault[] = {
	{ 0xbf, 0x61, "SST49LF020", 0x40000 },  // default flash types
	{ 0x01, 0xd5, "Am29F080B", 0x100000 },  // default flash types
	{ 0x04, 0xd5, "Fujitsu MBM29F080A", 0x100000 },  // default flash types
	{ 0xad, 0xd5, "Hynix HY29F080", 0x100000 },  // default flash types
	{ 0x20, 0xf1, "ST M29F080A", 0x100000 },  // default flash types
	{ 0x89, 0xA6, "Sharp LHF08CH1", 0x100000 }, // default flash types
	{ 0, 0, "", 0 }
};

/*
Hints:

1) We are in a funny environment, executing out of ROM.  That means it is not possible to
 define filescope non-const variables with an initializer.  If you do so, the linker will oblige and you will end up with
 a 4MByte image instead of a 1MByte one, the linker has added the RAM init at 400000.
*/


// access to RTC CMOS memory

void BiosCmosWrite(BYTE bAds, BYTE bData) {
		IoOutputByte(0x70, bAds);
		IoOutputByte(0x71, bData);
}

BYTE BiosCmosRead(BYTE bAds)
{
		IoOutputByte(0x70, bAds);
		return IoInputByte(0x71);
}


//////////////////////////////////////////////////////////////////////
//
//  BootResetAction()

extern void BootResetAction ( void ) {
	BYTE bAvPackType;
	__int64 i64Timestamp;
	bool fMbrPresent=false;
	bool fSeenActive=false;
	int nActivePartitionIndex=0;
	int nTempCursorX, nTempCursorY;

#if INCLUDE_FILTROR
	// clear down channel quality stats
	bfcqs.m_dwBlocksFromPc=0;
	bfcqs.m_dwCountChecksumErrorsSeenFromPc=0;
	bfcqs.m_dwBlocksToPc=0;
	bfcqs.m_dwCountTimeoutErrorsSeenToPc=0;
#endif

	bprintf("\nBOOT: starting BootResetAction()\n\r");

		// init malloc() and free() structures

#ifdef XBE
	MemoryManagementInitialization((void *)0x1000000, 0x0E00000);
#else
	MemoryManagementInitialization((void *)0x1000000, 0x2000000);
#endif

	// then setup the interrupt table
	// RJS - I tried to enable interrupts earlier to fix some instability
	// but there was no improvement -  table setup left here


		// prep our BIOS console print state

	VIDEO_ATTR=0xffffffff;
	VIDEO_LUMASCALING=0;
	VIDEO_RSCALING=0;
	VIDEO_BSCALING=0;

		// if we don't make the PIC happy within 200mS, the damn thing will reset us

	BootPerformPicChallengeResponseAction();
	WATCHDOG;
	bprintf("BOOT: done with PIC challenge\n\r");

	// bring up Video (2BL portion)
#ifndef XBE
	BootVgaInitialization();
	WATCHDOG;
	bprintf("BOOT: done with VGA initialization\n\r");
#endif

	BootEepromReadEntireEEPROM();

	WATCHDOG;
	bAvPackType=BootVgaInitializationKernel(VIDEO_PREFERRED_LINES);

	{ // decode and malloc backdrop bitmap
		extern int _start_backdrop, _end_backdrop;
		BootVideoJpegUnpackAsRgb(
			(BYTE *)&_start_backdrop,
			((DWORD)(&_end_backdrop)-(DWORD)(&_start_backdrop)),
			&jpegBackdrop
		);
	}

	WATCHDOG; 	TRACE;

		// paint the backdrop

	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
	TRACE;

		// get icon background into storage array

	BootVideoBlit((DWORD *)&baBackdrop[0], 60*4, (DWORD *)(FRAMEBUFFER_START+640*4*VIDEO_MARGINY+VIDEO_MARGINX*4), 640*4, 72);
	TRACE;

		// finally bring up video
	BootVideoEnableOutput(bAvPackType);
		// initialize the PCI devices

	BootPciPeripheralInitialization();
	WATCHDOG;
	bprintf("BOOT: done with PCI initialization\n\r");


	{ // instability tracking, looking for duration changes
		int a2, a3;
		 __asm__ __volatile__ (" rdtsc" :"=a" (a3), "=d" (a2));
		i64Timestamp=((__int64)a3)|(((__int64)a2)<<32);
	}

	BootInterruptsWriteIdt();
	WATCHDOG;

		// display solid red frontpanel LED while we start up
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );

	I2CTransmitWord(0x10, 0x1901); // no reset on eject
#ifdef IS_XBE_BOOTLOADER
	I2CTransmitWord(0x10, 0x0c01); // close DVD tray
#else
	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
#endif

	WATCHDOG;

		// start up Timer 0 so we get the 18.2Hz tick interrupts

	IoOutputByte(0x43, 0x36); // was 0x36
	IoOutputByte(0x40, 0xff);
	IoOutputByte(0x40, 0xff);

	WATCHDOG;
	TRACE;

	VIDEO_CURSOR_POSY=VIDEO_MARGINY;
	VIDEO_CURSOR_POSX=(VIDEO_MARGINX+64)*4;
#ifdef XBE
	printk("\2Xbox Linux XROMWELL  " VERSION "\2\n" );
#else
	printk("\2Xbox Linux Clean BIOS  " VERSION "\2\n" );
#endif
	VIDEO_CURSOR_POSY=VIDEO_MARGINY+32;
	VIDEO_CURSOR_POSX=(VIDEO_MARGINX+64)*4;
	printk( __DATE__ " -  http://xbox-linux.sf.net\n");
	VIDEO_CURSOR_POSX=(VIDEO_MARGINX+64)*4;
	printk("(C)2002 Xbox Linux Team - Licensed under the GPL\n");
	printk("\n");

	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=VIDEO_CURSOR_POSY;

#ifdef REPORT_VIDEO_MODE
	
	VIDEO_ATTR=0xffc8c8c8;
	printk("AV Cable : ");
	VIDEO_ATTR=0xffc8c800;

	switch(bAvPackType) {
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
			printk("Composite 480 PAL");
			break;
		case 7:
			printk("Composite 576 PAL");
			break;
		case 8:
			printk("Composite 480 NTSC");
			break;
	}

//	printk("  - Time=0x%08X/0x%08X", (DWORD)(i64Timestamp>>32), (DWORD)i64Timestamp);
	printk("\n");
#endif

	WATCHDOG;

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

			bprintf("Rewriting EEPROM, please wait...\n");

			while(nAds<0x100) {
				I2CTransmitWord( 0x54, (nAds<<8)|baMyEEProm[nAds], true);
				for(n=0;n<1000;n++) { IoInputByte(I2C_IO_BASE+0); }
				nAds++;
			}
	}
#endif


			// now do the main BIOS action

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

			// configure ACPI hardware to generate interrupt on PIC-chip pin6 action (via EXTSMI#)

	IoOutputByte(0x80c0, 0x08);  // from 2bl
	IoOutputByte(0x8004, IoInputByte(0x8004)|1);  // KERN: SCI enable == SCI interrupt generated
	IoOutputByte(0x8022, IoInputByte(0x8022)|2);  // KERN: Interrupt enable register, b1 RESERVED in AMD docs
	IoOutputByte(0x8002, IoInputByte(0x8002)|1);  // KERN: Enable SCI interrupt when timer status goes high
	IoOutputByte(0x8028, IoInputByte(0x8028)|1);  // KERN: setting readonly trap event???


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

		BootEepromPrintInfo();

#ifndef XBE
		{
			OBJECT_FLASH of;
			of.m_pbMemoryMappedStartAddress=(BYTE *)0xff000000;
			BootFlashCopyCodeToRam();
			BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0]);
			VIDEO_ATTR=0xffc8c8c8;
			printk("Flash type: ");
			VIDEO_ATTR=0xffc8c800;
			printk("%s\n", of.m_szFlashDescription);
		}
#endif

			// init the HDD and DVD
		VIDEO_ATTR=0xffc8c8c8;
		printk("Initializing IDE Controller\n");

			// wait around for HDD to become ready

		BootIdeWaitNotBusy(0x1f0);

			// reuse BIOS status area

		BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);  // blank out volatile data area
		VIDEO_CURSOR_POSX=nTempCursorX;
		VIDEO_CURSOR_POSY=nTempCursorY;

		BootIdeInit();
		printk("\n");

		I2CTransmitWord(0x10, 0x0b01); // unknown, done immediately after reading out eeprom data
		I2CTransmitWord(0x10, 0x0b00); // unknown, done after video update
		I2CTransmitWord(0x10, 0x1a01); // unknown, done immediately after reading out eeprom data

			// HDD Partition Table dump

		{
			BYTE ba[512];
			if(BootIdeReadSector(0, &ba[0], 0, 0, 512)) {
				printk("Unable to read HDD first sector\n");
			} else {

				if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) fMbrPresent=true;

				if(fMbrPresent) {
					volatile BYTE * pb;
					int n=0, nPos=0;
#ifdef DISPLAY_MBR_INFO
					char sz[512];

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
							nPos+=sprintf(&sz[nPos],"Start: 0x%08x\tTotal: 0x%08x\t(%dMB)\n", *((DWORD *)&pb[8]), *((DWORD *)&pb[0xc]), (*((DWORD
*)&pb[0xc]))/2048 );
						} else {
							VIDEO_ATTR=0xffa8a8a8;
							sz[nPos++]='\n'; sz[nPos]='\0';
						}
						printk(sz);
	#endif
						n++; pb+=16;
					}

					printk("\n");

// If there is no active partition, it's posible
// that the HDD has and empty MBR and we want
// to boot from cdrom
//
//					if(!fSeenActive) {
//						printk("No active partition.  Cannot boot, halting\n");
//						while(1) ;
//					}
					VIDEO_ATTR=0xffffffff;
				} else { // no mbr signature
					;
				}

			}
		}

			// if we made it this far, lets have a solid green LED to celebrate
		I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3);

//	printk("i2C=%d SMC=%d, IDE=%d, tick=%d una=%d unb=%d\n", nCountI2cinterrupts, nCountInterruptsSmc, nCountInterruptsIde, BIOS_TICK_COUNT, nCountUnusedInterrupts, nCountUnusedInterruptsPic2);


	// Used to start Bochs; now a misnomer as it runs vmlinux
	// argument 0 for hdd and 1 for from CDROM
#ifdef FORCE_CD_BOOT
	StartBios(1, 0);
#else
		if(fMbrPresent && fSeenActive) { // if there's an MBR and active partition , try to boot from HDD
			StartBios(0, nActivePartitionIndex);
		} else { // otherwise prompt for CDROM
			StartBios(1, 0);
		}
#endif
	}
