/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"
#include "video.h"
#include "memory_layout.h"
#include <shared.h>
#include <filesys.h>
#include "rc4.h"
#include "sha1.h"
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"
#include "BootParser.h"

#include "config.h"

//Grub bits
unsigned long saved_drive;
grub_error_t errnum;
unsigned long saved_partition;
unsigned long boot_drive;
	
static int nRet;
static u32 dwKernelSize= 0, dwInitrdSize = 0;


int ExittoLinux(CONFIGENTRY *config);
void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine);


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


void memPlaceKernel(const u8* kernelOrg, u32 kernelSize)
{
	unsigned int nSizeHeader=((*(kernelOrg + 0x01f1))+1)*512;
	memcpy((u8 *)KERNEL_SETUP, kernelOrg, nSizeHeader);
	memcpy((u8 *)KERNEL_PM_CODE,(kernelOrg+nSizeHeader),kernelSize-nSizeHeader);
}



CONFIGENTRY* LoadConfigNative(int drive, int partition) {
	CONFIGENTRY *config;
	CONFIGENTRY *currentConfigItem;
	unsigned int nLen;
	u32 dwConfigSize=0;
	char *szGrub;
	u8* tempBuf;
	
	szGrub = (char *) malloc(265+4);
        memset(szGrub,0,256+4);
        
	memset((u8 *)KERNEL_SETUP,0,2048);

	szGrub[0]=0xff;
	szGrub[1]=0xff;
	szGrub[2]=partition;
	szGrub[3]=0x00;

	errnum=0;
	boot_drive=0;
	saved_drive=0;
	saved_partition=0x0001ffff;
	buf_drive=-1;
	current_partition=0x0001ffff;
	current_drive=drive;
	buf_drive=-1;
	fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;

	//Try for /boot/linuxboot.cfg first
	strcpy(&szGrub[4], "/boot/linuxboot.cfg");
	nRet=grub_open(szGrub);

	if(nRet!=1 || errnum) {
		//Not found - try /linuxboot.cfg
		errnum=0;
		strcpy(&szGrub[4], "/linuxboot.cfg");
		nRet=grub_open(szGrub);
	}
	
	dwConfigSize=filemax;
	if (nRet!=1 || errnum) {
		//File not found
		free(szGrub);
		return NULL;
	}
	
	nLen=grub_read((void *)KERNEL_SETUP, filemax);

	if (nLen > MAX_CONFIG_FILESIZE) nLen = MAX_CONFIG_FILESIZE;
	
	config = ParseConfig((char *)KERNEL_SETUP, nLen, NULL);

	for (currentConfigItem = (CONFIGENTRY*) config; currentConfigItem!=NULL ; currentConfigItem = (CONFIGENTRY*)currentConfigItem->nextConfigEntry) {
		//Set the drive ID and partition IDs for the returned config items
		currentConfigItem->bootType=BOOT_NATIVE;
		currentConfigItem->drive=drive;
		currentConfigItem->partition=partition;
	}
	
	grub_close();
	free(szGrub);
	return config;
}

int LoadKernelNative(CONFIGENTRY *config) {
	int nLen;
	u32 dwConfigSize=0;
	char *szGrub;
	u8* tempBuf;

	szGrub = (char *) malloc(265+4);
        memset(szGrub,0,256+4);
        
	memset((u8 *)KERNEL_SETUP,0,2048);

	szGrub[0]=0xff;
	szGrub[1]=0xff;
	szGrub[2]=config->partition;
	szGrub[3]=0x00;

	errnum=0;
	boot_drive=0;
	saved_drive=0;
	saved_partition=0x0001ffff;
	buf_drive=-1;
	current_partition=0x0001ffff;
	current_drive=config->drive;
	buf_drive=-1;
	fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;
	
	I2CTransmitWord(0x10, 0x0c01); // Close DVD tray
	
        strncpy(&szGrub[4], config->szKernel,strlen(config->szKernel));

	nRet=grub_open(szGrub);

	if(nRet!=1) {
		printk("Unable to load kernel, Grub error %d\n", errnum);
		while(1) ;
	}
        
	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (u8*)INITRD_START;
	dwKernelSize=grub_read(tempBuf, MAX_KERNEL_SIZE);
	memPlaceKernel(tempBuf, dwKernelSize);
	grub_close();
	printk(" -  %d bytes...\n", dwKernelSize);

	if( (strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd[0]) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s ", config->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
 		strncpy(&szGrub[4], config->szInitrd,sizeof(config->szInitrd));
		nRet=grub_open(szGrub);
		if(filemax==0) {
			printf("Empty file\n"); while(1);
		}
		if( (nRet!=1) || (errnum)) {
			printk("Unable to load initrd, Grub error %d\n", errnum);
			while(1) ;
		}
		printk(" - %d bytes\n", filemax);
		dwInitrdSize=grub_read((void*)INITRD_START, MAX_INITRD_SIZE);
		grub_close();
	} else {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  No initrd from config file");
		VIDEO_ATTR=0xffa8a8a8;
		dwInitrdSize=0;
	}
	free(szGrub);
	return true;
}

