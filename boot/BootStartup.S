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
	// Move the Stack Pointer to Upper Limit
	movl    $RAM_BASE, %esp

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

	clts
	fninit
	//.byte 0xDB,0xE4         /* fsetpm for 287, ignored by 387 */	


	nop
	nop
	nop    
	
	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
	
	// kill the cache  = Disable bit 30 + 29 = CD + NW 
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
       	mov 	%cr0, %eax
	orl	$0x60000000, %eax
	mov	%eax, %cr0
	wbinvd

  
  	// We clear the cr3 register
	mov	%eax, %eax
	mov	%eax, %cr3

	// Clear Mmeory Type register
	movl	$0x2ff, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr



	// MTRR for RAM
	// from address 0, Writeback Caching, 60MB range
        movl	$0x200, %ecx 		// 
	movl	$0x00000000, %edx 	// 
	movl	$0x00000006, %eax 	// == WB_CACHE == 6
	wrmsr

	// MASK0 set to 0xffc400[000] == 60M    
        movl	$0x201, %ecx  		// 
	movl	$0x0000000f, %edx 	// 0x0f
	movl	$0xfc400800, %eax  	// 0xfc000800 b11= enable MTRR
	wrmsr


        // MTRR for Video Memory (last 4MByte of shared Ram, starts at 60MB)
	// Writethrough type trumps Writeback for overlapping region
        movl	$0x202, %ecx           	// IA32_MTRR_PHYS Base 2
	movl	$0x00000000, %edx 	// 0x00
	movl	$0x03C00004, %eax 	// == Writethrough == 4
	wrmsr
	       
	// MASK0 set to 0xfffC00[000] == 4M
	movl	$0x203, %ecx            // IA32_MTRR_PHYS_MASK 2
	movl	$0x0000000f, %edx // 0x0f
	movl	$0xFFC00800, %eax  // 0xffC00800
	wrmsr
	
	// MTRR for FLASH +PCI config 
	movl	$0x204, %ecx		// IA32_MTRR_PHYS Base 1 
	movl	$0x00000000, %edx 	// 0x00
	movl	$0xF0000000, %eax 	// As it is a Flash, we set the Flag to Uncacheable =0
	wrmsr

        movl	$0x205, %ecx             
	movl	$0x0000000f, %edx 
	movl	$0xF0000800, %eax  
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


	// Define Memory in IA32_MTRR_DEF_TYPE
	movl	$0x2ff, %ecx
	xor	%edx, %edx
	movl	$0x806, %eax 		//Enable MTRRs
	wrmsr
      

 


/*
  These Init Codes Can be found in the 2'nd Bootloader

*/ 
        
   
	movl	$0x600, %eax
	mov	%eax, %cr4

	mov	%eax, %eax
	mov	%eax, %cr3
  
 
   	/* turn on normal cache */
	//  bit 30 + 29 = CD + NW 
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
	
	movl	%cr0, %eax
	mov 	%eax, %ebx
	andl	$0x9fFFffFF,%eax // we additionally clear the WP flag too
	movl	%eax, %cr0
	wbinvd
	
	
	
   
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





