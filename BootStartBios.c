/**************************************************************************/
/* BIOS start                                                             */
/*  Michael Steil                                                         */
/*  2002-12-19 andy@warmcat.com changed to use partition marked as boot   */
/*                              changed to use non 8.3 ISO9660 names      */
/*  2002-12-18 andy@warmcat.com added stuff for ISO9660                   */
/*  2002-11-25 andy@warmcat.com changed to using existing GDT/IDT         */
/*                              fixed AND bug in 16-bit code, tidied      */
/*  2002-12-11 andy@warmcat.com rewrote entirely to use xbeloader method  */
/*                              and reiserfs grub code                    */
/**************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"
#include <shared.h>
#include <filesys.h>
#include "rc4.h"
#include "sha1.h"
#include "BootParser.h"
#include "BootFATX.h"
#include "xbox.h"

#ifdef XBE
#include "config-xbe.h"
#else
#include "config-rom.h"
#endif

#include "BootUsbOhci.h"
extern volatile USB_CONTROLLER_OBJECT usbcontroller[2];

#undef strcpy

unsigned long saved_drive;
unsigned long saved_partition;
grub_error_t errnum;
unsigned long boot_drive;

extern int nTempCursorMbrX, nTempCursorMbrY;

void console_putchar(char c) { printk("%c", c); }
extern unsigned long current_drive;
char * strcpy(char *sz, const char *szc);
int _strncmp(const char *sz1, const char *sz2, int nMax);

void setup(void* KernelPos, void* PhysInitrdPos, void* InitrdSize, char* kernel_cmdline);

int nRet;
DWORD dwKernelSize, dwInitrdSize;
int nSizeHeader;
const BYTE baGdt[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x00 dummy
	0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00, // 0x08 code32
	0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00, // 0x10 code32
	0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00, // 0x18 data32
	0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0x8f, 0x00, // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
	0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0x8f, 0x00, // 0x28 data16

	0x30, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


#define ICON_FATX 	0
#define ICON_NATIVE 	1
#define ICON_CD 	2
#define ICON_SETUP	3

#define ICONCOUNT	4

typedef struct {
	int nDestX;
	int nDestY;
	int nSrcX;
	int nSrcLength;
	int nSrcHeight;
	int nTextX;
	int nTextY;
} ICON;

ICON icon[ICONCOUNT];

void BootPrintConfig(CONFIGENTRY *config) {
	printk("  Bootconfig : Kernel  %s\n", config->szKernel);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Initrd  %s\n", config->szInitrd);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Command %s\n", config->szAppend);
	VIDEO_ATTR=0xffa8a8a8;
}

int BootLodaConfigNative(int nActivePartition, CONFIGENTRY *config) {
	DWORD dwConfigSize=0;
	char szGrub[256+4];

	szGrub[0]=0xff; szGrub[1]=0xff; szGrub[2]=nActivePartition; szGrub[3]=0x00;

	errnum=0; boot_drive=0; saved_drive=0; saved_partition=0x0001ffff; buf_drive=-1;
	current_partition=0x0001ffff; current_drive=0xff; buf_drive=-1; fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;

	
	VIDEO_ATTR=0xffa8a8a8;

	strcpy(&szGrub[4], "/boot/linuxboot.cfg");
	nRet=grub_open(szGrub);
	dwConfigSize=filemax;
	if(nRet!=1 || (errnum)) {
		printk("linuxboot.cfg not found, using defaults\n");
	} else {
		grub_read((void *)0x90000, filemax);
		ParseConfig((char *)0x90000,config);
		BootPrintConfig(config);
		printf("linuxboot.cfg is %d bytes long.\n", dwConfigSize);
	}
	grub_close();
	
	strcpy(&szGrub[4], config->szKernel);
	nRet=grub_open(szGrub);

	if(nRet!=1) {
		printk("Unable to load kernel, Grub error %d\n", errnum);
		while(1) ;
	}
	dwKernelSize=grub_read((void *)0x90000, 0x400);
	nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
	dwKernelSize+=grub_read((void *)0x90400, nSizeHeader-0x400);
	dwKernelSize+=grub_read((void *)0x00100000, filemax-nSizeHeader);
	grub_close();
	printk(" -  %d bytes...\n", dwKernelSize);

	if( (_strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd[0]) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s ", config->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
		strcpy(&szGrub[4], config->szInitrd);
		nRet=grub_open(szGrub);
		if(filemax==0) {
			printf("Empty file\n"); while(1);
		}
		if( (nRet!=1 ) || (errnum)) {
			printk("Unable to load initrd, Grub error %d\n", errnum);
			while(1) ;
		}
		printk(" - %d bytes\n", filemax);
		dwInitrdSize=grub_read((void *)0x03000000, filemax);
		grub_close();
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
	}
	
	return true;
}

int BootLodaConfigFATX(CONFIGENTRY *config) {
	
	FATXPartition *partition = NULL;
	FATXFILEINFO fileinfo;
	FATXFILEINFO infokernel;
	FATXFILEINFO infoinitrd;
	
	printk("Loading linuxboot.cfg form FATX\n");
	partition = OpenFATXPartition(0,
			SECTOR_STORE,
			STORE_SIZE);
	if(partition != NULL) {
		if(!LoadFATXFile(partition,"/linuxboot.cfg",&fileinfo) ) {
			printk("Loading of linuxboot.cfg failed\n");
			while(1);
		}
	}
	ParseConfig(fileinfo.buffer,config);
	BootPrintConfig(config);
	if(! LoadFATXFile(partition,config->szKernel,&infokernel)) {
		printk("Error loading kernel %s\n",config->szKernel);
		while(1);
	} else {
		dwKernelSize = infokernel.fileSize;
		// moving the kernel to its final location
		memcpy((BYTE *)0x90000,&infokernel.buffer[0],0x400);	
		nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
		memcpy((BYTE *)0x90400,&infokernel.buffer[0x400],nSizeHeader-0x400);
		memcpy((BYTE *)0x00100000,&infokernel.buffer[nSizeHeader],infokernel.fileSize);
		printk(" -  %d %d bytes...\n", dwKernelSize, infokernel.fileRead);
	}
	if( (_strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd[0]) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from FATX", config->szInitrd);
		if(! LoadFATXFile(partition,config->szInitrd,&infoinitrd)) {
			printk("Error loading initrd %s\n",config->szInitrd);
			while(1);
		} else {
			dwInitrdSize = infoinitrd.fileSize;
			memcpy((BYTE *)0x03000000,infoinitrd.buffer,infoinitrd.fileSize);	// moving the initrd to its final location
		}
		printk(" - %d %d bytes\n", dwInitrdSize,infoinitrd.fileRead);
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
		printk("");
	}
	return true;
}

int BootLodaConfigCD(CONFIGENTRY *config) {

	DWORD dwConfigSize=0;
	BYTE ba[2048], baBackground[320*32*4];
#ifndef IS_XBE_CDLOADER
	BYTE b;
#endif
	BYTE bCount=0, bCount1;
	int n;

	DWORD dwY=VIDEO_CURSOR_POSY;
	DWORD dwX=VIDEO_CURSOR_POSX;

	BootVideoBlit((DWORD *)&baBackground[0], 320*4, (DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX), currentvideomodedetails.m_dwWidthInPixels*4, 32);

#ifndef IS_XBE_CDLOADER
	while((b=BootIdeGetTrayState())>=8) {
		VIDEO_CURSOR_POSX=dwX;
		VIDEO_CURSOR_POSY=dwY;
		bCount++;
		bCount1=bCount; if(bCount1&0x80) { bCount1=(-bCount1)-1; }
		if(b>=16) {
			VIDEO_ATTR=0xff000000|(((bCount1>>1)+64)<<16)|(((bCount1>>1)+64)<<8)|0 ;
		} else {
			VIDEO_ATTR=0xff000000|(((bCount1>>2)+192)<<16)|(((bCount1>>2)+192)<<8)|(((bCount1>>2)+192)) ;
		}
		printk("\2Please insert CD\2");
		for(n=0;n<1000000;n++) { ; }
	}
#endif

	VIDEO_ATTR=0xffffffff;

	VIDEO_CURSOR_POSX=dwX;
	VIDEO_CURSOR_POSY=dwY;
	BootVideoBlit(
		(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
		currentvideomodedetails.m_dwWidthInPixels*4, (DWORD *)&baBackground[0], 320*4, 32
	);

		// wait until the media is readable

	{
		bool fMore=true, fOkay=true;
		while(fMore) {
			if(BootIdeReadSector(1, &ba[0], 0x10, 0, 2048)) { // starts at 16

				if(BootIdeAtapiAdditionalSenseCode(1, &ba[0], 2048)<12) {
					fMore=false; fOkay=false;
				} else {
					VIDEO_CURSOR_POSX=dwX;
					VIDEO_CURSOR_POSY=dwY;
					bCount++;
					bCount1=bCount; if(bCount1&0x80) { bCount1=(-bCount1)-1; }

					VIDEO_ATTR=0xff000000|(((bCount1)+64)<<16)|(((bCount1>>1)+128)<<8)|(((bCount1)+128)) ;

					printk("\2Waiting for drive\2");
					for(n=0;n<1000000;n++) { ; }
				}

// If cromwell acts as an xbeloader it falls back to try reading
//	the config file from fatx

					if(!fOkay) {
						void BootFiltrorDebugShell();
						printk("cdrom unhappy\n");
#if INCLUDE_FILTROR
						BootFiltrorDebugShell();
#endif
						while(1);
					}
				} else {
					fMore=false;
				}
			}
		}

		VIDEO_CURSOR_POSX=dwX;
		VIDEO_CURSOR_POSY=dwY;
		BootVideoBlit(
			(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
			currentvideomodedetails.m_dwWidthInPixels*4, (DWORD *)&baBackground[0], 320*4, 32
		);

//			BootVideoClearScreen();
//			VideoDumpAddressAndData(0, &ba[0x1d0], 0x200);

		{
			ISO_PRIMARY_VOLUME_DESCRIPTOR * pipvd = (ISO_PRIMARY_VOLUME_DESCRIPTOR *)&ba[0];
			char sz[64];
			BootIso9660DescriptorToString(pipvd->m_szSystemIdentifier, sizeof(pipvd->m_szSystemIdentifier), sz);
			VIDEO_ATTR=0xffeeeeee;
			printk("Cdrom: ");
			VIDEO_ATTR=0xffeeeeff;
			printk("%s", sz);
			VIDEO_ATTR=0xffeeeeee;
			printk(" - ");
			VIDEO_ATTR=0xffeeeeff;
			BootIso9660DescriptorToString(pipvd->m_szVolumeIdentifier, sizeof(pipvd->m_szVolumeIdentifier), sz);
			printk("%s\n", sz);
		}

	printk("  Loading linuxboot.cfg from CDROM... \n");
	dwConfigSize=BootIso9660GetFile("/linuxboot.cfg", (BYTE *)0x90000, 0x800, 0x0);

	if((int)dwConfigSize<0) { // not found, try mangled 8.3 version
		dwConfigSize=BootIso9660GetFile("/LINUXBOO.CFG", (BYTE *)0x90000, 0x800, 0x0);
		if((int)dwConfigSize<0) { // has to be there on CDROM
			printk("Unable to find it, halting\n");
			while(1) ;
		}
	}
	
	ParseConfig((char *)0x90000,config);
	BootPrintConfig(config);
	
	dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE *)0x90000, 0x400, 0x0);
	if((int)dwKernelSize<0) { // not found, try 8.3
		strcpy(config->szKernel, "/VMLINUZ.");
		dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE *)0x90000, 0x400, 0x0);
	}
	nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
	dwKernelSize+=BootIso9660GetFile(config->szKernel, (void *)0x90400, nSizeHeader-0x400, 0x400);
	dwKernelSize+=BootIso9660GetFile(config->szKernel, (void *)0x00100000, 4096*1024, nSizeHeader);
	printk(" -  %d bytes...\n", dwKernelSize);
	
	if( (_strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from CDROM", config->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;

		dwInitrdSize=BootIso9660GetFile(config->szInitrd, (void *)0x03000000, 4096*1024, 0);
		if((int)dwInitrdSize<0) { // not found, try 8.3
			strcpy(config->szInitrd, "/INITRD.");
			dwInitrdSize=BootIso9660GetFile(config->szInitrd, (void *)0x03000000, 4096*1024, 0);
		}
		printk(" - %d bytes\n", dwInitrdSize);
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
		printk("");
	}
	
	return true;
}

void BootIcons(int nXOffset, int nYOffset, int nTextOffsetX, int nTextOffsetY) {
	icon[ICON_FATX].nDestX = nXOffset + 120;
	icon[ICON_FATX].nDestY = nYOffset - 74;
	icon[ICON_FATX].nSrcX = 640;
	icon[ICON_FATX].nSrcLength = 90;
	icon[ICON_FATX].nSrcHeight = 64;
	icon[ICON_FATX].nTextX = (nTextOffsetX+60)<<2;;
	icon[ICON_FATX].nTextY = nTextOffsetY;
	
	icon[ICON_NATIVE].nDestX = nXOffset + 245;
	icon[ICON_NATIVE].nDestY = nYOffset - 74;
	icon[ICON_NATIVE].nSrcX = 396;
	icon[ICON_NATIVE].nSrcLength = 90;
	icon[ICON_NATIVE].nSrcHeight = 64;
	icon[ICON_NATIVE].nTextX = (nTextOffsetX+240)<<2;;
	icon[ICON_NATIVE].nTextY = nTextOffsetY;
	
	icon[ICON_CD].nDestX = nXOffset + 350;
	icon[ICON_CD].nDestY = nYOffset - 74;
	icon[ICON_CD].nSrcX = 488;
	icon[ICON_CD].nSrcLength = 555-488;
	icon[ICON_CD].nSrcHeight = 64;
	icon[ICON_CD].nTextX = (nTextOffsetX+350)<<2;
	icon[ICON_CD].nTextY = nTextOffsetY;
	
	icon[ICON_SETUP].nDestX = nXOffset + 440;
	icon[ICON_SETUP].nDestY = nYOffset - 74;
	icon[ICON_SETUP].nSrcX = 556;
	icon[ICON_SETUP].nSrcLength = 639-556;
	icon[ICON_SETUP].nSrcHeight = 64;
	icon[ICON_SETUP].nTextX = (nTextOffsetX+440)<<2;
	icon[ICON_SETUP].nTextY = nTextOffsetY;
}

void BootStartBiosDoIcon(ICON *icon, BYTE bOpaqueness)
{

		BootVideoJpegBlitBlend(
			(DWORD *)(FRAMEBUFFER_START+/*(640*4*currentvideomodedetails.m_dwMarginYInLinesRecommended)*/+(icon->nDestX<<2)+(currentvideomodedetails.m_dwWidthInPixels*4*icon->nDestY)),
			currentvideomodedetails.m_dwWidthInPixels * 4, // dest bytes per line
			&jpegBackdrop, // source jpeg object
			(DWORD *)(((BYTE *)jpegBackdrop.m_pBitmapData)+((jpegBackdrop.m_nHeight-64)*jpegBackdrop.m_nWidth*jpegBackdrop.m_nBytesPerPixel)+(icon->nSrcX *jpegBackdrop.m_nBytesPerPixel)),
			0xff00ff|(((DWORD)bOpaqueness)<<24),
			(DWORD *)(((BYTE *)BootVideoGetPointerToEffectiveJpegTopLeft(&jpegBackdrop))+(jpegBackdrop.m_nWidth * (icon->nDestY) *jpegBackdrop.m_nBytesPerPixel)+((icon->nDestX) *jpegBackdrop.m_nBytesPerPixel)),
			jpegBackdrop.m_nWidth*jpegBackdrop.m_nBytesPerPixel,
			jpegBackdrop.m_nBytesPerPixel,
			icon->nSrcLength, icon->nSrcHeight
		);
}