CONFIGENTRY* LoadConfigFatX(void) {
	FATXPartition *partition = NULL;
	FATXFILEINFO fileinfo;
	FATXFILEINFO infokernel;
	CONFIGENTRY *config, *currentConfigItem;
	
	partition = OpenFATXPartition(0,SECTOR_STORE,STORE_SIZE);
	
	if(partition != NULL) {

		if(LoadFATXFile(partition,"/linuxboot.cfg",&fileinfo)) {
			//Root of E has a linuxboot.cfg in
			config = (CONFIGENTRY*)malloc(sizeof(CONFIGENTRY));	
			config = ParseConfig(fileinfo.buffer, fileinfo.fileSize, NULL);
			free(fileinfo.buffer);
		}
		else if(LoadFATXFile(partition,"/debian/linuxboot.cfg",&fileinfo) ) {
			//Try in /debian on E
			config = (CONFIGENTRY*)malloc(sizeof(CONFIGENTRY));	
			config = ParseConfig(fileinfo.buffer, fileinfo.fileSize, "/debian");
			free(fileinfo.buffer);
			CloseFATXPartition(partition);
		}
	} 
	if (config == NULL) return NULL;

	for (currentConfigItem = (CONFIGENTRY*) config; currentConfigItem!= NULL ; currentConfigItem = (CONFIGENTRY*)currentConfigItem->nextConfigEntry) {
		currentConfigItem->bootType = BOOT_FATX;
	}
	return config;
}

