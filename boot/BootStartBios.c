/**************************************************************************/
/*  2003-07-04 georg@acher.org  added USB input demo                       *
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
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"

#include "config.h"

extern EEPROMDATA eeprom;

extern volatile AC97_DEVICE ac97device;

#undef strcpy

unsigned long saved_drive;
unsigned long saved_partition;
grub_error_t errnum;
unsigned long boot_drive;

extern unsigned int CACHE_VSYNC_WRITEBACK;
extern int nTempCursorMbrX, nTempCursorMbrY;

extern unsigned long current_drive;
char * strcpy(char *sz, const char *szc);
int _strncmp(const char *sz1, const char *sz2, int nMax);

void setup(void* KernelPos, void* PhysInitrdPos, unsigned long InitrdSize, const char* kernel_cmdline);

extern int etherboot(void);

static int nRet;
static DWORD dwKernelSize, dwInitrdSize;
static int nSizeHeader;


  
enum {
	ICON_FATX = 0,
	ICON_NATIVE,
	ICON_CD,
	ICON_FLASH,
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
	int nEnabled;
	int nSelected;
	char *szCaption;
} ICON;

ICON icon[ICONCOUNT];

const int naChimeFrequencies[] = {
	329, 349, 392, 440
};

void BootPrintConfig(CONFIGENTRY *config) {
	int CharsProcessed=0, CharsSinceNewline=0, Length=0;
	char c;
	printk("  Bootconfig : Kernel  %s \n", config->szKernel);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Initrd  %s \n", config->szInitrd);
	VIDEO_ATTR=0xffa8a8a8;
	printk("  Bootconfig : Appended arguments :\n");
	Length=strlen(config->szAppend);
	while (CharsProcessed<Length) {
		c = config->szAppend[CharsProcessed];
		CharsProcessed++;
		CharsSinceNewline++;
		if ((CharsSinceNewline>50 && c==' ') || CharsSinceNewline>65) {
			printk("\n");
			if (CharsSinceNewline>25) printk("%c",c);
			CharsSinceNewline = 0;
		} 
		else printk("%c",c);
	}
	printk("\n");
	VIDEO_ATTR=0xffa8a8a8;
}



// if fJustTestingForPossible is true, returns 0 if this kind of boot not possible, 1 if it is worth trying

int BootLoadConfigNative(int nActivePartition, CONFIGENTRY *config, bool fJustTestingForPossible) {
	DWORD dwConfigSize=0;
	char szGrub[256+4];
        
        memset(szGrub,0,256+4);
        
	memset((BYTE *)0x90000,0,4096);

	szGrub[0]=0xff;
	szGrub[1]=0xff;
	szGrub[2]=nActivePartition;
	szGrub[3]=0x00;

	errnum=0;
	boot_drive=0;
	saved_drive=0;
	saved_partition=0x0001ffff;
	buf_drive=-1;
	current_partition=0x0001ffff;
	current_drive=0xff;
	buf_drive=-1;
	fsys_type = NUM_FSYS;
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
			int nLen;
			CACHE_VSYNC_WRITEBACK=0;
			nLen=grub_read((void *)0x90000, filemax);
			if(nLen>0) { ((char *)0x90000)[nLen]='\0'; }  // needed to terminate incoming string, reboot in ParseConfig without it
		}
		ParseConfig((char *)0x90000,config,&eeprom, NULL);
		BootPrintConfig(config);
		printf("linuxboot.cfg is %d bytes long.\n", dwConfigSize);
	}
	grub_close();
	CACHE_VSYNC_WRITEBACK=1;
	
	//strcpy(&szGrub[4], config->szKernel);
        _strncpy(&szGrub[4], config->szKernel,sizeof(config->szKernel));

	nRet=grub_open(szGrub);

	if(nRet!=1) {
		if(fJustTestingForPossible) return 0;
		printk("Unable to load kernel, Grub error %d\n", errnum);
		while(1) ;
	}
	if(fJustTestingForPossible) return 1; // if there's a default kernel it must be worth trying to boot
        
        // We use the INITRD_POS as temporary location for the Loading of the Kernel into intermediate Ram
	dwKernelSize=grub_read((BYTE*)INITRD_POS, filemax);
	memcpy((BYTE *)0x90000,(BYTE*)INITRD_POS,0x400);
	nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
	memcpy((BYTE *)0x90400,(BYTE*)(INITRD_POS+0x400),nSizeHeader-0x400);
	memcpy((BYTE *)0x00100000,(BYTE*)(INITRD_POS+nSizeHeader),dwKernelSize-nSizeHeader);
		
	// Fillup
	memset((BYTE *)(0x00100000+dwKernelSize-nSizeHeader),0xff,0x10000);
	grub_close();
	printk(" -  %d bytes...\n", dwKernelSize);



	if( (_strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd[0]) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s ", config->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
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
		// Fillup
		memset((void *)(INITRD_POS+dwInitrdSize),0xff,0x10000);
		
		grub_close();
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
	}

	return true;
}


int BootTryLoadConfigFATX(CONFIGENTRY *config) {

	FATXPartition *partition = NULL;
	FATXFILEINFO fileinfo;
	FATXFILEINFO infokernel;
	int nConfig = 0;

	partition = OpenFATXPartition(0,SECTOR_STORE,STORE_SIZE);
	
	if(partition != NULL) {

		if(!LoadFATXFile(partition,"/linuxboot.cfg",&fileinfo)) {
			if(LoadFATXFile(partition,"/debian/linuxboot.cfg",&fileinfo) ) {
				fileinfo.buffer[fileinfo.fileSize]=0;
				ParseConfig(fileinfo.buffer,config,&eeprom,"/debian");
				free(fileinfo.buffer);
				CloseFATXPartition(partition);
			}
		} else {
			fileinfo.buffer[fileinfo.fileSize]=0;
			ParseConfig(fileinfo.buffer,config,&eeprom,NULL);
			free(fileinfo.buffer);
		}
	} else {
		CloseFATXPartition(partition);
		return 0;
	}

	// We use the INITRD_POS as temporary location for the Loading of the Kernel into intermediate Ram
	
	if(! LoadFATXFilefixed(partition,config->szKernel,&infokernel,(BYTE*)INITRD_POS)) {
		CloseFATXPartition(partition);
		return 0;
	} else {
		CloseFATXPartition(partition);
		return 1; // worth trying, since the filesystem and kernel exists
	}
	
}

/* ----------------------------------------------------------------------------------------- */

