/**************************************************************************/
/* BIOS start                                                             */
/*  Michael Steil                                                         */
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

unsigned long saved_drive;
unsigned long saved_partition;
grub_error_t errnum;
unsigned long boot_drive;

void console_putchar(char c) { printk("%c", c); }
extern unsigned long current_drive;

void setup(void* KernelPos, void* PhysInitrdPos, void* InitrdSize, char* kernel_cmdline);

	int nRet;
	DWORD dwKernelSize;
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


void StartBios() {

	errnum=0; boot_drive=0; saved_drive=0; saved_partition=0x0001ffff; buf_drive=-1;
	current_partition=0x0001ffff; current_drive=0xff; buf_drive=-1; fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;


//	__asm __volatile ( "cli");
	VIDEO_ATTR=0xffd8d8d8;
	printk("  Loading /boot/vmlinuz ");
	VIDEO_ATTR=0xffa8a8a8;
	nRet=grub_open("\377\377\1\0/boot/vmlinuz");

	if(nRet==1) {

		dwKernelSize=grub_read((void *)0x90000, 0x400);
		nSizeHeader=((*((BYTE *)0x901f1))+1)*512;
//		printk("Header size = 0x%x (0x%x)\n", nSizeHeader, *((BYTE *)0x901f1) );
		dwKernelSize+=grub_read((void *)0x90400, nSizeHeader-0x400);
		dwKernelSize+=grub_read((void *)0x00100000, filemax-nSizeHeader);
		grub_close();

		printk(" -  %d bytes done...\n", dwKernelSize);
		VIDEO_ATTR=0xff8888a8;
		printk("     Kernel Version:  %s\n", (char *)(0x00090200+(*((WORD *)0x9020e)) ));

		VIDEO_ATTR=0xffd8d8d8;
		printk("  Loading initrd ");
		VIDEO_ATTR=0xffa8a8a8;

		nRet=grub_open("\377\377\1\0/boot/initrd");
		if(filemax==0) {
			printf("Empty file\n"); while(1);
		}
		if( (nRet==1 ) && (errnum==0)) {
			nRet=grub_read((void *)0x03000000, filemax);
			printk(" - %d bytes done\n", nRet);
			grub_close();

			printf("\n");
			{
				char *sz="\2Starting Linux\2";
				VIDEO_CURSOR_POSX=((640-BootVideoGetStringTotalWidth(sz))/2)*4;
				VIDEO_CURSOR_POSY=VIDEO_HEIGHT-64;

				VIDEO_ATTR=0xff9f9fbf;
				printk(sz);
			}

			memcpy((void *)0xa0000, (void *)&baGdt[0], 0x50);

			setup( (void *)0x90000, (void *)0x03000000, (void *)nRet, "root=/dev/hda2 devfs=mount kbd-reset");

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


		} else {
			printk("Unable to load, Grub error %d\n", errnum);
		}

//		DumpAddressAndData(0x100000, (void *)0x100000, 1024);
	} else {
			printk("Unable to load, Grub error %d\n", errnum);
	}

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