void RecoverMbrArea()
{
		BootVideoClearScreen(&jpegBackdrop, nTempCursorMbrY, VIDEO_CURSOR_POSY);  // blank out volatile data area
		VIDEO_CURSOR_POSX=nTempCursorMbrX;
		VIDEO_CURSOR_POSY=nTempCursorMbrY;
}


void StartBios(	int nDrive, int nActivePartition , int nFATXPresent) {
	CONFIGENTRY config;
	char szGrub[256+4];
#ifdef MENU
	int nTempCursorResumeX, nTempCursorResumeY;
#endif
	/*
	unsigned char hash[16];
	int a;
	*/
	int nIcon = ICONCOUNT;
	/*
	struct SHA1Context context;
	*/

#ifndef XBE
	BootPciInterruptEnable();
#else
#endif

	memset(&config,0,sizeof(CONFIGENTRY));

	szGrub[0]=0xff; szGrub[1]=0xff; szGrub[2]=nActivePartition; szGrub[3]=0x00;

	errnum=0; boot_drive=0; saved_drive=0; saved_partition=0x0001ffff; buf_drive=-1;
	current_partition=0x0001ffff; current_drive=0xff; buf_drive=-1; fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;


	strcpy(config.szAppend, "root=/dev/hda2 devfs=mount kbd-reset"); // default
	strcpy(config.szKernel, "/boot/vmlinuz");
	strcpy(config.szInitrd, "/boot/initrd");

#ifndef IS_XBE_CDLOADER
#ifdef MENU
	{
		int nTempCursorX, nTempCursorY, nTempStartMessageCursorX, nTempStartMessageCursorY, nTempEntryX, nTempEntryY;
		int nModeDependentOffset=(currentvideomodedetails.m_dwWidthInPixels-640)/2;  // icon offsets computed for 640 modes, retain centering in other modes
		#define DELAY_TICKS 72
		#define TRANPARENTNESS 0x30
		#define OPAQUENESS 0xc0
		#define SELECTED 0xff

		nTempCursorResumeX=nTempCursorMbrX;
		nTempCursorResumeY=nTempCursorMbrY;

		nTempEntryX=VIDEO_CURSOR_POSX;
		nTempEntryY=VIDEO_CURSOR_POSY;

		nTempCursorX=VIDEO_CURSOR_POSX;
		nTempCursorY=currentvideomodedetails.m_dwHeightInLines-80;

		VIDEO_CURSOR_POSX=((215+nModeDependentOffset)<<2);
		VIDEO_CURSOR_POSY=nTempCursorY-100;
 		nTempStartMessageCursorX=VIDEO_CURSOR_POSX;
 		nTempStartMessageCursorY=VIDEO_CURSOR_POSY;
		VIDEO_ATTR=0xffc8c8c8;
		printk("Close DVD tray to select\n");
		VIDEO_ATTR=0xffffffff;

		BootIcons(nModeDependentOffset, nTempCursorY, nModeDependentOffset, nTempCursorY);
		BootStartBiosDoIcon(&icon[ICON_FATX], TRANPARENTNESS);
		BootStartBiosDoIcon(&icon[ICON_NATIVE], TRANPARENTNESS);
		BootStartBiosDoIcon(&icon[ICON_CD], TRANPARENTNESS);
		BootStartBiosDoIcon(&icon[ICON_SETUP], TRANPARENTNESS);

		{
			int nShowSelect = false;
#ifndef XBE
			DWORD dwTick=BIOS_TICK_COUNT;
#endif
#ifdef XBE
			int n,m;
#endif
#ifdef XBE
			for(n=0;n<10000;n++){for(m=0;m<100000;m++){;}}
#endif

			for(nIcon = 0; nIcon < ICONCOUNT;nIcon ++) {
#ifdef XBE
				traystate = ETS_OPEN_OR_OPENING;
#endif
				nShowSelect = false;
				VIDEO_CURSOR_POSX=icon[nIcon].nTextX;
				VIDEO_CURSOR_POSY=icon[nIcon].nTextY;
				switch(nIcon){
					case ICON_FATX:
						if(nFATXPresent) {
							printk("/linuxboot.cfg from FATX\n");
							nShowSelect = true;
						} else {
							continue;
						}
						break;
					case ICON_NATIVE:
						if(nDrive != 1) {
							printk("/dev/hda\n");
							nShowSelect = true;
						} else {
							continue;
						}
						break;
					case ICON_CD:
						printk("/dev/hdb\n");
						nShowSelect = true;
						break;
					case ICON_SETUP:
						printk("Setup [TBD]\n");
						nShowSelect = false;
						break;
				}
				if(nShowSelect) {
#ifndef XBE
					while((BIOS_TICK_COUNT<(dwTick+DELAY_TICKS)) && 
							(traystate==ETS_OPEN_OR_OPENING)) {
						BootStartBiosDoIcon(&icon[nIcon], OPAQUENESS-((OPAQUENESS-TRANPARENTNESS)
									*(BIOS_TICK_COUNT-dwTick))/DELAY_TICKS);
					}
					dwTick=BIOS_TICK_COUNT;
#endif
#ifdef XBE
					BootStartBiosDoIcon(&icon[nIcon], OPAQUENESS);
					for(n=0;n<20000;n++){for(m=0;m<100000;m++){;}}
					if(BootIdeGetTrayState()<=8) {
						traystate=ETS_CLOSED;
					}
#endif
					if(traystate!=ETS_OPEN_OR_OPENING) {
						VIDEO_CURSOR_POSX=icon[nIcon].nTextX;
						VIDEO_CURSOR_POSY=icon[nIcon].nTextY;
						RecoverMbrArea();
						BootStartBiosDoIcon(&icon[nIcon], SELECTED);
						break;
					}
					BootStartBiosDoIcon(&icon[nIcon], TRANPARENTNESS);
				}
				BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);
			}
		}

		BootVideoClearScreen(&jpegBackdrop, nTempCursorResumeY, nTempCursorResumeY+100);
		BootVideoClearScreen(&jpegBackdrop, nTempStartMessageCursorY, nTempCursorY+16);
		
		VIDEO_CURSOR_POSX=nTempCursorResumeX;
		VIDEO_CURSOR_POSY=nTempCursorResumeY;
		
		
	}