int BootLoadConfigFATX(CONFIGENTRY *config) {

	static FATXPartition *partition = NULL;
	static FATXFILEINFO fileinfo;
	static FATXFILEINFO infokernel;
	static FATXFILEINFO infoinitrd;

	memset((BYTE *)0x90000,0,4096);
	memset(&fileinfo,0x00,sizeof(fileinfo));
	memset(&infokernel,0x00,sizeof(infokernel));
	memset(&infoinitrd,0x00,sizeof(infoinitrd));

	I2CTransmitWord(0x10, 0x0c01); // Close DVD tray
	
	printk("Loading linuxboot.cfg from FATX\n");
	partition = OpenFATXPartition(0,
			SECTOR_STORE,
			STORE_SIZE);
	
	if(partition != NULL) {
		if(LoadFATXFile(partition,"/linuxboot.cfg",&fileinfo) ) {
			wait_ms(50);
			fileinfo.buffer[fileinfo.fileSize]=0;
			ParseConfig(fileinfo.buffer,config,&eeprom, NULL);
			free(fileinfo.buffer);
		} else {
			if(LoadFATXFile(partition,"/debian/linuxboot.cfg",&fileinfo) ) {
				wait_ms(50);
				fileinfo.buffer[fileinfo.fileSize]=0;
				ParseConfig(fileinfo.buffer,config,&eeprom, "/debian");
				free(fileinfo.buffer);
			} else {
				wait_ms(50);
				printk("linuxboot.cfg not found, using defaults\n");
			}
		}

	} 

	BootPrintConfig(config);
	
	// We use the INITRD_POS as temporary location for the Loading of the Kernel into intermediate Ram
	
	if(! LoadFATXFilefixed(partition,config->szKernel,&infokernel,(BYTE*)INITRD_POS)) {
		printk("Error loading kernel %s\n",config->szKernel);
		while(1);
	} else {
		dwKernelSize = infokernel.fileSize;
		// moving the kernel to its final location
		memcpy((BYTE *)0x90000,(BYTE*)INITRD_POS,0x400);
		nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
		memcpy((BYTE *)0x90400,(BYTE*)(INITRD_POS+0x400),nSizeHeader-0x400);
		memcpy((BYTE *)0x00100000,(BYTE*)(INITRD_POS+nSizeHeader),infokernel.fileSize-nSizeHeader);
		
		// Fillup
		memset((BYTE *)(0x00100000+dwKernelSize-nSizeHeader),0xff,0x10000);
		
		printk(" -  %d %d bytes...\n", dwKernelSize, infokernel.fileRead);
	}
	
	if( (_strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd[0]) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from FATX", config->szInitrd);
		wait_ms(50);
		if(! LoadFATXFilefixed(partition,config->szInitrd,&infoinitrd,(BYTE*)INITRD_POS)) {
			printk("Error loading initrd %s\n",config->szInitrd);
			while(1);
		}
		
		// Fillup
		memset((BYTE *)(INITRD_POS+infoinitrd.fileSize),0xff,0x10000);

		dwInitrdSize = infoinitrd.fileSize;
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


/* -------------------------------------------------------------------------------- */

int BootLoadConfigCD(CONFIGENTRY *config) {

	DWORD dwConfigSize=0, dw;
	BYTE ba[2048],baBackground[640*64*4]; 
	


	BYTE bCount=0, bCount1;
	int n;
	
	DWORD dwY=VIDEO_CURSOR_POSY;
	DWORD dwX=VIDEO_CURSOR_POSX;

	memset((BYTE *)0x90000,0,4096);

	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
	wait_ms(2000); // Wait for DVD to become responsive to inject command
		
selectinsert:
	BootVideoBlit(
		(DWORD *)&baBackground[0], 640*4,
		(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
		currentvideomodedetails.m_dwWidthInPixels*4, 64
	);

	
	while(1) {
                int n;
		
		if (DVD_TRAY_STATE == DVD_CLOSING) {
			wait_ms(500);
			break;
		}
		
		if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) {
			I2CTransmitWord(0x10, 0x0c01); // close DVD tray
			wait_ms(500);
			break;
		}
                USBGetEvents();
		wait_ms(10);

		VIDEO_CURSOR_POSX=dwX;
		VIDEO_CURSOR_POSY=dwY;
		bCount++;
		bCount1=bCount; if(bCount1&0x80) { bCount1=(-bCount1)-1; }
		VIDEO_ATTR=0xff000000|(((bCount1>>1)+64)<<16)|(((bCount1>>1)+64)<<8)|0 ;
		printk("\2Please insert CD and press Button A\n\2");
	}						

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
		int timeoutcount = 0;
		
		while(fMore) {
			timeoutcount++;
			// We waited very long now for a Good read sector, but we did not get one, so we
			// jump back and try again
			if (timeoutcount>200) {
				VIDEO_ATTR=0xffffffff;
				VIDEO_CURSOR_POSX=dwX;
				VIDEO_CURSOR_POSY=dwY;
				BootVideoBlit(
				(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
				currentvideomodedetails.m_dwWidthInPixels*4, (DWORD *)&baBackground[0], 640*4, 64
				);
				I2CTransmitWord(0x10, 0x0c00); // eject DVD tray	
				wait_ms(2000); // Wait for DVD to become responsive to inject command

				goto selectinsert;
			}
			wait_ms(200);
			
			if(BootIdeReadSector(1, &ba[0], 0x10, 0, 2048)) { // starts at 16
				VIDEO_CURSOR_POSX=dwX;
				VIDEO_CURSOR_POSY=dwY;
				bCount++;
				bCount1=bCount; if(bCount1&0x80) { bCount1=(-bCount1)-1; }

				VIDEO_ATTR=0xff000000|(((bCount1)+64)<<16)|(((bCount1>>1)+128)<<8)|(((bCount1)+128)) ;

				printk("\2Waiting for drive\2\n");
	
			} else {  // read it successfully
				fMore=false;
				fOkay=true;
			}
		}

		if(!fOkay) {
			void BootFiltrorDebugShell(void);
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

	printk("  Loading linuxboot.cfg from CDROM... \n");
	dwConfigSize=BootIso9660GetFile("/linuxboot.cfg", (BYTE *)INITRD_POS, 0x800, 0x0);

	if(((int)dwConfigSize)<0) { // not found, try mangled 8.3 version
		dwConfigSize=BootIso9660GetFile("/LINUXBOO.CFG", (BYTE *)INITRD_POS, 0x800, 0x0);
		if(((int)dwConfigSize)<0) { // has to be there on CDROM
			printk("Unable to find it, halting\n");
			while(1) ;
		}
	}
        
        // LinuxBoot.cfg File Loaded
        
        ((char *)INITRD_POS)[dwConfigSize]=0;
	ParseConfig((char *)INITRD_POS,config,&eeprom, NULL);
	BootPrintConfig(config);

	// We use the INITRD_POS as temporary location for the Loading of the Kernel into intermediate Ram
	dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE*)INITRD_POS, 4*1024*1024, 0);

	// If failed, lets look for an other name ...
	if(((int)dwKernelSize)<0) { // not found, try 8.3
		strcpy(config->szKernel, "/VMLINUZ.");
		dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE*)INITRD_POS, 4*1024*1024, 0);
		if(((int)dwKernelSize)<0) { 
			strcpy(config->szKernel, "/VMLINUZ_.");
			dwKernelSize=BootIso9660GetFile(config->szKernel, (BYTE*)INITRD_POS, 4*1024*1024, 0);
			if(((int)dwKernelSize)<0) { 
				printk("Not Found, error %d\nHalting\n", dwKernelSize); 
				while(1);
			}
		}
	}
	
	if (dwKernelSize>0) 
	{
		memcpy((BYTE *)0x90000,(BYTE*)INITRD_POS,0x400);
		nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
		memcpy((BYTE *)0x90400,(BYTE*)(INITRD_POS+0x400),nSizeHeader-0x400);
		memcpy((BYTE *)0x00100000,(BYTE*)(INITRD_POS+nSizeHeader),dwKernelSize-nSizeHeader);
		
		// Fillup
		memset((BYTE *)(0x00100000+dwKernelSize-nSizeHeader),0xff,0x10000);
		printk(" -  %d bytes...\n", dwKernelSize);
		
				
	} else {
		printk("Not Found, error %d\nHalting\n", dwKernelSize); 
		while(1);
	}	

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
		// Fillup
		memset((BYTE *)(INITRD_POS+dwInitrdSize),0xff,0x10000);
		
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

int BootLoadFlashCD(CONFIGENTRY *config) {

        DWORD dwConfigSize=0;
	BYTE ba[2048];
	BYTE baBackground[640*64*4]; 
	BYTE bCount=0, bCount1;
	unsigned char checksum[20];
	unsigned int n;
	
	DWORD dwY=VIDEO_CURSOR_POSY;
	DWORD dwX=VIDEO_CURSOR_POSX;
	struct SHA1Context context;
      	unsigned char SHA1_result[20];

	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
	wait_ms(2000); // Wait for DVD to become responsive to inject command
                        	
selectinsert:
	BootVideoBlit(
		(DWORD *)&baBackground[0], 640*4,
		(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
		currentvideomodedetails.m_dwWidthInPixels*4, 64
	);


	while(DVD_TRAY_STATE != DVD_CLOSING) {
	
		VIDEO_CURSOR_POSX=dwX;
		VIDEO_CURSOR_POSY=dwY;
		bCount++;
		bCount1=bCount; if(bCount1&0x80) { bCount1=(-bCount1)-1; }
			VIDEO_ATTR=0xff000000|(((bCount1>>1)+64)<<16)|(((bCount1>>1)+64)<<8)|0 ;
		printk("\2Please insert CD - Flashing Mode\n\2");
		
		for (n=0;n<1000000;n++) {;}
	}


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
		int timeoutcount = 0;
		
		while(fMore) {
			timeoutcount++;
			// We waited very long now for a Good read sector, but we did not get one, so we
			// jump back and try again
			if (timeoutcount>200) {
				VIDEO_ATTR=0xffffffff;
				VIDEO_CURSOR_POSX=dwX;
				VIDEO_CURSOR_POSY=dwY;
				BootVideoBlit(
				(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*currentvideomodedetails.m_dwWidthInPixels*4)+VIDEO_CURSOR_POSX),
				currentvideomodedetails.m_dwWidthInPixels*4, (DWORD *)&baBackground[0], 640*4, 64
				);
				I2CTransmitWord(0x10, 0x0c00); // eject DVD tray	
				wait_ms(2000); // Wait for DVD to become responsive to inject command
				goto selectinsert;
			}
			wait_ms(200);
			
			if(BootIdeReadSector(1, &ba[0], 0x10, 0, 2048)) { // starts at 16
				VIDEO_CURSOR_POSX=dwX;
				VIDEO_CURSOR_POSY=dwY;
				bCount++;
				bCount1=bCount; if(bCount1&0x80) { bCount1=(-bCount1)-1; }

				VIDEO_ATTR=0xff000000|(((bCount1)+64)<<16)|(((bCount1>>1)+128)<<8)|(((bCount1)+128)) ;

				printk("\2Waiting for drive - Mode Flashing\2\n");
	
			} else {  // read it successfully
				fMore=false;
				fOkay=true;
			}
		}

		if(!fOkay) {
			void BootFiltrorDebugShell(void);
			printk("cdrom unhappy\n");
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
    
	dwConfigSize=0;
        dwConfigSize=BootIso9660GetFile("/IMAGE.BIN", (BYTE *)0x100000, 256*1024, 0x0);   
	printk("Image size: %i\n", dwConfigSize);   
        if (dwConfigSize!=256*1024) {
		printk("Image != 256kbyte image\n");           	
		return 0;
	}
        dwConfigSize = 256*1024;
        
      	SHA1Reset(&context);
	SHA1Input(&context,(BYTE *)0x100000,dwConfigSize);
	SHA1Result(&context,SHA1_result);
/*	
	if (_memcmp(&checksum[0],&SHA1_result[0],20)!=0) {
		printk("Checksum on Disk Not Matching-Bad image or Missread\n");
		while(1);		
	}
  */    
  	memcpy(checksum,SHA1_result,20);
  
	//printk("Bios Disk Checksum Matching\n");
	
    	printk("Error code: $i\n", BootReflashAndReset((BYTE*) 0x100000, (DWORD) 0, (DWORD) dwConfigSize));   

	//memcpy((void *) 0x100004,(void *)LPCFlashadress,0x100000);
      	
      	SHA1Reset(&context);
	SHA1Input(&context,(void *)LPCFlashadress,dwConfigSize);
	SHA1Result(&context,SHA1_result);
        
    //    memcpy(&n,(void *)(0x100004+0x40000*4-8),4);
    //    printk("%08x\n",n);
    //    memcpy(&n,(void *)(0x100004+0x40000*4-4),4);
    //    printk("%08x\n",n);
		
	if (_memcmp(&checksum[0],&SHA1_result[0],20)==0) {
		printk("Checksum in Flash Matching - Success\n");
	} else {
		printk("Checksum in Flash not matching - MISTAKE -Reflashing!\n"); 
		printk("Error code: $i\n", BootReflashAndReset((BYTE*) 0x100000, (DWORD) 0, (DWORD) dwConfigSize));   
	//	while(1);		
	}		


	return true;
}

#endif

void BootIcons(int nXOffset, int nYOffset, int nTextOffsetX, int nTextOffsetY) {
	memset(icon,0,sizeof(ICON) * ICONCOUNT);
	icon[ICON_FATX].nDestX = nXOffset + 120;
	icon[ICON_FATX].nDestY = nYOffset - 74;
	icon[ICON_FATX].nSrcX = ICON_WIDTH;
	icon[ICON_FATX].nSrcLength = ICON_WIDTH;
	icon[ICON_FATX].nSrcHeight = ICON_HEIGH;
	icon[ICON_FATX].nTextX = (nTextOffsetX+60)<<2;;
	icon[ICON_FATX].nTextY = nTextOffsetY;
	icon[ICON_FATX].szCaption = "/linuxboot.cfg from FATX";

	icon[ICON_NATIVE].nDestX = nXOffset + 245;
	icon[ICON_NATIVE].nDestY = nYOffset - 74;
	icon[ICON_NATIVE].nSrcX = ICON_WIDTH;
	icon[ICON_NATIVE].nSrcLength = ICON_WIDTH;
	icon[ICON_NATIVE].nSrcHeight = ICON_HEIGH;
	icon[ICON_NATIVE].nTextX = (nTextOffsetX+240)<<2;;
	icon[ICON_NATIVE].nTextY = nTextOffsetY;
	icon[ICON_NATIVE].szCaption = "/dev/hda";

	icon[ICON_CD].nDestX = nXOffset + 350;
	icon[ICON_CD].nDestY = nYOffset - 74;
	icon[ICON_CD].nSrcX = ICON_WIDTH*2;
	icon[ICON_CD].nSrcLength = ICON_WIDTH;
	icon[ICON_CD].nSrcHeight = ICON_HEIGH;
	icon[ICON_CD].nTextX = (nTextOffsetX+350)<<2;
	icon[ICON_CD].nTextY = nTextOffsetY;
	icon[ICON_CD].szCaption = "/dev/hdb";
	
	icon[ICON_FLASH].nDestX = nXOffset + 440;
	icon[ICON_FLASH].nDestY = nYOffset - 74;
	icon[ICON_FLASH].nSrcX = ICON_WIDTH*3;
	icon[ICON_FLASH].nSrcLength = ICON_WIDTH;
	icon[ICON_FLASH].nSrcHeight = ICON_HEIGH;
	icon[ICON_FLASH].nTextX = (nTextOffsetX+440)<<2;
	icon[ICON_FLASH].nTextY = nTextOffsetY;
	icon[ICON_FLASH].szCaption = "Flash";

	if (cromwell_haverombios==1) icon[ICON_FLASH].szCaption = "BIOS";	
	
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





int BootMenu(CONFIGENTRY *config,int nDrive,int nActivePartition, int nFATXPresent){
	
	int old_nIcon = 0;
	int nSelected = -1;
	unsigned int menu=0;
	int change=0;

	int nTempCursorResumeX, nTempCursorResumeY ;
	int nTempCursorX, nTempCursorY;
	int nModeDependentOffset=(currentvideomodedetails.m_dwWidthInPixels-640)/2;  // icon offsets computed for 640 modes, retain centering in other modes
	int nShowSelect = false;
        unsigned char *videosavepage;
        
        DWORD COUNT_start;
        DWORD HH;
        DWORD temp;
        
	#define DELAY_TICKS 72
	#define TRANPARENTNESS 0x30
	#define OPAQUENESS 0xc0
	#define SELECTED 0xff

	nTempCursorResumeX=nTempCursorMbrX;
	nTempCursorResumeY=nTempCursorMbrY;

	
	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=currentvideomodedetails.m_dwHeightInLines-80;
	
	// We save the complete Video Page to a memory (we restore at exit)
	videosavepage = malloc(FRAMEBUFFER_SIZE);
	memcpy(videosavepage,(void*)FRAMEBUFFER_START,FRAMEBUFFER_SIZE);
	
	VIDEO_CURSOR_POSX=((215+nModeDependentOffset)<<2);
	VIDEO_CURSOR_POSY=nTempCursorY-100;
	
	VIDEO_ATTR=0xffc8c8c8;
	printk("Select from Menu\n");
	VIDEO_ATTR=0xffffffff;
	
	BootIcons(nModeDependentOffset, nTempCursorY, nModeDependentOffset, nTempCursorY);
	
	// Display The Icons
	for(menu = 0; menu < ICONCOUNT;menu ++) {
		BootStartBiosDoIcon(&icon[menu], TRANPARENTNESS);
	}
	
	// Look which Icons are enabled or disabled	
        for(menu = 0; menu < ICONCOUNT;menu ++) {
        
		icon[menu].nEnabled = 0;		
        	switch(menu){
	
			case ICON_FATX:
				if(nFATXPresent) {
					strcpy(config->szKernel, "/vmlinuz"); // fatx default kernel, looked for to detect fs
					if(BootTryLoadConfigFATX(config)){
						icon[menu].nEnabled = 1;
						if(nSelected == -1) nSelected = menu;
					}
				}
				break;
	
			case ICON_NATIVE:
				if(nDrive != 1) {
					strcpy(config->szKernel, "/boot/vmlinuz");  // Ext2 default kernel, looked for to detect fs
					if(BootLoadConfigNative(nActivePartition, config, true)) {
						icon[menu].nEnabled = 1;
						if(nSelected == -1) nSelected = menu;
					}
				}
				break;
	
			case ICON_CD:
				icon[menu].nEnabled = 1;
				if(nSelected == -1) nSelected = menu;
				break;
	
			case ICON_FLASH:
				icon[menu].nEnabled = 1;
				if(nSelected == -1) nSelected = menu;
				break;
		}
	}	
        
        if (nSelected==-1) nSelected = ICON_CD;
        
        // Initial Selected Icon
        menu = nSelected;
        old_nIcon = nSelected;
	icon[menu].nSelected = 1;
	change = 1;
	COUNT_start = IoInputDword(0x8008);
	
	while(1)
	{
		int n;
		USBGetEvents();
		

		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_LEFT) == 1)
		{
			change=1;
			menu = menu+3;
			menu = menu%4;
			
			while(icon[menu].nEnabled==0) {
				menu = menu+3;
				menu = menu%4;
			}
			COUNT_start = IoInputDword(0x8008);
		}
		

		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_RIGHT) == 1)
		{
			change=1;
			menu = menu+1;
			menu = menu%4;
			
			while(icon[menu].nEnabled==0) {
				menu = menu+5;
				menu = menu%4;
			}
			COUNT_start = IoInputDword(0x8008);
		}
                
		HH = IoInputDword(0x8008);
		temp = HH-COUNT_start;

		if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) || (temp>(0x369E99*BOOT_TIMEWAIT))) {
		
			change=1; 
			memcpy((void*)FRAMEBUFFER_START,videosavepage,FRAMEBUFFER_SIZE);
			free(videosavepage);
			
			VIDEO_CURSOR_POSX=nTempCursorResumeX;
			VIDEO_CURSOR_POSY=nTempCursorResumeY;
			
        		// We return the selected Menu
			return menu;			
			
		}
		
		if (change) 
		{
			
		        BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);
			BootStartBiosDoIcon(&icon[old_nIcon], TRANPARENTNESS);
			old_nIcon = menu;
			BootStartBiosDoIcon(&icon[menu], SELECTED);

                        VIDEO_CURSOR_POSX=icon[menu].nTextX;
			VIDEO_CURSOR_POSY=icon[menu].nTextY;
			
			printk("%s\n",icon[menu].szCaption);
			
			switch(menu){
				case ICON_NATIVE:
					strcpy(config->szKernel, "/boot/vmlinuz");
					break;
			}
		}

		change=0;	    
	}

}


