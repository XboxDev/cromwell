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
#include "BootFlash.h"
#include "cpu.h"
 
#include "config.h"

extern EEPROMDATA eeprom;

#include "BootUsbOhci.h"
extern volatile ohci_t usbcontroller[2];
extern volatile AC97_DEVICE ac97device;

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


enum {
	ICON_FATX = 0,
	ICON_NATIVE,
	ICON_CD,
	ICON_SETUP,
	ICONCOUNT // always last
};

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

const int naChimeFrequencies[] = {
	329, 349, 392, 440
};

void BootPrintConfig(CONFIGENTRY *config) {
	printk("  Bootconfig : Kernel  %s\n", config->szKernel);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Initrd  %s\n", config->szInitrd);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Command %s\n", config->szAppend);
	VIDEO_ATTR=0xffa8a8a8;
}

// if fJustTestingForPossible is true, returns 0 if this kind of boot not possible, 1 if it is worth trying

int BootLodaConfigNative(int nActivePartition, CONFIGENTRY *config, bool fJustTestingForPossible) {
	DWORD dwConfigSize=0;
	char szGrub[256+4];
        
        memset(szGrub,0,256+4);
        
	memset((BYTE *)0x90000,0,4096);

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
				if(!fJustTestingForPossible) printk("linuxboot.cfg not found, using defaults\n");
	} else {
		if(fJustTestingForPossible) return 1; // if there's a linuxboot.cfg it must be worth trying to boot
		{
			int nLen=grub_read((void *)0x90000, filemax);
			if(nLen>0) { ((char *)0x90000)[nLen]='\0'; }  // needed to terminate incoming string, reboot in ParseConfig without it
		}
		ParseConfig((char *)0x90000,config,&eeprom);
		BootPrintConfig(config);
		printf("linuxboot.cfg is %d bytes long.\n", dwConfigSize);
	}
	grub_close();

	//strcpy(&szGrub[4], config->szKernel);
        _strncpy(&szGrub[4], config->szKernel,sizeof(config->szKernel));

		// Force a particular kernel to be loaded here
		// leave commented out normally
//	strcpy(&szGrub[4], "/boot/vmlinuz-2.4.20-xbox");
//	strcpy(&szGrub[4], "/boot/vmlinuz-ag");

	nRet=grub_open(szGrub);

	if(nRet!=1) {
		if(fJustTestingForPossible) return 0;
		printk("Unable to load kernel, Grub error %d\n", errnum);
		while(1) ;
	}
	if(fJustTestingForPossible) return 1; // if there's a default kernel it must be worth trying to boot

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
//		strcpy(&szGrub[4], config->szInitrd); 
 		_strncpy(&szGrub[4], config->szInitrd,sizeof(config->szInitrd));
		nRet=grub_open(szGrub);
		if(filemax==0) {
			printf("Empty file\n"); while(1);
		}
		if( (nRet!=1) || (errnum)) {
			printk("Unable to load initrd, Grub error %d\n", errnum);
			while(1) ;
		}
		printk(" - %d bytes\n", filemax);
		dwInitrdSize=grub_read((void *)INITRD_POS, filemax);
		grub_close();
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
	}

	return true;
}

