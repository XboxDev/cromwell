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
        
	
.align 16
tableGdt:
        
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x00 dummy
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00 // 0x08 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00 // 0x10 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00 // 0x18 data32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0x8f, 0x00 // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0x8f, 0x00 // 0x28 data16
	
	// This is the GDT header having 8 bytes
	.word 0x30	// 30 Bytes Big GDT
	.long 0xa0000	// GDT located @ 0xa0000
	.word 0		// Fillup
	
	// Summary = 0x38 bytes
	

.align 16
tableIdtDescriptor:

	.word 2048
	.long 0xb0000
	.word 0     // fill Word, so we get alliged again
        
        // We are again Dword Alligated now
        
	.globl start_linux
start_linux:
       	
       	// i am sure, the irq's are turned off, but who knows this for sure
       	// never trust anybody
       	cli
  	
	// kill the cache  = Disable bit 30 + 29 = CD + NW 
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
       	mov 	%cr0, %eax
	orl	$0x60000000, %eax
	mov	%eax, %cr0
	wbinvd

	// Move the Stack Pointer to Upper Limit
	
	movl    $RAM_BASE, %esp
	
	// We kill the Local Descriptor Table        
        xor	%eax, %eax
	lldt	%ax	
	
	/*
	IMPORTANT !
	Linux Expects the GDT located at this position, so we have to move it to 0xa0000
	*/
	
	// We copy the GDT at the final location in RAM
	mov 	$0xa0000, %edi
	mov 	$tableGdt, %esi
	mov 	$0x40, %ecx  // this value should be GDT size + GDT header
	repe movsb
	
	// We load a new GDT Table 
	lgdt 0xa0030	// this is the pointer to the GDT header

        
        // We clear the IDT in RAM   (IDT located @  0xb0000 )
        xor %eax,%eax
	mov $0x5000,%ecx
	mov $0xb0000,%edi
	rep stosb
	
	// We load the Interrupt Descriptor Table
	lidt tableIdtDescriptor

       // FPU SETUP
	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
	clts
	fninit
	
	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
  
  	// We flush the TLB
	mov	%cr3, %eax
	mov	%eax, %cr3

	// Clear Mmeory Type register
	movl	$0x2ff, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr


#if 0
	// MTRR for RAM
	// from address 0, Writeback Caching, 64MB range
        movl	$0x200, %ecx 		
	movl	$0x00000000, %edx 	 
	movl	$0x00000006, %eax 	// == WB_CACHE == 6
	wrmsr

	// MASK0 set to 0xffc400[000] == 64M    
        movl	$0x201, %ecx  		 
	movl	$0x0000000f, %edx 	
	movl	$0xfc000800, %eax  	
	wrmsr
#else      


	// Very funny. We have no memory there, but if we set the memory from 64 to 192MB to 0 (UC type)
	// we freeze .. maybe a kind of memory from the Video Card
      	// So we set 64 MB memory + 128 MB (found out with try and error)
      	// PLEASE find the problem, it is happening in loading from .xbe mode
      	
      	movl	$0x200, %ecx 		
	movl	$0x00000000, %edx 	
	movl	$0x00000006, %eax 	// == WB_CACHE == 6
	wrmsr

	// MASK0 set to  == 192 MB
        movl	$0x201, %ecx  		
	movl	$0x0000000f, %edx 	
	movl	$0xf4000800, %eax  	
	wrmsr


#endif


#if 0
	/*
	Mega Mysterios thing.
	
	MS sets the Memory Type for the flash to 5 (means WP write-protected)
	Seems to be locical.
	
	BUT !
	Linux loader crashes then
	
	very mysterious 
	also, they defined only !?? the last 512 kbyte of the flash, no idea why
	But i did 1MB
	
	BUT: As this memory type == 0, and the global Memory type is set to 0, we can leave out the whole thing.
	
	*/
	
	// MTRR for FLASH   
	movl	$0x202, %ecx		
	movl	$0x00000000, %edx 	
	movl	$0xFff00000, %eax 	
	wrmsr

        movl	$0x203, %ecx             
	movl	$0x0000000f, %edx 
	movl	$0xfff00800, %eax  
	wrmsr	        
#else   
  	xor	%eax, %eax
	xor	%edx, %edx
	movl	$0x202, %ecx	
        wrmsr                                                   
	movl	$0x203, %ecx	
        wrmsr                                                   

#endif
    
        xor	%eax, %eax
	xor	%edx, %edx
	
	movl	$0x204, %ecx		// IA32_MTRR_PHYS Base 2
        wrmsr                                                   
	movl	$0x205, %ecx		// IA32_MTRR_PHYS_MASK 2
        wrmsr                                                   
                                                
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


	// Ok, we tell now the processor, we finished Memory Type definitions
	// We set the Default Memory Type in the Box now to Type == 0 .. Means UC .. Uncacheable
	
	movl	$0x2ff, %ecx
	xor	%edx, %edx
	movl	$0x800, %eax 		//Enable MTRRs
	wrmsr
      

        
   
	movl	$0x600, %eax
	mov	%eax, %cr4

	mov	%eax, %eax       // Flush the TLB    and resets it
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





