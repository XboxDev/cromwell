/*
 *
 *  BIOS ROM Startup Assembler
 */

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "consts.h"


	.code32

.section .text, "ax"
         .org 0x00
         jmp	start_linux
         
.global	Cromwellconfig
Cromwellconfig:      	
        .org 0x20       
        .long 0x0	// if XBE, then this bit is 0, if Cromwell mode, the bit is set to 1 by the Startuploader
        .long 0x0	// ImageRetryLoads
        .long 0x0	// Bank, from where Loaded
        .long 0x0	// 0 .. Bios = 256 k, 1 .. Bios = 1MB
        
	.org 0x40       // We have linkbase for this programm 0x03A00000 + 0x40 = divisable /4 .. so CPU likes it
tableGdt:

	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x00 dummy
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x08 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x10 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00 // 0x18 data32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0x8f, 0x00 // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0x8f, 0x00 // 0x28 data16
        .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Dummy
  
  	// We are Dword Alligated until now
tableGdtDescriptor:

	.word 0x30
	.long tableGdt
	.word 0    // fill Word, so we get alliged again
        
        // We are again Dword Alligated now
        
tableIdtDescriptor:

	.word 2048
	.long 0x00400000
	.word 0     // fill Word, so we get alliged again
        
        // We are again Dword Alligated now
        
	.globl start_linux
start_linux:


        // Clear Intel Interrupts in Processor Register
	// Everytime very good, specially when comming out of a running envoronment
	
	/*  
  	 There are Some Nops here
  	 Basically, they are useless, it works without them too.
  	 BUT !
  	 IA32 Manual Recommends some waiting after doing eg. CPU swapout or reloading of the IDT, GDT, as the CPU
  	 nees some minor time to settel.
  	 basically, they recommend a small "dummy" loop, but NOP is also good, but wastes more space .. who cares
  	 we have 733 Mhz here
  	*/
  	
        nop
	nop
	nop
	
	movl	$0x1b, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr

        // We clear the IDT in RAM
        xor %eax,%eax
	mov $0x5000,%ecx
	mov $0x400000,%edi
	rep stosb

	//call ClearIDT
	lgdt tableGdtDescriptor
	lidt tableIdtDescriptor

       // FPU SETUP
	movl %cr0,%eax          
	andl $0x80000011,%eax   
	orl $0x50022,%eax       
	movl %eax,%cr0
	clts
	fninit
	.byte 0xDB,0xE4         /* fsetpm for 287, ignored by 387 */	


	nop
	nop
	nop    
	
	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
	
	// kill the cache
       	mov %cr0, %eax
	orl	$0x60000000, %eax
	mov	%eax, %cr0
	wbinvd

  
  	// Sennles Task ?
//	mov	%cr3, %eax
//	mov	%eax, %cr3

	// Clear Mmeory Type register
	movl	$0x2ff, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr

/*
  We are setting the Ram Init's now to set up the Regions in the Ram

*/ 

	// MTRR for RAM
	// from address 0, Writeback Caching, 64MB range
        movl	$0x200, %ecx 		// IA32_MTRR_PHYS Base 0
	movl	$0x00000000, %edx 	// 0x00
	movl	$0x00000006, %eax 	// == WB_CACHE == 6
	wrmsr

	// MASK0 set to 0xffc000[000] == 64M    
        movl	$0x201, %ecx  		// IA32_MTRR_PHYS_MASK 0
	movl	$0x0f, %edx 		// 0x0f
	movl	$0xfc000800, %eax  	// 0xfc000800 b11= enable MTRR
	wrmsr
	
	// MTRR for BIOS
	movl	$0x202, %ecx		// IA32_MTRR_PHYS Base 1 
	movl	$0x00000000, %edx 	// 0x00
	movl	$0xFFF00006, %eax 	// == WB cache  NOTE SIZING!  Only last 1M cached!
	wrmsr

        // MASK0 set to 0xff0000[000] == 16M
        movl	$0x203, %ecx             // IA32_MTRR_PHYS_MASK 1
	movl	$0x0000000f, %edx // 0x0f
	movl	$0xFFF00800, %eax  // 0xff000800
	wrmsr
        
        
        // MTRR for Video Memory (last 4MByte of shared Ram)
	// Writethrough type trumps Writeback for overlapping region
        movl	$0x204, %ecx           	// IA32_MTRR_PHYS Base 2
	movl	$0x00000000, %edx 	// 0x00
	movl	$0x03C00004, %eax 	// == Writethrough == 4
	wrmsr
	
	// MASK0 set to 0xfffC00[000] == 4M
	movl	$0x205, %ecx            // IA32_MTRR_PHYS_MASK 2
	movl	$0x0000000f, %edx // 0x0f
	movl	$0xFFC00800, %eax  // 0xffC00800
	wrmsr

        xor	%eax, %eax
	xor	%edx, %edx
	
	movl	$0x206, %ecx		// IA32_MTRR_PHYS Base 3
        wrmsr                                                   
	movl	$0x207, %ecx		// IA32_MTRR_PHYS_MASK 3
        wrmsr                                                   

	movl	$0x208, %ecx		// IA32_MTRR_PHYS Base 4
        wrmsr                                                   
	movl	$0x209, %ecx		// IA32_MTRR_PHYS_MASK 4
        wrmsr                                                   

	movl	$0x20a, %ecx		// IA32_MTRR_PHYS Base 5
        wrmsr                                                   
	movl	$0x20b, %ecx		// IA32_MTRR_PHYS_MASK 5
        wrmsr                                                   

	movl	$0x20c, %ecx		// IA32_MTRR_PHYS Base 6
        wrmsr                                                   
	movl	$0x20d, %ecx		// IA32_MTRR_PHYS_MASK 6
        wrmsr

	movl	$0x20e, %ecx		// IA32_MTRR_PHYS Base 7
        wrmsr
	movl	$0x20f, %ecx		// IA32_MTRR_PHYS_MASK 7
        wrmsr                                                   