#endif
#endif

	{  	// turn off USB
		BootUsbTurnOff((USB_CONTROLLER_OBJECT *)&usbcontroller[0]);
		BootUsbTurnOff((USB_CONTROLLER_OBJECT *)&usbcontroller[1]);
	}

/*
	{
		char c='1'+nActivePartition;
		if(nDrive==1) c=' ';
		printk("Booting from /dev/hd%c%c\n", 'a'+nDrive, c);
	}
*/	

	if(nIcon >= ICONCOUNT) {
		if(nDrive == 0) {
			printk("Defaulting to HDD boot\n");
			I2CTransmitWord(0x10, 0x0c01); // close DVD tray
#ifdef DEFAULT_FATX
			nIcon = ICON_FATX;
#else
			nIcon = ICON_NATIVE;
#endif
		} else {
			printk("Defaulting to CD boot\n");
			nIcon = ICON_CD;
		}
	}
	
	switch(nIcon) {
		case ICON_FATX:
			BootLodaConfigFATX(&config);
			break;
		case ICON_NATIVE:
			BootLodaConfigNative(nActivePartition, &config);
			break;
		case ICON_CD:
			BootLodaConfigCD(&config);
			break;
		case ICON_SETUP:
			printk("Settings mode not done yet :-)\nPlease reboot and try again\n");
			while(1);
			break;
		default:
			printk("Selection not implemented\n");
			break;
	}



	/*
        SHA1Reset(&context);
        SHA1Input(&context,infokernel.buffer,infokernel.fileSize);
        SHA1Result(&context,hash);
        for (a=0;a<20;a++) printk("%02X:",hash[a]);
        printk("\n");
	
        SHA1Reset(&context);
        SHA1Input(&context,infoinitrd.buffer,infoinitrd.fileSize);
        SHA1Result(&context,hash);
	for (a=0;a<20;a++) printk("%02X:",hash[a]);
	printk("\n");
	*/

	VIDEO_ATTR=0xff8888a8;
	printk("     Kernel:  %s\n", (char *)(0x00090200+(*((WORD *)0x9020e)) ));
	printk("\n");

	{
		char *sz="\2Starting Linux\2";
		VIDEO_CURSOR_POSX=((currentvideomodedetails.m_dwWidthInPixels-BootVideoGetStringTotalWidth(sz))/2)*4;
		VIDEO_CURSOR_POSY=currentvideomodedetails.m_dwHeightInLines-64;

		VIDEO_ATTR=0xff9f9fbf;
		printk(sz);
	}

		// we have to copy the GDT to a safe place, because one of the first
		// things Linux is going to do is switch to paging, making the BIOS
		// inaccessible and so crashing if we leave the GDT in BIOS

	memcpy((void *)0xa0000, (void *)&baGdt[0], 0x50);

		// prep the Linux startup struct

	setup( (void *)0x90000, (void *)0x03000000, (void *)dwInitrdSize, config.szAppend);

	{
		int nAta=0;
		if(tsaHarddiskInfo[0].m_bCableConductors == 80) {
			if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&2) nAta=1;
			if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&4) nAta=2;
			if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&8) nAta=3;
			if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&16) nAta=4;
			if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&32) nAta=5;
		} else {
			// force the HDD into a good mode 0x40 ==UDMA | 2 == UDMA2
			nAta=2; // best transfer mode without 80-pin cable
		}
