/**************************************************************************/
/* BIOS start                                                             */
/*  Michael Steil                                                         */
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
#include "xbox.h"

#undef strcpy

unsigned long saved_drive;
unsigned long saved_partition;
grub_error_t errnum;
unsigned long boot_drive;

void console_putchar(char c) { printk("%c", c); }
extern unsigned long current_drive;

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

int strlen(const char *sz) {
	int n=0; while(*sz++) n++;
	return n;
}

char * strcpy(char *sz, const char *szc)
{
	char *szStart=sz;
	while(*szc) *sz++=*szc++;
	*sz='\0';
	return szStart;
}

int _strncmp(const char *sz1, const char *sz2, int nMax) {

	while((*sz1) && (*sz2) && nMax--) {
		if(*sz1 != *sz2) return (*sz1 - *sz2);
		sz1++; sz2++;
	}
	if(nMax==0) return 0;
	if((*sz1) || (*sz2)) return 0;
	return 0; // used up nMax
}

void StartBios(	int nDrive ) {
	char szKernelFile[128], szInitrdFile[128], szCommandline[1024];
	DWORD dwConfigSize=0;
	bool fHaveConfig=false;

	errnum=0; boot_drive=0; saved_drive=0; saved_partition=0x0001ffff; buf_drive=-1;
	current_partition=0x0001ffff; current_drive=0xff; buf_drive=-1; fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;

	strcpy(szCommandline, "root=/dev/hda2 devfs=mount kbd-reset"); // default



	if(nDrive==1) {  // CDROM
			BYTE ba[2048], baBackground[320*32*4];
			BYTE b;
			BYTE bCount=0, bCount1;
			int n;

			DWORD dwY=VIDEO_CURSOR_POSY;
			DWORD dwX=VIDEO_CURSOR_POSX;

			BootVideoBlit((DWORD *)&baBackground[0], 320*4, (DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*640*4)+VIDEO_CURSOR_POSX), 640*4, 32);

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
			
			VIDEO_ATTR=0xffffffff;

			VIDEO_CURSOR_POSX=dwX;
			VIDEO_CURSOR_POSY=dwY;
			BootVideoBlit(
				(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*640*4)+VIDEO_CURSOR_POSX),
				640*4, (DWORD *)&baBackground[0], 320*4, 32
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

							VIDEO_ATTR=0xff000000|(((bCount1>>1)+192)<<16)|(((bCount1>>1)+192)<<8)|(((bCount1>>1)+192)) ;

							printk("\2Waiting for drive\2");
							for(n=0;n<1000000;n++) { ; }
						}

						if(!fOkay) {
							printk("cdrom unhappy\n");
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
				(DWORD *)(FRAMEBUFFER_START+(VIDEO_CURSOR_POSY*640*4)+VIDEO_CURSOR_POSX),
				640*4, (DWORD *)&baBackground[0], 320*4, 32
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

//			while(1);
		}



	VIDEO_ATTR=0xffd8d8d8;

	if(nDrive==0) { // grub/reiserfs on HDD
		printk("  Looking for optional /boot/linuxboot.cfg ");
		VIDEO_ATTR=0xffa8a8a8;
		nRet=grub_open("\377\377\1\0/boot/linuxboot.cfg");
		dwConfigSize=filemax;
		nRet=2;
		if(nRet!=1) {
			printk("Not found, using defaults\n");
			strcpy(szKernelFile, "/boot/vmlinuz");
			strcpy(szInitrdFile, "/boot/initrd");
		} else {
			printk("okay\n");
			grub_read((void *)0x90000, 0x400);
			fHaveConfig=true;
		}
		grub_close();
	} else {  // ISO9660 traversal on CDROM

		printk("  Loading linuxboot.cfg from CDROM ");
		dwConfigSize=BootIso9660GetFile("/LINUXBOO.CFG", (BYTE *)0x90000, 0x400, 0x0);
		if((int)dwConfigSize<0) { // has to be there on CDROM
			printk("Unable to find it, halting\n");
			while(1) ;
		}
		printk("okay\n");
		fHaveConfig=true;
	}

	if(fHaveConfig) {
		char * szaTokens[] = { "kernel ", "initrd ", "append " };
		char * szaDestinations[] = { szKernelFile, szInitrdFile, szCommandline };
		int n1=0;
		while(n1<(sizeof(szaTokens)/sizeof(char *))) {
			char * sz=(char *)0x90000;
			int nSpan=dwConfigSize;
			while(nSpan--) {
				if(_strncmp(sz, szaTokens[n1], strlen(szaTokens[n1]))==0) { // hit
					int n=0;
					sz+=strlen(szaTokens[n1]);
					while(*sz!='\n') szaDestinations[n1][n++]=*sz++;
					szaDestinations[n1][n]='\0';
					n1++; nSpan=0;
				}
				sz++;
			}
		}
	}


	if(nDrive==0) { // grub/reiserfs on HDD
		char szGrub[256+4];
		szGrub[0]=0xff; szGrub[1]=0xff; szGrub[2]=0x01; szGrub[3]=0x00;

		printk("  Loading %s ", szKernelFile);
		VIDEO_ATTR=0xffa8a8a8;
		strcpy(&szGrub[4], szKernelFile);
		nRet=grub_open(szGrub);

		if(nRet!=1) {
			printk("Unable to load vmlinuz, Grub error %d\n", errnum);
			while(1) ;
		}
		dwKernelSize=grub_read((void *)0x90000, 0x400);
		nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
//		printk("Header size = 0x%x (0x%x)\n", nSizeHeader, *((BYTE *)0x901f1) );
		dwKernelSize+=grub_read((void *)0x90400, nSizeHeader-0x400);
		dwKernelSize+=grub_read((void *)0x00100000, filemax-nSizeHeader);
		grub_close();
		printk(" -  %d bytes done...\n", dwKernelSize);

		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s ", szInitrdFile);
		VIDEO_ATTR=0xffa8a8a8;

		strcpy(&szGrub[4], szInitrdFile);
		nRet=grub_open(szGrub);
		if(filemax==0) {
			printf("Empty file\n"); while(1);
		}
		if( (nRet!=1 ) || (errnum)) {
			printk("Unable to load initrd, Grub error %d\n", errnum);
			while(1) ;
		}
		dwInitrdSize=grub_read((void *)0x03000000, filemax);
		printk(" - %d bytes done\n", dwInitrdSize);
		grub_close();


	} else {  // ISO9660 traversal on CDROM
		char szIso[256];

		BootIso9660ConvertAsciiStringToDchar8point3String(szIso, szKernelFile);

		printk("  Loading %s from CDROM ", szIso);
		VIDEO_ATTR=0xffa8a8a8;

		dwKernelSize=BootIso9660GetFile(szIso, (BYTE *)0x90000, 0x400, 0x0);
		nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
		dwKernelSize+=BootIso9660GetFile(szIso, (void *)0x90400, nSizeHeader-0x400, 0x400);
		dwKernelSize+=BootIso9660GetFile(szIso, (void *)0x00100000, 4096*1024, nSizeHeader);

		printk(" -  %d bytes done...\n", dwKernelSize);

		BootIso9660ConvertAsciiStringToDchar8point3String(szIso, szInitrdFile);

		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading %s from CDROM ", szIso);
		VIDEO_ATTR=0xffa8a8a8;

		dwInitrdSize=BootIso9660GetFile(szIso, (void *)0x03000000, 4096*1024, 0);
		printk(" - %d bytes done\n", dwInitrdSize);

	}

	VIDEO_ATTR=0xff8888a8;
	printk("     Kernel Version:  %s\n", (char *)(0x00090200+(*((WORD *)0x9020e)) ));

	printf("\n");
	{
		char *sz="\2Starting Linux\2";
		VIDEO_CURSOR_POSX=((640-BootVideoGetStringTotalWidth(sz))/2)*4;
		VIDEO_CURSOR_POSY=VIDEO_HEIGHT-64;

		VIDEO_ATTR=0xff9f9fbf;
		printk(sz);
	}

		// we have to copy the GDT to a safe place, because one of the first
		// things Linux is going to do is switch to paging, making the BIOS
		// inaccessible and so crashing if we leave the GDT in BIOS

	memcpy((void *)0xa0000, (void *)&baGdt[0], 0x50);
	
		// prep the Linux startup struct

	setup( (void *)0x90000, (void *)0x03000000, (void *)dwInitrdSize, szCommandline);

	// force the HDD into a good mode 0x40 ==UDMA | 2 == UDMA2
	BootIdeSetTransferMode(0, 0x42); // best transfer mode without 80-pin cable

    __asm __volatile__ (

//	"wbinvd \n"

//	"movl %cr0, %eax \n"
//	"orl $0x60000000, %eax \n"
//	"movl %eax, %cr0 \n"
		// kill the cache

	"mov %cr0, %eax \n"
	"orl	$0x60000000, %eax \n"
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
		"movl	$0xf0000004, %eax  \n"// == Cacheable
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

			/* turn on normal cache */

		"movl	%cr0, %eax \n"
		"mov %eax, %ebx \n"
		"andl	$0x9FFFFFFF,%eax \n"
		"movl	%eax, %cr0 \n"

	"movl $0x90000, %esi        \n"
	"xor %ebx, %ebx \n"
	"xor %eax, %eax \n"
	"xor %ecx, %ecx \n"
	"xor %edx, %edx \n"
	"xor %edi, %edi \n"

	"lgdt 0xa0030 \n"
   "ljmp $0x10, $0x100000 \n" );


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