/*
	// MTRR for Filtror comms area

	movl	$0x206, %ecx   		// IA32_MTRR_PHYS Base 3
	movl	$0x00000000, %edx 	// 0x00
	movl	$0xffffe000, %eax 	// == Writethrough == 4
	wrmsr
	
	movl	$0x207, %ecx    	// IA32_MTRR_PHYS_MASK 3
					// MASK0 set to 0xfffC00[000] == 4M
	movl	$0x00000000, %edx 	// 0x0f
	movl	$0xFFFFF800, %eax  	// 0xffC00800
	wrmsr

*/
	// Define Memory in IA32_MTRR_DEF_TYPE
	movl	$0x2ff, %ecx
	xor	%edx, %edx
	movl	$0xc06, %eax 		//Enable MTRRs
	wrmsr
      /*
	
        // Cache turnon ... but we do not do this, this might be dangerous now.
	movl	%cr0, %eax
	mov 	%eax, %ebx
	andl	$0x9FFFFFFF,%eax
	movl	%eax, %cr0
	
	*/

     /*
        // No idea what this is, so i leave it out
        
	// set up selectors for everything

	mov	$0x18, %eax
	.byte 0x8e, 0xd8  // from 2bl first init

//	movl	$ 0x00200000,%esp
	movl	$ 0x00080000,%esp
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss
  
  	xor	%eax, %eax
	movw    %ax, %fs
	movw    %ax, %gs
        
        cld
       */
       
/*
  We are setting the Page directory's Now

*/ 

#if 1
		// set up the page directory

	mov $0xf000, %edi
	mov $0x40, %ecx  // 64 x 4M pages
	mov $0xe3, %eax  // page dir entry: present, cacheable, 4M page

	// +0-+FF AND +800-+8FF, 256M contiguous region pointing at linear 0-256M

patspin:
	movl %eax, 0x800(%edi)
	stosl  // .byte 0xab // stosd
	add $0x400000, %eax // bump by 4MBytes page size
	loop	patspin

	// +100-+7FF AND +900-+FFF, EMPTY

	mov	$0x1c0, %ecx
	xor	%eax, %eax  // page dir entry: not present, 4K page
	mov $0xfffc00f3, %eax  // 4M page, flash area area

patspin2:
	movl %eax, 0x800(%edi)
	stosl // .byte 0xab // stosd
	loop	patspin2

	mov $0xf000, %edi

	mov $0x3c000f3, %eax
	mov %eax, 0x3c(%edi) // page table f03c
	mov %eax, 0x83c(%edi) // page table f03c

	mov $0xf063, %eax  // 4K page page table itself
	mov %eax, 0xc00(%edi) // page table fc00

	mov $0xff0000f3, %eax  // 4M page, flash area area NOT CACHEABLE
	mov %eax, 0xff0(%edi)// page table entry fffc
	mov $0xff4000e3, %eax  // 4M page, flash area area
	mov %eax, 0xff4(%edi)// page table entry fffc
	mov $0xff8000e3, %eax  // 4M page, flash area area
	mov %eax, 0xff8(%edi)// page table entry fffc
	mov $0xffc000e3, %eax  // 4M page, flash area area
	mov %eax, 0xffc(%edi)// page table entry fffc

	mov $0xfd0000fb, %eax // not cacheable 4M area starting 0xfd000000
	mov	%eax, %ebx

	shr	$0x14, %ebx  // divide ebx by 1M, 0xfd0
	add	%edi, %ebx // add to page table base 0xffd0

		// contiguous 16M area starting 0xfd000000 - 0xfdffffff in page tbl at 0xffd0+

	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)

	mov 	$0xfe0000fb, %eax // not cacheable 4M area starting 0xfe000000
	mov	%eax, %ebx

	shr	$0x14, %ebx  // divide ebx by 1M, 0xfe0
	add	%edi, %ebx // add to page table base 0xffe0

	// contiguous 16M area starting 0xfe000000 - 0xfeffffff in page tbl at 0xffe0+

	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)
	add	$4, %ebx
	add	$0x400000, %eax
	mov	%eax, (%ebx)


	mov	$0x277, %ecx
	mov	$0x70106, %eax // was 70106
	mov	%eax, %edx
	wrmsr
	
	mov	%cr0, %eax
	mov	%eax, %ebx
	and 	$0xdfffffff, %eax
	or	$0x40000000, %eax
	mov	%eax, %cr0
	wbinvd