int ExittoLinux(CONFIGENTRY *config);
void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine);
int ExittoRomBios(void);
int etherboot(void);


void StartBios(CONFIGENTRY *config, int nActivePartition , int nFATXPresent,int bootfrom) {

	char szGrub[256+4];
	int menu=0,selected=0;
	int change=0;
        
        memset(szGrub,0x00,sizeof(szGrub));
        
	szGrub[0]=0xff; 
	szGrub[1]=0xff; 
	szGrub[2]=nActivePartition; 
	szGrub[3]=0x00;

	errnum=0; 
	boot_drive=0; 
	saved_drive=0; 
	saved_partition=0x0001ffff; 
	buf_drive=-1;
	
	current_partition=0x0001ffff; 
	current_drive=0xff; 
	buf_drive=-1; 
	fsys_type = NUM_FSYS;
	
	disk_read_hook=NULL;
	disk_read_func=NULL;


	
	// silence the audio
        	
        //BootAudioSilence(&ac97device);
	if (bootfrom==-1) {
        // Nothing in All selceted
		#ifdef DEFAULT_FATX
			bootfrom = ICON_FATX;
			printk("Defaulting to HDD boot\n");
			I2CTransmitWord(0x10, 0x0c01); // close DVD tray
			bootfrom = ICON_NATIVE;

		#else
			printk("Defaulting to CD boot\n");
			bootfrom = ICON_CD;

		#endif	
	}


	if(bootfrom == ICON_FATX) {
		strcpy(config->szAppend, "init=/linuxrc root=/dev/ram0 pci=biosirq kbd-reset"); // default
		strcpy(config->szKernel, "/vmlinuz");
		strcpy(config->szInitrd, "/initrd");
	} else {
		strcpy(config->szAppend, "root=/dev/hda2 devfs=mount kbd-reset"); // default
		strcpy(config->szKernel, "/boot/vmlinuz");
		strcpy(config->szInitrd, "/boot/initrd");
	}
        
        
	switch(bootfrom) {
		case ICON_FATX:
			BootLoadConfigFATX(config);
			ExittoLinux(config);
			break;
		case ICON_NATIVE:
			BootLoadConfigNative(nActivePartition, config, false);
			ExittoLinux(config);
			break;
		case ICON_CD:
			//hddclone();
			BootLoadConfigCD(config);
			ExittoLinux(config);
			break;
		case ICON_FLASH:
			etherboot();
			break;
		default:
			printk("Selection not implemented\n");
			break;
	}
}


