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

 */

#include "boot.h"
#include "BootEEPROM.h"
#include "BootFlash.h"
#include "BootFATX.h"
#include "xbox.h"
#include "cpu.h"
#include "config.h"
/*
#include "audio.h"
*/
#include "video.h"
#include "memory_layout.h"

JPEG jpegBackdrop;

CONFIGENTRY kernel_config;

int nTempCursorMbrX, nTempCursorMbrY;

extern volatile int nInteruptable;

volatile CURRENT_VIDEO_MODE_DETAILS vmode;
/*
volatile AC97_DEVICE ac97device;

volatile AUDIO_ELEMENT_SINE aesTux;
volatile AUDIO_ELEMENT_SINE aesSong;
volatile AUDIO_ELEMENT_NOISE aenTux;
*/
extern KNOWN_FLASH_TYPE aknownflashtypesDefault[];

/*
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
*/

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
        int temp;

 		
        memcpy(&cromwell_config,(void*)(0x03A00000+0x20),4);
        memcpy(&cromwell_retryload,(void*)(0x03A00000+0x24),4);
	memcpy(&cromwell_loadbank,(void*)(0x03A00000+0x28),4);
        memcpy(&cromwell_Biostype,(void*)(0x03A00000+0x2C),4);
 	
	VIDEO_CURSOR_POSX=40;
	VIDEO_CURSOR_POSY=140; 	
        
	VIDEO_AV_MODE = 0xff;
	nInteruptable = 0;	

		
	// prep our BIOS console print state
	VIDEO_ATTR=0xffffffff;

	// init malloc() and free() structures
	MemoryManagementInitialization((void *)MEMORYMANAGERSTART, MEMORYMANAGERSIZE);
	


	BootInterruptsWriteIdt();	
	//bprintf("BOOT: done interrupts\n\r");


	// initialize the PCI devices
	//bprintf("BOOT: starting PCI init\n\r");
	BootPciPeripheralInitialization();
	// Reset the AGP bus and start with good condition
	BootAGPBUSInitialization();
	
	// We disable The CPU Cache
       	cache_disable();
	// We Update the Microcode of the CPU
	display_cpuid_update_microcode();
       	// We Enable The CPU Cache
       	cache_enable();
       	//setup_ioapic();
	// We look how much memory we have ..
	BootDetectMemorySize();     
	
	BootEepromReadEntireEEPROM();
	bprintf("BOOT: Read EEPROM\n\r");
//	DumpAddressAndData(0, (BYTE *)&eeprom, 256);
        
        // Load and Init the Background image
        
        // clear the Video Ram
	memset((void *)FB_START,0x00,0x400000);
	
	BootVgaInitializationKernelNG((CURRENT_VIDEO_MODE_DETAILS *)&vmode);

	//bprintf("BOOT: kern VGA init done\n\r");
        
	{ // decode and malloc backdrop bitmap
		extern int _start_backdrop;
		BootVideoJpegUnpackAsRgb(
			(BYTE *)&_start_backdrop,
			&jpegBackdrop
		);
	}
        
	// display solid red frontpanel LED while we start up
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
       
   
       
	// paint the backdrop
#ifndef DEBUG_MODE
	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