//	invd

	mov	%ebx, %cr0   // Sensless ?

	mov	%cr4, %eax
	or	$0x610, %eax
	mov	%eax, %cr4
        wbinvd
        
	mov	$0xf000, %eax
	mov	%eax, %cr3
        wbinvd
        
	mov	%cr0, %eax
	or	$0x80010020, %eax
	mov	%eax, %cr0
	
	wbinvd

	jmp selfPipeKill

selfPipeKill:

	mov 	$0x00040000, %esp

#endif


/*
  These Init Codes Can be found in the 2'nd Bootloader

*/ 
   
        // something for Video ?
	mov 	$0x80000854, %eax
	movw 	$0xcf8, %dx
	outl 	%eax, %dx
	movw 	$0xcfc, %dx
	inl 	%dx, %eax 
	orl 	$0x88000000, %eax
	outl 	%eax, %dx 
     
	
	// Important
	mov 	$0x80000064, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx
	inl 	%dx, %eax 
	orl 	$0x88000000, %eax 
	outl	%eax, %dx 
	
	// Important
	mov 	$0x8000006c, %eax 
	movw 	$0xcf8, %dx
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	inl 	%dx, %eax
	push 	%eax 
	andl 	$0xfffffffe, %eax 
	outl	%eax, %dx 
	pop 	%eax
	outl	%eax, %dx 
      	
      	
      	/*
	// CPU Whoami   ? sesless ?
	mov 	$0x80000080, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	movl 	$0x100, %eax 
	outl	%eax, %dx 
        */
	mov 	$0x8000088C, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	movl 	$0x40000000, %eax 
	outl	%eax, %dx
	
	jmp BootResetAction

       


///////////////////////////////////////////
//
//  Interrupt Service Routines
//


		.global	IntHandlerTimer0
IntHandlerTimer0:
	cli
	pusha
	pushf
	call	IntHandlerCTimer0
	mov	$0x20, %al
	outb	%al, $0x20
	popf
//	sti
	popa
	iret


		.global	IntHandler1
IntHandler1:
	pusha
	pushf
	cli
	call	IntHandler1C
	mov	$0x21, %al
	outb	%al, $0x20
	popf
//	sti
	popa
	iret


		.global	IntHandler2
IntHandler2:
	cli
	pusha
	pushf
	call	IntHandler2C
	mov	$0x22, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret



		.global	IntHandler3
IntHandler3:
	cli
	pusha
	pushf
	call	IntHandler3VsyncC
	mov	$0x23, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret


		.global	IntHandler4
IntHandler4:
	cli
	pusha
	pushf
	call	IntHandler4C
	mov	$0x24, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret


		.global	IntHandler5
IntHandler5:
	cli
	pusha
	pushf
	call	IntHandler5C
	mov	$0x25, %al
	outb	%al, $0x20
	popf
	popa
//	sti
	iret


		.global	IntHandler6
IntHandler6:
	cli
	pusha
	pushf
	call	IntHandler6C
	mov	$0x26, %al
	outb	%al, $0x20
	popf
	popa
	sti
	iret



		.global	IntHandler7
IntHandler7:
	cli
	pusha
	pushf
	call	IntHandler7C
	mov	$0x27, %al
	outb	%al, $0x20
	popf
	popa
	sti	
	iret



	.global IntHandler8
IntHandler8:
	cli
	pusha
	pushf
	call	IntHandler8C
		// EOI on master and slave needed
	mov	$0x60, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret




	.global IntHandler9
IntHandler9:
	pusha
	pushf
	cli
	call	IntHandler9C
		// EOI on master and slave needed
	mov	$0x61, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 10 interrupts

	.global IntHandler10
IntHandler10:
	pusha
	pushf
	cli
	call	IntHandler10C
		// EOI on master and slave needed
	mov	$0x62, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 11 interrupts

	.global IntHandlerI2C