//		nAta=1;
		BootIdeSetTransferMode(0, 0x40 | nAta);
//		BootIdeSetTransferMode(0, 0x04);
	}


		__asm __volatile__ (

	"cli \n"

		// kill the cache

	"mov %cr0, %eax \n"
	"orl	$0x60000000, %eax \n"
	"andl	$0x7fffffff, %eax \n" // turn off paging
	"mov	%eax, %cr0 \n"
	"wbinvd \n"

	"mov	%cr3, %eax \n"
	"mov	%eax, %cr3 \n"

	"movl	$0x2ff, %ecx \n"
		"xor		%eax, %eax \n"
		"xor		%edx, %edx \n"
		"wrmsr \n"

		// Init the MTRRs for Ram and BIOS

		"movl	$0x200, %ecx \n" // from MCPX 0xee action

			// MTRR for Memory-mapped regs 0xf8xxxxxx .. 0xffxxxxxx - uncached

		"movl	$0x00000000, %edx \n" // 0x00
		"movl	$0xF8000000, %eax  \n"// == no cache
		"wrmsr \n"
		"inc		%ecx \n"
			// MASK0 set to 0xff0000[000] == 16M
		"movl	$0x0000000f, %edx \n" // 0x0f
		"movl	$0xF8000800, %eax \n"  // 0xff000800
		"wrmsr \n"
		"inc %ecx \n"

					// MTRR for shadow video memory

		"movl	$0x00000000, %edx  \n"// 0x00
		"movl	$0xf0000005, %eax  \n"// == Cacheable
		"wrmsr \n"
		"inc		%ecx \n"
			// MASK0 set to 0xfffC00[000] == 4M
		"movl	$0x0000000f, %edx  \n"// 0x0f
		"movl	$0xff000800, %eax   \n"// 0xffC00800
		"wrmsr \n"
		"inc %ecx \n"

							// MTRR for shadow video memory

		"movl	$0x00000000, %edx  \n"// 0x00
		"movl	$0xc0000004, %eax  \n"// == Cacheable
		"wrmsr \n"
		"inc		%ecx \n"
			// MASK0 set to 0xfffC00[000] == 4M
		"movl	$0x0000000f, %edx  \n"// 0x0f
		"movl	$0xc0000800, %eax   \n"// 0xffC00800
		"wrmsr \n"
		"inc %ecx \n"


			// MTRR for main RAM

		"movl	$0x00000000, %edx  \n"// 0x00
		"movl	$0x00000006, %eax  \n"// == Cacheable
		"wrmsr \n"
		"inc		%ecx \n"
			// MASK0 set to 0xfffC00[000] == 4M
		"movl	$0x0000000f, %edx  \n"// 0x0f
		"movl	$0xfc000800, %eax   \n"// 0xffC00800
		"wrmsr \n"
		"inc %ecx \n"

		"xor		%eax, %eax \n"
		"xor		%edx, %edx \n"
"cleardown: \n"
		"wrmsr \n"
		"inc	%ecx \n"
		"cmpb	$0xf, %cl \n"
		"jna cleardown \n"

// madeline

		"movl	$0x2ff, %ecx \n"
		"movl	$0x806, %eax  \n"// edx still 0, default to CACHEABLE Enable MTRRs
		"wrmsr \n"

			/* turn on normal cache, TURN OFF PAGING */

		"movl	%cr0, %eax \n"
		"mov %eax, %ebx \n"
		"andl	$0x1FFFFFFF,%eax \n"
		"movl	%eax, %cr0 \n"

	"movl $0x90000, %esi        \n"
	"xor %ebx, %ebx \n"
	"xor %eax, %eax \n"
	"xor %ecx, %ecx \n"
	"xor %edx, %edx \n"
	"xor %edi, %edi \n"

	"lgdt 0xa0030 \n"
  "ljmp $0x10, $0x100000 \n"
);