int ExittoLinux(CONFIGENTRY *config) {
	
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
	startLinux((void *)INITRD_POS, dwInitrdSize, config->szAppend);
}
	

void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine)
{
	int nAta=0;
	// turn off USB
	BootStopUSB();
	CACHE_VSYNC_WRITEBACK = 0;
	setup( (void *)0x90000, initrdStart, initrdSize, appendLine);
        
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
	// nAta=1;
	BootIdeSetTransferMode(0, 0x40 | nAta);
	BootIdeSetTransferMode(1, 0x40 | nAta);
	// BootIdeSetTransferMode(0, 0x04);

	// orangeness, people seem to like that colour
       
	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);
	         
	asm volatile ("wbinvd\n");
	
	// Tell Video Card we have changed the offset to higher up
	(*(unsigned int*)0xFD600800) = (0xf0000000 | ((xbox_ram*0x100000) - FRAMEBUFFER_SIZE));
	   
	memset((void*)0x00700000,0x0,1024*8);
	
	__asm __volatile__ (
       	"cli \n"
	
	// Flush the TLB
	"xor %eax, %eax \n"
	"mov %eax, %cr3 \n"
	
	// We kill the Local Descriptor Table        
        "xor	%eax, %eax \n"
	"lldt	%ax	\n"
	
	// We clear the IDT table (the first 8 bytes of the GDT are 0x0)
	"lidt 	0x00700000\n"		
	
	// DR6/DR7: Clear the debug registers
	"xor %eax, %eax \n"
	"mov %eax, %dr6 \n"
	"mov %eax, %dr7 \n"
	"mov %eax, %dr0 \n"
	"mov %eax, %dr1 \n"
	"mov %eax, %dr2 \n"
	"mov %eax, %dr3 \n"
	
	// IMPORTANT!  Linux expects the GDT located at a specific position,
	// 0xA0000, so we have to move it there.
	
	// Kill the LDT, if any
	"xor	%eax, %eax \n"
	"lldt %ax \n"
        
	// Reload CS as 0010 from the new GDT using a far jump
	".byte 0xEA       \n"   // jmp far 0010:reload_cs
	".long reload_cs_exit  \n"
	".word 0x0010  \n"
	
	".align 16  \n"
	"reload_cs_exit: \n"

	// CS is now a valid entry in the GDT.  Set SS, DS, and ES to valid
	// descriptors, but clear FS and GS as they are not necessary.

	// Set SS, DS, and ES to a data32 segment with maximum limit.
	"movw $0x0018, %ax \n"
	"mov %eax, %ss \n"
	"mov %eax, %ds \n"
	"mov %eax, %es \n"

	// Clear FS and GS
	"xor %eax, %eax \n"
	"mov %eax, %fs \n"
	"mov %eax, %gs \n"

	// Set the stack pointer to give us a valid stack
	"movl $0x03BFFFFC, %esp \n"
	
	"xor 	%ebx, %ebx \n"
	"xor 	%eax, %eax \n"
	"xor 	%ecx, %ecx \n"
	"xor 	%edx, %edx \n"
	"xor 	%edi, %edi \n"
	"movl 	$0x90000, %esi\n"       // Offset of the GRUB
  	"ljmp 	$0x10, $0x100000\n"	// Jump to Kernel
	);
	
	// We are not longer here, we are already in the Linux loader, we never come back here
	
	// See you again in Linux then	
	while(1);
}