int BootLodaConfigFATX(CONFIGENTRY *config, bool fJustTestingForPossible) {

	static FATXPartition *partition = NULL;
	static FATXFILEINFO fileinfo;
	static FATXFILEINFO infokernel;
	static FATXFILEINFO infoinitrd;

	memset((BYTE *)0x90000,0,4096);
	memset(&fileinfo,0x00,sizeof(fileinfo));
	memset(&infokernel,0x00,sizeof(infokernel));
	memset(&infoinitrd,0x00,sizeof(infoinitrd));
	
	if(!fJustTestingForPossible) printk("Loading linuxboot.cfg form FATX\n");
	partition = OpenFATXPartition(0,
			SECTOR_STORE,
			STORE_SIZE);
	if(partition != NULL) {
		if(!LoadFATXFile(partition,"/linuxboot.cfg",&fileinfo) ) {
			if(!fJustTestingForPossible) printk("linuxboot.cfg not found, using defaults\n");
		} else {
			ParseConfig(fileinfo.buffer,config,&eeprom);
		}
	} else {
		if(fJustTestingForPossible) return 0;
	}

	if(!fJustTestingForPossible) BootPrintConfig(config);
	if(! LoadFATXFile(partition,config->szKernel,&infokernel)) {
		if(fJustTestingForPossible) return 0;
		printk("Error loading kernel %s\n",config->szKernel);
		while(1);
	} else {
		if(fJustTestingForPossible) return 1; // worth trying, since the filesystem and kernel exists
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
			memcpy((BYTE *)INITRD_POS,infoinitrd.buffer,infoinitrd.fileSize);	// moving the initrd to its final location
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

	DWORD dwConfigSize=0, dw;
	BYTE ba[2048],baBackground[640*64*4]; 
	

#ifndef IS_XBE_CDLOADER
	BYTE b;
#endif
	BYTE bCount=0, bCount1;
	int n;
	
	DWORD dwY=VIDEO_CURSOR_POSY;
	DWORD dwX=VIDEO_CURSOR_POSX;

	memset((BYTE *)0x90000,0,4096);
	
	BootVideoBlit(
		(DWORD *)&baBackground[0], 640*4,
		(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
		currentvideomodedetails.m_dwWidthInPixels*4, 64
	);

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
//		printk("\2BootResetAction 0x%08X\2\n",&BootResetAction);
//		printk("%08x",(*((DWORD * )0xfd600800)));
		printk("\2Please insert CD\n\2");
		
		for(n=0;n<1000000;n++) { ; }
	}
#endif

	VIDEO_ATTR=0xffffffff;

	VIDEO_CURSOR_POSX=dwX;
	VIDEO_CURSOR_POSY=dwY;
	BootVideoBlit(
		(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
		currentvideomodedetails.m_dwWidthInPixels*4, (DWORD *)&baBackground[0], 640*4, 64
	);

		// wait until the media is readable

	{
		bool fMore=true, fOkay=true;
		while(fMore) {
			if(BootIdeReadSector(1, &ba[0], 0x10, 0, 2048)) { // starts at 16

				VIDEO_CURSOR_POSX=dwX;
				VIDEO_CURSOR_POSY=dwY;
				bCount++;
				bCount1=bCount; if(bCount1&0x80) { bCount1=(-bCount1)-1; }

				VIDEO_ATTR=0xff000000|(((bCount1)+64)<<16)|(((bCount1>>1)+128)<<8)|(((bCount1)+128)) ;

				printk("\2Waiting for drive\2\n");
				for(n=0;n<3000000;n++) { ; }

// If cromwell acts as an xbeloader it falls back to try reading
//	the config file from fatx

			} else {  // read it successfully
				fMore=false;
				fOkay=true;
//				printk("\n");
//				printk("Saw successful read\n");
			}
		}

		if(!fOkay) {
			void BootFiltrorDebugShell();
			printk("cdrom unhappy\n");
#if INCLUDE_FILTROR
			BootFiltrorDebugShell();
#endif
			while(1);
		} else {
			printk("\n");
//			printk("HAPPY\n");
		}
	}

	VIDEO_CURSOR_POSX=dwX;
	VIDEO_CURSOR_POSY=dwY;
	BootVideoBlit(
		(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
		currentvideomodedetails.m_dwWidthInPixels*4, (DWORD *)&baBackground[0], 640*4, 64
	);
 
        
//	printk("STILL HAPPY\n");

	{
		ISO_PRIMARY_VOLUME_DESCRIPTOR * pipvd = (ISO_PRIMARY_VOLUME_DESCRIPTOR *)&ba[0];
		char sz[64];
		memset(&sz,0x00,sizeof(sz));
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
// Uncomment the following to test flashing   
 #if 0   
         dwConfigSize=BootIso9660GetFile("/IMAGE.BIN", (BYTE *)0x100000, 0x100000, 0x0);   
         printk("Image size: %i\n", dwConfigSize);   
         printk("Error code: $i\n", BootReflashAndReset((BYTE*) 0x100000, (DWORD) 0, (DWORD) dwConfigSize));   
 #endif 

	printk("  Loading linuxboot.cfg from CDROM... \n");
	dwConfigSize=BootIso9660GetFile("/linuxboot.cfg", (BYTE *)0x90000, 0x800, 0x0);

	if(((int)dwConfigSize)<0) { // not found, try mangled 8.3 version
		dwConfigSize=BootIso9660GetFile("/LINUXBOO.CFG", (BYTE *)0x90000, 0x800, 0x0);
		if(((int)dwConfigSize)<0) { // has to be there on CDROM
			printk("Unable to find it, halting\n");
			while(1) ;
		}
	}

	ParseConfig((char *)0x90000,config,&eeprom);
	BootPrintConfig(config);

	dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE *)0x90000, 0x400, 0x0);

	if(((int)dwKernelSize)<0) { // not found, try 8.3
		strcpy(config->szKernel, "/VMLINUZ.");
		dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE *)0x90000, 0x400, 0x0);
		if(((int)dwKernelSize)<0) { 
			strcpy(config->szKernel, "/VMLINUZ_.");
			dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE *)0x90000, 0x400, 0x0);
			if(((int)dwKernelSize)<0) { printk("Not Found, error %d\nHalting\n", dwKernelSize); while(1) ; }
		}
	}
	nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
	dw=BootIso9660GetFile(config->szKernel, (void *)0x90400, nSizeHeader-0x400, 0x400);
	if(((int)dwKernelSize)<0) { printk("Load prob 2, error %d\nHalting\n", dw); while(1) ; }
	dwKernelSize+=dw;
	dw=BootIso9660GetFile(config->szKernel, (void *)0x00100000, 4096*1024, nSizeHeader);
	if(((int)dwKernelSize)<0) { printk("Load prob 3, error %d\nHalting\n", dw); while(1) ; }
	dwKernelSize+=dw;
	printk(" -  %d bytes...\n", dwKernelSize);

	if( (_strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from CDROM", config->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;

		dwInitrdSize=BootIso9660GetFile(config->szInitrd, (void *)INITRD_POS, 4096*1024, 0);
		if((int)dwInitrdSize<0) { // not found, try 8.3
			strcpy(config->szInitrd, "/INITRD.");
			dwInitrdSize=BootIso9660GetFile(config->szInitrd, (void *)INITRD_POS, 4096*1024, 0);
			if((int)dwInitrdSize<0) { // not found, try 8.3
				strcpy(config->szInitrd, "/INITRD_I.");
				dwInitrdSize=BootIso9660GetFile(config->szInitrd, (void *)INITRD_POS, 4096*1024, 0);
				if((int)dwInitrdSize<0) { printk("Not Found, error %d\nHalting\n", dwInitrdSize); while(1) ; }
			}
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


#ifdef FLASH
void BootFlashConfirm()
{
	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray

	printk("Close the DVD tray to confirm flash\n");
	while(1) ;
}
#endif

void BootIcons(int nXOffset, int nYOffset, int nTextOffsetX, int nTextOffsetY) {
	icon[ICON_FATX].nDestX = nXOffset + 120;
	icon[ICON_FATX].nDestY = nYOffset - 74;
	icon[ICON_FATX].nSrcX = ICON_WIDTH;
	icon[ICON_FATX].nSrcLength = ICON_WIDTH;
	icon[ICON_FATX].nSrcHeight = ICON_HEIGH;
	icon[ICON_FATX].nTextX = (nTextOffsetX+60)<<2;;
	icon[ICON_FATX].nTextY = nTextOffsetY;

	icon[ICON_NATIVE].nDestX = nXOffset + 245;
	icon[ICON_NATIVE].nDestY = nYOffset - 74;
	icon[ICON_NATIVE].nSrcX = ICON_WIDTH;
	icon[ICON_NATIVE].nSrcLength = ICON_WIDTH;
	icon[ICON_NATIVE].nSrcHeight = ICON_HEIGH;
	icon[ICON_NATIVE].nTextX = (nTextOffsetX+240)<<2;;
	icon[ICON_NATIVE].nTextY = nTextOffsetY;

	icon[ICON_CD].nDestX = nXOffset + 350;
	icon[ICON_CD].nDestY = nYOffset - 74;
	icon[ICON_CD].nSrcX = ICON_WIDTH*2;
	icon[ICON_CD].nSrcLength = ICON_WIDTH;
	icon[ICON_CD].nSrcHeight = ICON_HEIGH;
	icon[ICON_CD].nTextX = (nTextOffsetX+350)<<2;
	icon[ICON_CD].nTextY = nTextOffsetY;

	icon[ICON_SETUP].nDestX = nXOffset + 440;
	icon[ICON_SETUP].nDestY = nYOffset - 74;
	icon[ICON_SETUP].nSrcX = ICON_WIDTH*3;
	icon[ICON_SETUP].nSrcLength = ICON_WIDTH;
	icon[ICON_SETUP].nSrcHeight = ICON_HEIGH;
	icon[ICON_SETUP].nTextX = (nTextOffsetX+440)<<2;
	icon[ICON_SETUP].nTextY = nTextOffsetY;
}

void BootStartBiosDoIcon(ICON *icon, BYTE bOpaqueness)
{

		BootVideoJpegBlitBlend(
			(DWORD *)(FRAMEBUFFER_START+(icon->nDestX<<2)+(currentvideomodedetails.m_dwWidthInPixels*4*icon->nDestY)),
			currentvideomodedetails.m_dwWidthInPixels * 4, // dest bytes per line
			&jpegBackdrop, // source jpeg object
			(DWORD *)(((BYTE *)jpegBackdrop.m_pBitmapData)+(icon->nSrcX *jpegBackdrop.m_nBytesPerPixel)),
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
	static CONFIGENTRY config;
	char szGrub[256+4];
#ifdef MENU
	int nTempCursorResumeX, nTempCursorResumeY, nTempStartMessageCursorX, nTempStartMessageCursorY;
#endif
	int nIcon = ICONCOUNT; 
	

//	BootPciInterruptEnable();

#ifdef DEBUG_MODE
	memset((DWORD *)(FRAMEBUFFER_START),0,1024*1024*4);
	while(1);
#endif
	memset(szGrub,0x00,sizeof(szGrub));
	memset(&config,0,sizeof(CONFIGENTRY));

	szGrub[0]=0xff; szGrub[1]=0xff; szGrub[2]=nActivePartition; szGrub[3]=0x00;

	errnum=0; boot_drive=0; saved_drive=0; saved_partition=0x0001ffff; buf_drive=-1;
	current_partition=0x0001ffff; current_drive=0xff; buf_drive=-1; fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;


#ifndef IS_XBE_CDLOADER
#ifdef MENU
	{
		int nTempCursorX, nTempCursorY, nTempEntryX, nTempEntryY;
		int nModeDependentOffset=(currentvideomodedetails.m_dwWidthInPixels-640)/2;  // icon offsets computed for 640 modes, retain centering in other modes
		AUDIO_ELEMENT_SINE aesIconSound;

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


		for(nIcon = 0; nIcon < ICONCOUNT;nIcon ++) {
			BootStartBiosDoIcon(&icon[nIcon], TRANPARENTNESS);
		}

		{
			int nShowSelect = false;

			for(nIcon = 0; nIcon < ICONCOUNT;nIcon ++) {
				traystate = ETS_OPEN_OR_OPENING;
				nShowSelect = false;
				VIDEO_CURSOR_POSX=icon[nIcon].nTextX;
				VIDEO_CURSOR_POSY=icon[nIcon].nTextY;

				switch(nIcon){

					case ICON_FATX:
						if(nFATXPresent) {
							strcpy(config.szKernel, "/vmlinuz"); // fatx default kernel, looked for to detect fs
							if(!BootLodaConfigFATX(&config, true)) continue;  // only bother with it if the filesystem exists
							printk("/linuxboot.cfg from FATX\n");
							nShowSelect = true;
						} else {
							continue;
						}
						break;

					case ICON_NATIVE:
						if(nDrive != 1) {
							strcpy(config.szKernel, "/boot/vmlinuz");  // Ext2 default kernel, looked for to detect fs
							if(!BootLodaConfigNative(nActivePartition, &config, true)) continue; // only bother with it if the filesystem exists
							printk("/dev/hda\n");
							nShowSelect=true;
						} else {
							continue;
						}
						break;

					case ICON_CD:
						printk("/dev/hdb\n");
						nShowSelect = true;  // always might want to boot from CD
						break;

					case ICON_SETUP:
						printk("Flash\n");
						nShowSelect = true;
						break;
				}
				if(nShowSelect) {

					DWORD dwTick=BIOS_TICK_COUNT;
					
					{
						DestructAUDIO_ELEMENT_SINE(&ac97device, &aesIconSound); // harmless if not yet constructed or attached
						ConstructAUDIO_ELEMENT_SINE(&aesIconSound, naChimeFrequencies[nIcon]);  // constructed silent
						aesIconSound.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[0]=0x1000;
						aesIconSound.m_paudioelement.m_dwVolumeSustainRate=0x2800;
						BootAudioAttachAudioElement(&ac97device, (AUDIO_ELEMENT *)&aesIconSound);
					}       


					while(
						(BIOS_TICK_COUNT<(dwTick+DELAY_TICKS)) &&
						(traystate==ETS_OPEN_OR_OPENING)
					) {
						BootStartBiosDoIcon(
							&icon[nIcon], OPAQUENESS-((OPAQUENESS-TRANPARENTNESS)
							 *(BIOS_TICK_COUNT-dwTick))/DELAY_TICKS
						);
					}
					dwTick=BIOS_TICK_COUNT;


					if(traystate!=ETS_OPEN_OR_OPENING) {  // tray went in, specific choice made
						{
							DestructAUDIO_ELEMENT_SINE(&ac97device, &aesIconSound); // harmless if not yet constructed or attached
						}
						VIDEO_CURSOR_POSX=icon[nIcon].nTextX;
						VIDEO_CURSOR_POSY=icon[nIcon].nTextY;
						RecoverMbrArea();
						BootStartBiosDoIcon(&icon[nIcon], SELECTED);
						break;
					}

						// timeout

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


	{
	  	// turn off USB

		#ifdef DO_USB
		BootUsbTurnOff((ohci_t *)&usbcontroller[0]);
		BootUsbTurnOff((ohci_t *)&usbcontroller[1]);
		#endif
			// silence the audio
        	
        	BootAudioSilence(&ac97device);
                
		
	
	}

#ifdef DEFAULT_FATX
nDrive=0;
#endif

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
			
			
			//Dumping some PCI registers
			//DWORD PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off);
                 /*
			for (a=0x10;a<0xF0;a=a+16) {
				tmp = PciReadDword(0,0,0,a);
				printk("%02X :-> %08x  ",a,tmp);
	
				tmp = PciReadDword(0,0,0,a+4);				
				printk("%02X :-> %08x  ",a+4,tmp);

				tmp = PciReadDword(0,0,0,a+4+4);				
				printk("%02X :-> %08x  ",a+4+4,tmp);

				tmp = PciReadDword(0,0,0,a+4+4+4);				
				printk("%02X :-> %08x \n",a+4+4+4,tmp);
	


				
			}
		*/	
			
		/*
		{
		unsigned int temp=0;
		while (1) {
		VIDEO_CURSOR_POSX=50;
		VIDEO_CURSOR_POSY=BIOS_TICK_COUNT%400;
		VIDEO_ATTR=0xff9f9fbf;                     
		//printk(" %08X \n",BIOS_TICK_COUNT);
		printk(" %08X \n",VIDEO_VSYNC_DIR);
		if (temp !=BIOS_TICK_COUNT) {
			
		printk(" %08X \n",VIDEO_VSYNC_DIR);
		//	printk(" %08X \n",BIOS_TICK_COUNT);
			VIDEO_ATTR=0xff8888a8;
			temp = BIOS_TICK_COUNT; 
			}		
		
			
		}	
		}	
		*/
		
		//printk("%08X",BootResetAction)	;    
		/*
		{
		int a;
		unsigned char SHA1_result[SHA1HashSize];
		struct SHA1Context context;
	        int i;
	        VIDEO_CURSOR_POSY=150;
	        for (a=0;a<20;a++) {
	        	VIDEO_CURSOR_POSX=0;
       			SHA1Reset(&context);   
			SHA1Input(&context,(void*)0xFFF00000,0x80000);
			SHA1Result(&context,SHA1_result);	
			for (i=0;i<20;i++) printk("%02x",SHA1_result[i]);
			printk("\n");
			}
		}
	        */
		/*
		{
		unsigned char SHA1_result[SHA1HashSize];
		struct SHA1Context context;
	        int i;
	        VIDEO_CURSOR_POSY=200;
	        VIDEO_CURSOR_POSX=0;
       		SHA1Reset(&context);   
		SHA1Input(&context,(void*)0x03B00000,0x40000);
		SHA1Result(&context,SHA1_result);	
		for (i=0;i<20;i++) printk("%02x",SHA1_result[i]);

	        VIDEO_CURSOR_POSY=220;
	        VIDEO_CURSOR_POSX=0;
       		SHA1Reset(&context);   
		SHA1Input(&context,(void*)0x03B40000,0x40000);
		SHA1Result(&context,SHA1_result);	
		for (i=0;i<20;i++) printk("%02x",SHA1_result[i]);

	        VIDEO_CURSOR_POSY=240;
	        VIDEO_CURSOR_POSX=0;
       		SHA1Reset(&context);   
		SHA1Input(&context,(void*)0x03B80000,0x40000);
		SHA1Result(&context,SHA1_result);	
		for (i=0;i<20;i++) printk("%02x",SHA1_result[i]);

	        VIDEO_CURSOR_POSY=280;
	        VIDEO_CURSOR_POSX=0;
       		SHA1Reset(&context);   
		SHA1Input(&context,(void*)0x03Bc0000,0x40000);
		SHA1Result(&context,SHA1_result);	
		for (i=0;i<20;i++) printk("%02x",SHA1_result[i]);
			
		}
		 */
		/* 
		{
		int i;			
		
		
		
			for (i=0;i<0x60;i++) {
			 
				if ((i%8)==0) printk("\n");
				printk("(%02x)->%02x  ",i,IoInputByte(0x8000+i));	
				}
			
			}	
			
		*/
		
		/*
		{
		extern int _start_backdrop, _end_backdrop;
			extern void IntHandler1(void);
			extern void IntHandler2(void);
			extern void IntHandler3(void);
			extern void IntHandler4(void);
			extern void IntHandlerException0(void);
			void IntHandler4C(void);
			printk("%08X\n",IntHandler1);
			printk("%08X\n",IntHandlerException0);
			printk("%08X\n",BootResetAction);
			printk("%08X\n",IntHandler4C);
			printk("%08X\n",_start_backdrop);
			printk("%08X\n",_end_backdrop);
		}

		while(1);
		*/
//		display_cpuid_update_microcode();
//		while(1);
		} 
		
		
	}


	if(nIcon == ICON_FATX) {
		strcpy(config.szAppend, "init=/linuxrc root=/dev/ram0 pci=biosirq kbd-reset"); // default
		strcpy(config.szKernel, "/vmlinuz");
		strcpy(config.szInitrd, "/initrd");
	} else {
		strcpy(config.szAppend, "root=/dev/hda2 devfs=mount kbd-reset"); // default
		strcpy(config.szKernel, "/boot/vmlinuz");
		strcpy(config.szInitrd, "/boot/initrd");
	}


	switch(nIcon) {
		case ICON_FATX:
			BootLodaConfigFATX(&config, false);
			break;
		case ICON_NATIVE:
			BootLodaConfigNative(nActivePartition, &config, false);
			break;
		case ICON_CD:
			BootLodaConfigCD(&config);
			break;
		case ICON_SETUP:
#ifndef XBE
#ifdef FLASH
			BootVideoClearScreen(&jpegBackdrop, nTempStartMessageCursorY, nTempCursorResumeY+100);
			VIDEO_CURSOR_POSY=nTempStartMessageCursorY;
			VIDEO_CURSOR_POSX=0;
			BootFlashConfirm();
#endif
#endif
			break;
		default:
			printk("Selection not implemented\n");
			break;
	}


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
    
        I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );


	// we have to copy the GDT to a safe place, because one of the first
	// things Linux is going to do is reinint the paging, making the BIOS
	// inaccessible and so crashing if we leave the GDT in BIOS
        
    
        memset((void *)0xa0000,0x00,0x100);
        memcpy((void *)0xa0000, (void *)&baGdt[0], sizeof(baGdt));
	// prep the Linux startup struct

	setup( (void *)0x90000, (void *)INITRD_POS, (void *)dwInitrdSize, config.szAppend);
        
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
		BootIdeSetTransferMode(1, 0x40 | nAta);
//		BootIdeSetTransferMode(0, 0x04);
	}

		// orangeness, people seem to like that colour

	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);


		__asm __volatile__ (

	"cli \n"

	// kill the cache

	"mov %cr0, %eax \n"
	"orl	$0x60000000, %eax \n"
	"andl	$0x7fffffff, %eax \n" 		// turn off paging
	"mov	%eax, %cr0 \n"
	"wbinvd \n"

	"mov	%cr3, %eax \n"
	"mov	%eax, %cr3 \n"

	// Clear Memory Defines
	"movl	$0x2ff, %ecx \n"
	"xor	%eax, %eax \n"
	"xor	%edx, %edx \n"
	"wrmsr \n"

	// Init the MTRRs for Ram and BIOS
        
        // MTRR for Memory-mapped regs 0xf8xxxxxx .. 0xffxxxxxx - uncached
	"movl	$0x200, %ecx \n"		// Base 0
        "movl	$0x00000000, %edx \n" 		// 0x00
	"movl	$0xF8000000, %eax  \n"		// == no cache
	"wrmsr \n"
	
	"movl	$0x201, %ecx \n"		// Mask 0
						// MASK0 set to 0xff0000[000] == 16M
	"movl	$0x0000000f, %edx \n" 		// 0x0f
	"movl	$0xF8000800, %eax \n"  		// 0xff000800
	"wrmsr \n"
	
	
	"movl	$0x202, %ecx \n"		// Base 1
						// MTRR for shadow video memory
        "movl	$0x00000000, %edx  \n"		// 0x00
	"movl	$0xf0000005, %eax  \n"		// == Cacheable
	"wrmsr \n"
	
	"movl	$0x203, %ecx \n"		// Mask 1
						// MASK set to 0xfffC00[000] == 4M
	"movl	$0x0000000f, %edx  \n"		// 0x0f
	"movl	$0xff000800, %eax   \n"		// 0xffC00800
	"wrmsr \n"
	
	
	"movl	$0x204, %ecx \n"		// Base 2
						// MTRR for shadow video memory
	"movl	$0x00000000, %edx  \n"		// 0x00
	"movl	$0xc0000004, %eax  \n"		// == Cacheable
	"wrmsr \n"
	
	"movl	$0x205, %ecx \n"       		// Mask 2
						// MASK set to 0xfffC00[000] == 4M
	"movl	$0x0000000f, %edx  \n"		// 0x0f
	"movl	$0xc0000800, %eax   \n"		// 0xffC00800
	"wrmsr \n"                              
	
	"movl	$0x206, %ecx \n"		// Base 3
						// MTRR for main RAM
	"movl	$0x00000000, %edx  \n"		// 0x00
	"movl	$0x00000006, %eax  \n"		// == Cacheable
	"wrmsr \n"
	
	"movl	$0x207, %ecx \n"		// Mask 3
						// MASK set to 0xfffC00[000] == 4M
	"movl	$0x0000000f, %edx  \n"		// 0x0f
	"movl	$0xfc000800, %eax   \n"		// 0xffC00800
	"wrmsr \n"
	
	
        "xor	%eax, %eax  \n"
	"xor	%edx, %edx  \n"
	
	"movl	$0x208, %ecx  \n"		// IA32_MTRR_PHYS Base 4
        "wrmsr  \n"                                                   
	"movl	$0x209, %ecx  \n"		// IA32_MTRR_PHYS_MASK 4
        "wrmsr  \n"                                                   

	"movl	$0x20a, %ecx  \n"		// IA32_MTRR_PHYS Base 5
        "wrmsr  \n"                                                   
	"movl	$0x20b, %ecx  \n"		// IA32_MTRR_PHYS_MASK 5
        "wrmsr  \n"                                                   

	"movl	$0x20c, %ecx  \n"		// IA32_MTRR_PHYS Base 6
        "wrmsr  \n"                                                   
	"movl	$0x20d, %ecx  \n"		// IA32_MTRR_PHYS_MASK 6
        "wrmsr  \n"                                                   

	"movl	$0x20e, %ecx  \n"		// IA32_MTRR_PHYS Base 7
        "wrmsr  \n"                                                   
	"movl	$0x20f, %ecx  \n"		// IA32_MTRR_PHYS_MASK 7
        "wrmsr  \n"                                                   
        
        
// madeline

	"xor %edx, %edx \n"
	"movl	$0x2ff, %ecx \n"
	"movl	$0x806, %eax  \n"		// default to CACHEABLE Enable MTRRs
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

	"lgdt 0xa0030\n"
  "ljmp $0x10, $0x100000\n"
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