#endif
	//bprintf("BOOT: done backdrop\n\r");

	nInteruptable = 1;	      
       
	I2CTransmitWord(0x10, 0x1a01); // unknown, done immediately after reading out eeprom data
	I2CTransmitWord(0x10, 0x1b04); // unknown
	/*
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
	*/
	/* Here, the interrupts are Switched on now */
	BootPciInterruptEnable();
        /* We allow interrupts */
	nInteruptable = 1;	
	

	I2CTransmitWord(0x10, 0x1901); // no reset on eject
         
	VIDEO_CURSOR_POSY=vmode.ymargin;
	VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
	
	if (cromwell_config==XROMWELL) 	printk("\2Xbox Linux Xromwell  " VERSION "\2\n" );
	if (cromwell_config==CROMWELL)	printk("\2Xbox Linux Cromwell BIOS  " VERSION "\2\n" );
	VIDEO_CURSOR_POSY=vmode.ymargin+32;
	VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
	printk( __DATE__ " -  http://xbox-linux.org\n");
	VIDEO_CURSOR_POSX=(vmode.xmargin/*+64*/)*4;
	printk("(C)2002-2004 Xbox Linux Team   RAM : %d MB  ",xbox_ram);
	if (cromwell_config==CROMWELL) {
		printk("(Load Tries: %d Bank: %d ",cromwell_retryload,cromwell_loadbank);
		if (cromwell_Biostype == 0) printk("Bios: 256k)");
		if (cromwell_Biostype == 1) printk("Bios: 1MB)");
	}
        printk("\n");
    
	// capture title area
	VIDEO_ATTR=0xffc8c8c8;
	printk("Encoder: ");
	VIDEO_ATTR=0xffc8c800;
	printk("%s  ", VideoEncoderName());
	VIDEO_ATTR=0xffc8c8c8;
	printk("Cable: ");
	VIDEO_ATTR=0xffc8c800;
	printk("%s  ", AvCableName());
        
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
		printk("%doC  \n", nx);
	}

	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=VIDEO_CURSOR_POSY;
		
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );

		VIDEO_ATTR=0xffffffff;

		// gggb while waiting for Ethernet & Hdd

		I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2);

		// set Ethernet MAC address from EEPROM
	        {
		volatile BYTE * pb=(BYTE *)0xfef000a8;  // Ethernet MMIO base + MAC register offset (<--thanks to Anders Gustafsson)
		int n;
		for(n=5;n>=0;n--) { *pb++=	eeprom.MACAddress[n]; } // send it in backwards, its reversed by the driver
	        }

		BootEepromPrintInfo();
#ifdef FLASH
		{
		OBJECT_FLASH of;
		memset(&of,0x00,sizeof(of));
		of.m_pbMemoryMappedStartAddress=(BYTE *)LPCFlashadress;
		//BootFlashCopyCodeToRam();
		BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0]);
		VIDEO_ATTR=0xffc8c8c8;
		printk("Flash type: ");
		VIDEO_ATTR=0xffc8c800;
		printk("%s\n", of.m_szFlashDescription);
		}
#endif
		printk("BOOT: start USB init\n");
		BootStartUSB();

		// init the IDE devices
		VIDEO_ATTR=0xffc8c8c8;
		printk("Initializing IDE Controller\n");
		BootIdeWaitNotBusy(0x1f0);
                wait_ms(200);
		
		printk("Ready\n");
		
		// reuse BIOS status area

#ifndef DEBUG_MODE
		BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);  // blank out volatile data area
#endif
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
			memset(ba,0x00,sizeof(ba));
			if(BootIdeReadSector(0, &ba[0], 0, 0, 512)) {
				printk("Unable to read HDD first sector getting partition table\n");
			} else {

				if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) fMbrPresent=true;

				if(fMbrPresent) {
					volatile BYTE * pb;
					int n=0, nPos=0;
#ifdef DISPLAY_MBR_INFO
					char sz[512];
					memset(sz,0x00,sizeof(sz));

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

//#ifndef IS_XBE_CDLOADER
	


	temp = -1; // Nothing Choosed
  	memset(&kernel_config,0,sizeof(CONFIGENTRY));
  		
#ifdef MENU

	if(fMbrPresent && fSeenActive) {
		temp = BootMenu(&kernel_config, 0,nActivePartitionIndex, nFATXPresent);
	} else {
		temp = BootMenu(&kernel_config, 1,0, nFATXPresent); 
	}
#endif  
 
        //printk("We are starting the config %d\n",temp); 
     	StartBios(&kernel_config, nActivePartitionIndex, nFATXPresent,temp);
    
    while(1);   
 


}