IntHandlerI2C:
	pusha
	pushf
	cli
	call	IntHandlerCI2C

		// EOI on master and slave needed
	mov	$0x63, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 12 interrupts

	.global IntHandlerSmc
IntHandlerSmc:
	cli
	pusha
	pushf

	call	IntHandlerCSmc

		// acknowledge EXTSMI# action (from PIC p6)

	mov	$0x8020, %dx
	inw %dx, %ax
	or $0x0200, %ax
	outw	%ax, %dx

		// EOI on master and slave needed
	mov	$0x64, %al // is int12
	out	%al, $0xa0
	mov	$0x62, %al // do cascaded master
	out	%al, $0x20
	popf
	popa
	iret

		// Int 13 interrupts

	.global IntHandler13
IntHandler13:
	cli
	pusha
	pushf

	call	IntHandler13C
		// EOI on master and slave needed
	mov	$0x65, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

			// Int 14 interrupts

	.global IntHandlerIde
IntHandlerIde:
	cli
	pusha
	pushf
	
	call	IntHandlerCIde

		// EOI on master and slave needed
	mov	$0x66, %al // was $0x20
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

		// Int 15 interrupts

	.global IntHandler15
IntHandler15:
	cli
	pusha
	pushf
	
	call	IntHandler15C
		// EOI on master and slave needed
	mov	$0x67, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret


		// unused interrupts on master PIC

	.global IntHandlerUnused
IntHandlerUnused:
	cli
	pusha
	pushf
	
	call	IntHandlerUnusedC
	mov	$0x20, %al
	out	%al, $0x20
	popf
	popa
	iret

		// unused interrupts on slave PIC

	.global IntHandlerUnusedPic2
IntHandlerUnusedPic2:
	cli
	pusha
	pushf
	
	call	IntHandlerUnusedC2
	mov $0xffff, %ax
	mov	$0x8028, %dx
	outw	%ax, %dx
	mov	$0x80cc, %dx
	mov	$0x40, %al
	outb	%al, %dx

	mov	$0x20, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret

	.global SpareIntNop
SpareIntNop:
	iret


		// CPU Exception Interrupts

	.global IntHandlerException0
IntHandlerException0:
	pusha
	pushf
	cli
	call	IntHandlerException0C
	popf
	popa
	iret

	.global IntHandlerException1
IntHandlerException1:
	pusha
	pushf
	cli
	call	IntHandlerException1C
	popf
	popa
	iret

	.global IntHandlerException2
IntHandlerException2:
	pusha
	pushf
	cli
	call	IntHandlerException2C
	popf
	popa
	iret

	.global IntHandlerException3
IntHandlerException3:
	pusha
	pushf
	cli
	call	IntHandlerException3C
	popf
	popa
	iret

	.global IntHandlerException4
IntHandlerException4:
	pusha
	pushf
	cli
	call	IntHandlerException4C
	popf
	popa
	iret

	.global IntHandlerException5
IntHandlerException5:
	pusha
	pushf
	cli
	call	IntHandlerException5C
	popf
	popa
	iret

	.global IntHandlerException6
IntHandlerException6:
	pusha
	pushf
	cli
	call	IntHandlerException6C
	popf
	popa
	iret

	.global IntHandlerException7
IntHandlerException7:
	pusha
	pushf
	cli
	call	IntHandlerException7C
	popf
	popa
	iret

	.global IntHandlerException8
IntHandlerException8:
	pusha
	pushf
	cli
	call	IntHandlerException8C
	popf
	popa
	iret

	.global IntHandlerException9
IntHandlerException9:
	pusha
	pushf
	cli
	call	IntHandlerException9C
	popf
	popa
	iret

	.global IntHandlerExceptionA
IntHandlerExceptionA:
	pusha
	pushf
	cli
	call	IntHandlerExceptionAC
	popf
	popa
	iret

	.global IntHandlerExceptionB
IntHandlerExceptionB:
	pusha
	pushf
	cli
	call	IntHandlerExceptionBC
	popf
	popa
	iret

	.global IntHandlerExceptionC
IntHandlerExceptionC:
	pusha
	pushf
	cli
	call	IntHandlerExceptionCC
	popf
	popa
	iret

	.global IntHandlerExceptionD
IntHandlerExceptionD:
	pusha
	pushf
	cli
	call	IntHandlerExceptionDC
	popf
	popa
	iret

	.global IntHandlerExceptionE
IntHandlerExceptionE:
	pusha
	pushf
	cli
	call	IntHandlerExceptionEC
	popf
	popa
	iret

	.global IntHandlerExceptionF
IntHandlerExceptionF:
	pusha
	pushf
	cli
	call	IntHandlerExceptionFC
	popf
	popa
	iret

	.global IntHandlerException10
IntHandlerException10:
	pusha
	pushf
	cli
	call	IntHandlerException10C
	popf
	popa
	iret