int ExittoRomBios(void) {
	
        unsigned int tempstart;
        unsigned int templen;
	extern int _end_pcrombios;
 	extern int _start_pcrombios;
 	
 	tempstart = (unsigned int)((BYTE *)&_start_pcrombios);
	templen = ((DWORD)(&_end_pcrombios)-(DWORD)(&_start_pcrombios));       
        
	// turn off USB
	BootStopUSB();

	{
		char *sz="\2Starting BOCHS-BIOS\2";
		VIDEO_CURSOR_POSX=((currentvideomodedetails.m_dwWidthInPixels-BootVideoGetStringTotalWidth(sz))/2)*4;
		VIDEO_CURSOR_POSY=currentvideomodedetails.m_dwHeightInLines-64;

		VIDEO_ATTR=0xff9f9fbf;
		printk(sz);
	}


//		int n=0;
//		DWORD dwCsum1=0, dwCsum2=0;
//		BYTE *pb1=(BYTE *)0xf0000, *pb2=(BYTE*)&rombios;
  //  printk("  Copying BIOS into RAM...\n");
	
	//extern char rombios[1];

	//	I2cSetFrontpanelLed(0x77);

	// copy the 64K 16-bit BIOS code into memory at 0xF0000, this is where a BIOS
	// normally appears in 16-bit mode


	// here we should copy .. disabled for the moment


		
	memcpy((void *)0xf0000, (void *)(tempstart), templen);
        	
        // Test code for integrity check
	
	
	{

	unsigned char state2[20];
	unsigned int i;
	struct SHA1Context context;
	
	VIDEO_CURSOR_POSY= 200;
	VIDEO_CURSOR_POSX = 100;
	
	SHA1Reset(&context);
	SHA1Input(&context,(void *)0xf0000,templen);
	SHA1Result(&context,state2);

	printk(" %08x\n",tempstart);
	printk(" %08x\n", templen);
       	printk(" SHA-1 Checksum of the pcbios/rompcbios.bin   ... this should match ..\n", templen);
       	
	for (i=0;i<20;i++) printk(" %02x",state2[i]); 

	}
	
	
	
	// LEDs to yellow

	// copy a 16-bit LJMP stub into a safe place that is visible in 16-bit mode
	// (the BIOS isn't visible in 1MByte address space)

	__asm __volatile__ (
		
		"cli \n"
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

		"movl %cr0, %eax     \n" // 16-bi
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

       	// See you again in Windows ??? hihih
	while(1);

}