int LoadKernelFatX(CONFIGENTRY *config) {

	static FATXPartition *partition = NULL;
	static FATXFILEINFO fileinfo;
	static FATXFILEINFO infokernel;
	static FATXFILEINFO infoinitrd;
	u8* tempBuf;

	memset((u8 *)KERNEL_SETUP,0,4096);
	memset(&fileinfo,0x00,sizeof(fileinfo));
	memset(&infokernel,0x00,sizeof(infokernel));
	memset(&infoinitrd,0x00,sizeof(infoinitrd));

	I2CTransmitWord(0x10, 0x0c01); // Close DVD tray
	
	partition = OpenFATXPartition(0,
			SECTOR_STORE,
			STORE_SIZE);
	
	if(partition == NULL) return 0;

	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (u8*)INITRD_START;
	if(! LoadFATXFilefixed(partition,config->szKernel,&infokernel,tempBuf)) {
		printk("Error loading kernel %s\n",config->szKernel);
		while(1);
	} else {
		dwKernelSize = infokernel.fileSize;
		// moving the kernel to its final location
		memPlaceKernel(tempBuf, dwKernelSize);
		
		printk(" -  %d bytes...\n", infokernel.fileRead);
	}
	
	if( (strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd[0]) {

		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from FATX", config->szInitrd);
		wait_ms(50);
		if(! LoadFATXFilefixed(partition,config->szInitrd,&infoinitrd, (void*)INITRD_START)) {
			printk("Error loading initrd %s\n",config->szInitrd);
			while(1);
		}
		
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


CONFIGENTRY *LoadConfigCD(int cdromId) {
	long dwConfigSize=0;
	int n;
	int configLoaded=0;
	CONFIGENTRY *config, *currentConfigItem;

	memset((u8 *)KERNEL_SETUP,0,4096);

	printk("\2Please wait\n\n");
	//See if we already have a CDROM in the drive
	//Try for 8 seconds - takes a while to 'spin up'.
	I2CTransmitWord(0x10, 0x0c01); // close DVD tray
	for (n=0;n<32;++n) {
		dwConfigSize = BootIso9660GetFile(cdromId,"/linuxboo.cfg", (u8 *)KERNEL_SETUP, 0x800);
		if (dwConfigSize>0) {
			configLoaded=1;
			break;
		}
		wait_ms(250);
	}

	if (!configLoaded) {
		//Needs to be changed for non-xbox drives, which don't have an eject line
		//Need to send ATA eject command.
		I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
		wait_ms(2000); // Wait for DVD to become responsive to inject command
			
		VIDEO_ATTR=0xffeeeeff;
		printk("\2Please insert CD and press Button A\n\n");

		while(1) {
			if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) {
				I2CTransmitWord(0x10, 0x0c01); // close DVD tray
				wait_ms(500);
				break;
			}
			wait_ms(10);
		
			//Keep trying to read, in case somebody puts the tray in manually, without pressing A
			dwConfigSize = BootIso9660GetFile(cdromId,"/linuxboo.cfg", (u8 *)KERNEL_SETUP, 0x800);
			if (dwConfigSize>0) {
				configLoaded=1;
				break;
			}
		}						

		VIDEO_ATTR=0xffffffff;

		printk("Loading linuxboot.cfg from CDROM... \n");
		//Try to load linuxboot.cfg - if we can't after a while, give up.
		for (n=0;n<48;++n) {
			dwConfigSize = BootIso9660GetFile(cdromId,"/linuxboo.cfg", (u8 *)KERNEL_SETUP, 0x800);
			if (dwConfigSize>0) {
				configLoaded=1;
				break;
			}
			wait_ms(250);
		}
	}

	//Failed to load the config file
	if (!configLoaded) return NULL;
        
	// LinuxBoot.cfg File Loaded
	config = ParseConfig((char *)KERNEL_SETUP, dwConfigSize, NULL);
	//Populate the configs with the drive ID
	for (currentConfigItem = (CONFIGENTRY*) config; currentConfigItem!=NULL; currentConfigItem = (CONFIGENTRY*)currentConfigItem->nextConfigEntry) {
		//Set the drive ID and partition IDs for the returned config items
		currentConfigItem->drive=cdromId;
		currentConfigItem->bootType=BOOT_CDROM;
	}
	
	return config;
}

int LoadKernelCdrom(CONFIGENTRY *config) {
	u8* tempBuf;
	
	// Use INITRD_START as temporary location for loading the Kernel 
	tempBuf = (u8*)INITRD_START;
	dwKernelSize=BootIso9660GetFile(config->drive,config->szKernel, tempBuf, MAX_KERNEL_SIZE);

	if( dwKernelSize < 0 ) {
		printk("Not Found, error %d\nHalting\n", dwKernelSize); 
		while(1);
	} else {
		memPlaceKernel(tempBuf, dwKernelSize);
		printk(" -  %d bytes...\n", dwKernelSize);
	}

	if( (strncmp(config->szInitrd, "/no", strlen("/no")) != 0) && config->szInitrd) {
		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from CDROM", config->szInitrd);
		VIDEO_ATTR=0xffa8a8a8;
		
		dwInitrdSize=BootIso9660GetFile(config->drive, config->szInitrd, (void*)INITRD_START, MAX_INITRD_SIZE);
		if( dwInitrdSize < 0 ) {
			printk("Not Found, error %d\nHalting\n", dwInitrdSize); 
			while(1);
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
int BootLoadFlashCD(int cdromId) {
	
	u32 dwConfigSize=0, dw;
	int n;
	int cdPresent=0;
	u8* tempBuf;
	struct SHA1Context context;
	unsigned char SHA1_result[20];
	unsigned char checksum[20];
	 
	memset((u8 *)KERNEL_SETUP,0,4096);

	//See if we already have a CDROM in the drive
	//Try for 4 seconds.
	I2CTransmitWord(0x10, 0x0c01); // close DVD tray
	for (n=0;n<16;++n) {
		if((BootIso9660GetFile(cdromId,"/image.bin", (u8 *)KERNEL_SETUP, 0x10)) >=0 ) {
			cdPresent=1;
			break;
		}
		wait_ms(250);
	}

	if (!cdPresent) {
		//Needs to be changed for non-xbox drives, which don't have an eject line
		//Need to send ATA eject command.
		I2CTransmitWord(0x10, 0x0c00); // eject DVD tray
		wait_ms(2000); // Wait for DVD to become responsive to inject command
			
		VIDEO_ATTR=0xffeeeeff;
		
		printk("Please insert CD with image.bin file on, and press Button A\n");

		while(1) {
			if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) {
				I2CTransmitWord(0x10, 0x0c01); // close DVD tray
				wait_ms(500);
				break;
			}
        	        USBGetEvents();
			wait_ms(10);
		}						

		VIDEO_ATTR=0xffffffff;

		// wait until the media is readable
		while(1) {
			if((BootIso9660GetFile(cdromId,"/image.bin", (u8 *)KERNEL_SETUP, 0x10)) >=0 ) {
				break;
			}
			wait_ms(200);
		}
	}
	printk("CDROM: ");
	printk("Loading bios image from CDROM:/image.bin. \n");
	dwConfigSize=BootIso9660GetFile(cdromId, "/image.bin", (u8 *)KERNEL_PM_CODE, 256*1024);
	
	if( dwConfigSize < 0 ) { //It's not there
		printk("image.bin not found on CDROM... Halting\n");
		while(1) ;
	}

	printk("Image size: %i\n", dwConfigSize);
        if (dwConfigSize!=256*1024) {
		printk("Image is not a 256kB image - aborted\n");
		while (1);
	}
	SHA1Reset(&context);
	SHA1Input(&context,(u8 *)KERNEL_PM_CODE,dwConfigSize);
	SHA1Result(&context,SHA1_result);
	memcpy(checksum,SHA1_result,20);
	printk("Result code: %d\n", BootReflashAndReset((u8*) KERNEL_PM_CODE, (u32) 0, (u32) dwConfigSize));
	SHA1Reset(&context);
	SHA1Input(&context,(void *)LPCFlashadress,dwConfigSize);
	SHA1Result(&context,SHA1_result);
	if (memcmp(checksum,SHA1_result,20)==0) {
		printk("Checksum in flash matches - Flash successful.\nRebooting.");
		wait_ms(2000);
		I2CRebootSlow();	
	} else {
		printk("Checksum in Flash not matching - MISTAKE - Reflashing!\n");
		printk("Result code: %d\n", BootReflashAndReset((u8*) KERNEL_PM_CODE, (u32) 0, (u32) dwConfigSize));
	}
}
#endif //Flash


int ExittoLinux(CONFIGENTRY *config) {
	VIDEO_ATTR=0xff8888a8;
	BootPrintConfig(config);
	printk("     Kernel:  %s\n", (char *)(0x00090200+(*((u16 *)0x9020e)) ));
	printk("\n");
	{
		char *sz="\2Starting Linux\2";
		VIDEO_CURSOR_POSX=((vmode.width-BootVideoGetStringTotalWidth(sz))/2)*4;
		VIDEO_CURSOR_POSY=vmode.height-64;

		VIDEO_ATTR=0xff9f9fbf;
		printk(sz);
	}
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );
	startLinux((void*)INITRD_START, dwInitrdSize, config->szAppend);
}
	

void startLinux(void* initrdStart, unsigned long initrdSize, const char* appendLine)
{
	int nAta=0;
	// turn off USB
	BootStopUSB();
	setup( (void *)KERNEL_SETUP, initrdStart, initrdSize, appendLine);
        
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

	// orange, people seem to like that colour
	I2cSetFrontpanelLed(
		I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3 |
		I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3
	);
	         
	// Set framebuffer address to final location (for vesafb driver)
	(*(unsigned int*)0xFD600800) = (0xf0000000 | ((xbox_ram*0x100000) - FB_SIZE));
	
	// disable interrupts
	asm volatile ("cli\n");
	
	// clear idt area
	memset((void*)IDT_LOC,0x0,1024*8);
	
	__asm __volatile__ (
	"wbinvd\n"
	
	// Flush the TLB
	"xor %eax, %eax \n"
	"mov %eax, %cr3 \n"
	
	// Load IDT table (0xB0000 = IDT_LOC)
	"lidt 	0xB0000\n"
	
	// DR6/DR7: Clear the debug registers
	"xor %eax, %eax \n"
	"mov %eax, %dr6 \n"
	"mov %eax, %dr7 \n"
	"mov %eax, %dr0 \n"
	"mov %eax, %dr1 \n"
	"mov %eax, %dr2 \n"
	"mov %eax, %dr3 \n"
	
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
	"movl 	$0x90000, %esi\n"       // kernel setup area
	"ljmp 	$0x10, $0x100000\n"     // Jump to Kernel protected mode entry
	);
	
	// We are not longer here, we are already in the Linux loader, we never come back here
	
	// See you again in Linux then	
	while(1);
}