//		DumpAddressAndData(0x100000, (void *)0x100000, 1024);

	while(1);

#if 0
//		int n=0;
//		DWORD dwCsum1=0, dwCsum2=0;
//		BYTE *pb1=(BYTE *)0xf0000, *pb2=(BYTE*)&rombios;
  //  printk("  Copying BIOS into RAM...\n");
extern char rombios[1];
void * memcpy(void *dest, const void *src,  size_t size);

	//	I2cSetFrontpanelLed(0x77);

			// copy the 64K 16-bit BIOS code into memory at 0xF0000, this is where a BIOS
			// normally appears in 16-bit mode


    memcpy((void *)0xf0000, ((BYTE *)&rombios), 0x10000);


//		for(n=0;n<0x10000;n++) {
//			dwCsum1+=*pb1++; dwCsum2+=*pb2++;
//		}

    	// LEDs to yellow

		// if(*((BYTE *)0xffff0)==0xe9)
//		if(dwCsum1==dwCsum2) { I2cSetFrontpanelLed(0xff); } else { I2cSetFrontpanelLed(0x01); }

 //   printk("  done.  Running BIOS...\n");

			// copy a 16-bit LJMP stub into a safe place that is visible in 16-bit mode
			// (the BIOS isn't visible in 1MByte address space)

			__asm __volatile__ (
		"mov  $code_start, %esi \n"
		"mov  $0x600, %edi       \n"
		"mov  $0x100, %ecx   \n"
		"rep movsb            \n"
		"wbinvd \n"

			// prep the segment regs with the right GDT entry for 16-bit access
			// then LJMP to a 16-bit GDT entry, at the stub we prepared earlier
			// the stub code does some CPU mode setting then LJMPs to F000:FFF0
			// which starts off the BIOS as if it was a reset

		"mov  $0x28, %ax     \n"
		"mov  %ax, %ds      \n"
		"mov  %ax, %es      \n"
		"mov  %ax, %fs      \n"
		"mov  %ax, %gs      \n"
		"mov  %ax, %ss      \n"
		"ljmp $0x20, $0x600       \n"

		"code_start:          \n"  // this is the code copied to the 16-bit stub at 0x600
		".code16 \n"

		"movl %cr0, %eax     \n" /* 16-bit */
		"andl $0xFFFFFFFE, %eax \n"  // this was not previously ANDL, generated 16-bit AND despite EAX
		"movl %eax, %cr0    \n"

		"mov  $0x8000, %ax      \n"
		"mov  %ax, %sp \n"
		"mov  $0x0000, %ax      \n"
		"mov  %ax, %ss \n"
		"mov $0x0000,%ax \n"
		"mov  %ax, %ds \n"
		"mov  %ax, %es \n"
#if 0
		"mov $0xc004, %dx \n"
		"mov $0x20, %al \n"
		"out %al, %dx \n"
		"mov $0xc008, %dx \n"
		"mov $0x8, %al \n"
		"out %al, %dx \n"
		"mov $0xc006, %dx \n"
		"mov $0xa6, %al \n"
		"out %al, %dx \n"
		"mov $0xc006, %dx \n"
		"in %dx,%al \n"
		"mov $0xc002, %dx \n"
		"mov $0x1a, %al \n"
		"out %al, %dx \n"
		"mov $0xc000, %dx \n"

		"ledspin: in %dx, %al ; cmp $0x10, %al ; jnz ledspin \n"

		"mov $0xc004, %dx \n"
		"mov $0x20, %al \n"
		"out %al, %dx \n"
		"mov $0xc008, %dx \n"
		"mov $0x7, %al \n"
		"out %al, %dx \n"
		"mov $0xc006, %dx \n"
		"mov $0x1, %al \n"
		"out %al, %dx \n"
		"mov $0xc006, %dx \n"
		"in %dx,%al \n"
		"mov $0xc002, %dx \n"
		"mov $0x1a, %al \n"
		"out %al, %dx \n"
		"mov $0xc000, %dx \n"

		"ledspin1: in %dx, %al ; cmp $0x10, %al ; jnz ledspin1 \n"

		"jmp ledspin1 \n"
#endif
		".byte 0xea          \n"  // long jump to reset vector at 0xf000:0xfff0
		".word 0xFFF0 \n"
		".word 0xF000 \n"
		".code32 \n"
		);
#endif
	}
