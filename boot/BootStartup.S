/*
 *
 *  BIOS ROM Startup Assembler
 *  (C)2002 Andy, Michael, Paul, Steve
 * Original top and bottom ROM code by Steve from an idea by Michael
 *
 *  This contains the magic values read by the MCPX before the CPU gets started,
 *  as well as a minimal Xcode program which exits the Xcode interpreter immediately
 *  by turning off the MCPX ROM while the interpreter is running.
 *  The CPU will start executing *our* ROM instructions at location 0xFFFFFE62.
 *  See label jmp_code in this file for where execution picks up
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

.section .low_rom, "ax"

					/* MCPX Magic Values */

		.long 0xff000009
		.long 0xff000008
mcpx_magic:  // needs to be DWORD aligned
    .long 0x2b16d065, 0x3346322d
    .byte 1, 1, 1, 1, 8, 8, 8, 8, 1, 8, 0, 0
		.byte 0x8a, 0x7c, 0xfc, 0xc8
		.byte 0x13, 0x02, 0x29, 0x44, 0x98, 0x49, 0x00, 0x90
		.long 0x0, 0xffffffff, 0xffffffff

		.org 0x60

		jmp	start_linux

		.org 0x6c
		.byte 0x07, 0x01
		.org 0x70

		.byte 0x0f, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40
		.byte 0x70, 0x00, 0xd1, 0x12, 0x90, 0x0c, 0x00, 0x00

	//.section .text, "ax"
	//.text
        .section rom, "ax"


	/*
 *
 * this is the entry code.  We get here by a jump
 * from the top of rom, and we get there by the
 * single xcode which exposes the top 512
 * bytes and nops take us to a long jump.
 *
 * We arrive here in protected mode with the cs set
 * to 08 offset in the gdt, which has been set to
 * 4GB flat mode (code and data). Data is 0x10 offset.
 * Caching is disabled since we did not go through the EE opcode.
 */


 	//

.globl tableGdt
tableGdt:

	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x00 dummy
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x08 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x10 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00 // 0x18 data32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0x8f, 0x00 // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0x8f, 0x00 // 0x28 data16

tableGdtDescriptor:

	.word 0x30
	.long tableGdt
	.word 0

tableIdtDescriptor:

	.word 2048
	.long 0xef000
	.word 0

	.globl start_linux
start_linux:


	// NOTE:
	/*
		Xromwell & Cromwell is sarting here
	*/
	
        // Clear Intel Interrupts in Processor Register
	// Everytime very good, specially when comming out of a running envoronment
		
	movl	$0x1b, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr
	
	// Interrupts now Dead
	

	lidt tableIdtDescriptor
	lgdt tableGdtDescriptor
	ljmp $0x10, $entrypoint_start

entrypoint_end: 
	
       
       
       
       	push %edx
	push %eax 

	mov $0x80000854, %eax
	movw $0xcf8, %dx
	outl %eax, %dx
	movw $0xcfc, %dx
	inl %dx, %eax 
	orl $0x88000000, %eax
	outl %eax, %dx 

	mov $0x80000064, %eax 
	movw $0xcf8, %dx 
	outl	%eax, %dx 
	movw $0xcfc, %dx
	inl %dx, %eax 
	orl $0x88000000, %eax 
	outl	%eax, %dx 
	
	mov $0x8000006c, %eax 
	movw $0xcf8, %dx
	outl	%eax, %dx 
	movw $0xcfc, %dx 
	inl %dx, %eax
	push %eax 
	andl $0xfffffffe, %eax 
	outl	%eax, %dx 
	pop %eax
	outl	%eax, %dx 

	mov $0x80000080, %eax 
	movw $0xcf8, %dx 
	outl	%eax, %dx 
	movw $0xcfc, %dx 
	movl $0x100, %eax 
	outl	%eax, %dx 

	mov $0x8000088C, %eax 
	movw $0xcf8, %dx 
	outl	%eax, %dx 
	movw $0xcfc, %dx 
	movl $0x40000000, %eax 
	outl	%eax, %dx

	pop %eax
	pop %edx 
	movl $0x0, 0x0f680600
       
       
#ifdef XBE
	// We jump out of the rest of init, 
	// as we expect, this is all done when we are starting from the .xbe file
	// this Jump can be found in BootResetAction.c
	jmp BootResetAction
#endif
       
       


//	mov	$0, %al
//	mov	$0x72, %dx
//	out	%al, %dx

	mov	$0x8, %al
	mov	$0x61, %dx
	out	%al, %dx

		// xcode actions first of all

	mov $0x80000810, %eax ;	movw $0xcf8, %dx ; outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x8001, %eax ;	outl	%eax, %dx  // v1.0 ACPI IO region enable
	mov $0x80000884, %eax ;	movw $0xcf8, %dx ; outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x8001, %eax ;	outl	%eax, %dx  // v1.1  "
	movw $0x8026, %dx ;	movw $0x2201, %ax ;	outb %al, %dx // extsmi# able to control power (b0<-0 causes CPU powerdown after couple of seconds)
#if 1
	mov $0x80000804, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x3, %eax ;	outl	%eax, %dx

	movw $0x80d6, %dx ;	movb $4, %al ;	outb %al, %dx
	movw $0x80d8, %dx ;	movb $4, %al ;	outb %al, %dx
	movw $0x8049, %dx ;	movb $8, %al ;	outb %al, %dx
//	movw $0x80d9, %dx ;	movb $0, %al ;	outb %al, %dx

//	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0, %eax ;	outl	%eax, %dx
//	mov $0x80000340, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf81c4400, %eax ;	outl	%eax, %dx

	mov $0x8000f04c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000001, %eax ;	outl	%eax, %dx
	mov $0x8000f018, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00010100, %eax ;	outl	%eax, %dx
	// What is this ?
//	mov $0x80000084, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x07ffffff, %eax ;	outl	%eax, %dx
	
	mov $0x8000f020, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0ff00f00, %eax ;	outl	%eax, %dx
	mov $0x8000f024, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf7f0f000, %eax ;	outl	%eax, %dx
	mov $0x80010010, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0f000000, %eax ;	outl	%eax, %dx
	mov $0x80010014, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf0000000, %eax ;	outl	%eax, %dx
	mov $0x80010004, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000007, %eax ;	outl	%eax, %dx
	mov $0x8000f004, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000007, %eax ;	outl	%eax, %dx

	movl $0x07633461, 0x0f0010b0
	movl $0x66660000, 0x0f0010cc

	movl $0x03c00000, 0x0f600800  // new guy, move the video out of the way
//	movl $0x0, 0x0f680600  // stopped blue horrors previously

	cld             // clear direction flag

	movl 0x0f101000, %eax
	testl $0x000c0000, %eax
	jnz nota1a

	andl	$0xe1f3ffff, %eax
	orl $0x80000000, %eax
	movl %eax, 0x0f101000
	movl	$0xeeee0000, 0x0f0010b8

//	movl $0x07633451, 0x0f0010b0
//	movl $0x0, 0x0f0010cc
//	movl $0xffff0000, 0x0f0010b8
//	movl $0x5, 0x0f0010d4

	jmp donea1a

nota1a:

	andl	$0xe1f3ffff, %eax
	orl $0x860c0000, %eax
	movl %eax, 0x0f101000
	movl	$0xffff0000, 0x0f0010b8

//	movl $0x07633461, 0x0f0010b0
//	movl $0x66660000, 0x0f0010cc
//	movl $0xffff0000, 0x0f0010b8
//	movl $0x9, 0x0f0010d4

donea1a:

	movl $0x0, 0x0f0010b4
	movl $0x5866, 0x0f0010bc
	movl $0x0351c858, 0x0f0010c4
	movl $0x30007d67, 0x0f0010c8
	movl $0x0, 0x0f0010d8
	movl $0xa0423635, 0x0f0010dc
	movl $0x0c6558c6, 0x0f0010e8

	movl $0x03070103, 0x0f100200

	movl $0x11000016, 0x0f100410
	movl $0x11000016, 0x0f100410
	movl $0x84848888, 0x0f100330

	movl $0xffffcfff, 0x0f10032c
	movl $0x00000001, 0x0f100328
	movl $0x000000df, 0x0f100338

	movb 0x0f000000, %al
	cmpb $0xa1, %al
	jnz nota1b

	mov $0x803d4401, %eax ; mov 0x0f101000, %eax

nota1b:

	mov $0x80000904, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000001, %eax ;	outl	%eax, %dx
	mov $0x80000914, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0000c001, %eax ;	outl	%eax, %dx
	mov $0x80000918, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0000c201, %eax ;	outl	%eax, %dx

	mov $0x8000093c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	inl %dx, %eax ; orl	$0x7, %eax ;	outl	%eax, %dx // I2C int=7

	movw $0xc200, %dx ;	movb $0x70, %al ;	outb %al, %dx
//	movw $0xc002, %dx ;	movb $0x10, %al ;	outb %al, %dx // enable I2C interrupt

		// skipped unnecessary conexant init

	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx
	movw $0xc004, %dx ;	movb $0x20, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x01, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0x00, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

spin1:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin1

	  // (skipped PIC test here)
	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx
	movw $0xc004, %dx ;	movb $0x21, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x01, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

spin1a:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin1a

	movw $0xc006, %dx
	inb %dx, %al
	cmp $0x50, %al
	jz skipPicDecidedInit

	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x01000000, %eax ;	outl	%eax, %dx

skipPicDecidedInit:

	movb 0x0f000000, %al
	cmpb $0xa1, %al
	jnz nota1c

	mov $0x10101010, %eax ; mov 0x0f001214, %eax

	jmp donea1c

nota1c:

	mov $0x12121212, %eax ; mov 0x0f001214, %eax

donea1c:

	movl $0xaaaaaaaa, 0x0f00122c
	movl $0xaaaaaaaa, 0x0f001230
	movl $0xaaaaaaaa, 0x0f001234
	movl $0xaaaaaaaa, 0x0f001238
	movl $0x8b8b8b8b, 0x0f00123c
	movl $0x8b8b8b8b, 0x0f001240
	movl $0x8b8b8b8b, 0x0f001244
	movl $0x8b8b8b8b, 0x0f001248
	movl $0x00000001, 0x0f1002d4
	movl $0x00100042, 0x0f1002c4
	movl $0x00100042, 0x0f1002cc
	movl $0x11, 0x0f1002c0
	movl $0x11, 0x0f1002c8
	movl $0x32, 0x0f1002c0
	movl $0x32, 0x0f1002c8
	movl $0x132, 0x0f1002c0
	movl $0x132, 0x0f1002c8
	movl $0x1, 0x0f1002d0
	movl $0x1, 0x0f1002d0
	movl $0x80000000, 0x0f100210
	movl $0xaa8baa8b, 0x0f00124c
	movl $0x0000aa8b, 0x0f001250
	movl $0x081205ff, 0x0f100228
	movl $0x00010000, 0x0f000218

	mov $0x80000860, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	in %dx, %eax ; orl $0x400, %eax ;	outl	%eax, %dx
	mov $0x8000084c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xfdde, %eax ;	outl	%eax, %dx
	mov $0x8000089c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x871cc707, %eax ;	outl	%eax, %dx

	mov $0x800008b4, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	in %dx, %eax ; orl $0xf00, %eax ;	outl	%eax, %dx

	mov $0x80000340, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xf0f0c0c0, %eax ;	outl	%eax, %dx
	mov $0x80000344, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00c00000, %eax ;	outl	%eax, %dx
	mov $0x8000035c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x04070000, %eax ;	outl	%eax, %dx
	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00230801, %eax ;	outl	%eax, %dx
	mov $0x8000036c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x01230801, %eax ;	outl	%eax, %dx

	mov $8, %eax ; timloop2: dec %eax ; cmp $0, %eax ; jnz timloop2

		// 5F1

	movl $0x03070103, 0x0f100200
	movl $0x11448000, 0x0f100204

			// skipped actual memory test

		// A95
	movl $0x03070003, 0x0f100200

		// A9E
	mov $0x80000084, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x03ffffff, %eax ;	outl	%eax, %dx

	movw $0xc006, %dx ;	movb $0x0f, %al ;	outb %al, %dx // result1 AB9

	movw $0xc004, %dx ;	movb $0x20, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x13, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

		// B2E
spin2:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin2

	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0xf0, %al ;	outb %al, %dx
	movw $0xc004, %dx ;	movb $0x20, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x12, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0xf0, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

		// B76
spin3:
	movw $0xc000, %dx
	inb %dx, %al
	cmp $0x10, %al
	jnz	spin3

	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx

	mov $0x8000f020, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xfdf0fd00, %eax ;	outl	%eax, %dx
	mov $0x80010010, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0xfd000000, %eax ;	outl	%eax, %dx

#endif

#ifndef XBE
// copy everything into RAM
            mov $_ram_location, %edi
            mov $_start_ramcopy, %esi
	    //add $0xfffc0000, %esi
            mov $_size_ramcopy, %ecx
	    rep movsb
#endif
        jmp BootResetAction  // this can be found in BootResetAction.c





// Main Hardware init routines


entrypoint_start:

	xor	%eax, %eax
	xor	%edx, %edx
	xor	%ecx, %ecx
	
	// kill the cache
       	mov %cr0, %eax
	orl	$0x60000000, %eax
	mov	%eax, %cr0
	wbinvd

	mov	%cr3, %eax
	mov	%eax, %cr3

	// Clear Mmeory Type register
	movl	$0x2ff, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr

	// Init the MTRR for Ram

	

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
	movl	$0x800, %eax //Enable MTRRs
	wrmsr

	/* turn on normal cache */

	movl	%cr0, %eax
	mov 	%eax, %ebx
	andl	$0x9FFFFFFF,%eax
	movl	%eax, %cr0


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

	mov $0xfe0000fb, %eax // not cacheable 4M area starting 0xfe000000
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


	mov	%cr0, %eax
	mov	%eax, %ebx
	and $0xdfffffff, %eax
	or	$0x40000000, %eax
	mov	%eax, %cr0
	wbinvd

	mov	$0x277, %ecx
	mov	$0x70106, %eax // was 70106
	mov	%eax, %edx
	wrmsr
//	invd

	mov	%ebx, %cr0

	mov	%cr4, %eax
	or	$0x610, %eax
	mov	%eax, %cr4

	mov	$0xf000, %eax
	mov	%eax, %cr3

	mov	%cr0, %eax
	or	$0x80010020, %eax
	mov	%eax, %cr0

	jmp selfPipeKill
selfPipeKill:

	mov $0x00040000, %esp

#endif
/*
  cld             // clear direction flag

	movl $0, %edi
	xor	%eax, %eax
 	movl	$0x100000, %ecx
	rep
  stosb

//	mov $0x00040000, %esp

        // copy initiliazed data to ram

        leal    _start_load_data, %esi
        leal    _start_data, %edi
        movl    $_end_load_data, %ecx
        subl    %esi, %ecx
        jz      .nodata
        rep
        movsb
.nodata:

    // clear bss
        leal    _bss, %edi
        movl    $_ebss, %ecx
        subl    %edi, %ecx
        jz      .nobss
        xorl    %eax, %eax
        rep
        stosb
.nobss:
*/

	jmp entrypoint_end















.section .text, "ax"

///////////////////////////////////////////
//
//  Interrupt Service Routines
//

		// int 0

		.global	IntHandlerTimer0
IntHandlerTimer0:
	pusha
	pushf
	cli
	call	IntHandlerCTimer0
	mov	$0x20, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// int 1

		.global	IntHandler1
IntHandler1:
	pusha
	pushf
	cli
	call	IntHandler1C
	mov	$0x21, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// int 2

		.global	IntHandler2
IntHandler2:
	pusha
	pushf
	cli
	call	IntHandler2C
	mov	$0x22, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// int 3

		.global	IntHandler3
IntHandler3:
	pusha
	pushf
	cli
	call	IntHandler3VsyncC
	mov	$0x23, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// int 4

		.global	IntHandler4
IntHandler4:
	pusha
	pushf
	cli
	call	IntHandler4C
	mov	$0x24, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// int 5

		.global	IntHandler5
IntHandler5:
	pusha
	pushf
	cli
	call	IntHandler5C
	mov	$0x25, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// int 6

		.global	IntHandler6
IntHandler6:
	pusha
	pushf
	cli
	call	IntHandler6C
	mov	$0x26, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// int 7

		.global	IntHandler7
IntHandler7:
	pusha
	pushf
	cli
	call	IntHandler7C
	mov	$0x27, %al
	outb	%al, $0x20
	popf
	sti
	popa
	iret

		// Int 8 interrupts

	.global IntHandler8
IntHandler8:
	pusha
	pushf
	cli
	call	IntHandler8C
		// EOI on master and slave needed
	mov	$0x60, %al
	out	%al, $0xa0
	mov	$0x62, %al
	out	%al, $0x20
	popf
	popa
	iret


		// Int 9 interrupts

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
	pusha
	pushf
	cli
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
	pusha
	pushf
	cli
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
	pusha
	pushf
	cli
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
	pusha
	pushf
	cli
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
	pusha
	pushf
	cli
	call	IntHandlerUnusedC
	mov	$0x20, %al
	out	%al, $0x20
	popf
	popa
	iret

		// unused interrupts on slave PIC

	.global IntHandlerUnusedPic2
IntHandlerUnusedPic2:
	pusha
	pushf
	cli
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

	.section .justbehind, "ax"  // v1.1: 128 byte space here, FFFFFC80
		.code32

		 //must be at 0xfffffc80

		mov $0xc004, %dx
		mov $0x20, %al
		out %al, %dx
		mov $0xc008, %dx
		mov $0x8, %al
		out %al, %dx
		mov $0xc006, %dx
		mov $0xa6, %al
		out %al, %dx
		mov $0xc006, %dx
		in %dx,%al
		mov $0xc002, %dx
		mov $0x1a, %al
		out %al, %dx
		mov $0xc000, %dx

		ledspin: in %dx, %al ; cmp $0x10, %al ; jnz ledspin

		mov $0xc004, %dx
		mov $0x20, %al
		out %al, %dx
		mov $0xc008, %dx
		mov $0x7, %al
		out %al, %dx
		mov $0xc006, %dx
		mov $0x1, %al
		out %al, %dx
		mov $0xc006, %dx
		in %dx,%al
		mov $0xc002, %dx
		mov $0x1a, %al
		out %al, %dx
		mov $0xc000, %dx

		ledspin1: in %dx, %al ; cmp $0x10, %al ; jnz ledspin1

		jmp ledspin1

	mov $0x18, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss

	mov $0x200000, %esp
//		jmp	hello


/************************************************************
	the linker locates this file at the top
	of the rom and fills the space with nops.
*/
////////////////////////////////////////////
//  High ROM

		.section .high_rom, "ax"
		.code32
		.global sizeof_top
		.equ start_top, .


//		jmp start_linux
		ljmp $8, $start_linux				// nop's fall through to here which
								// jumps to our code earlier in this file at start_linux

	nop

gdt_table:
		.quad 0			// beginning of gdt
		.quad 0x0CF9B000000FFFF	// segment 8 offset
								// code segment execute/read, base 0 limit 4GB
		.quad 0x0CF93000000FFFF	// segment 10h offset
								//data segment r/w base	0 limit	4GB
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
		nop
		nop
		nop
		nop
gdt_ptr:
		.word 0x18
		.long 0x0FFFFFFD8		//pointer to gdt table
		nop
		nop
		nop
		nop
		nop
		nop

	.equ end_top, .
/*
	this is used by the linker to put this at the top of
	the rom.
*/

.equ sizeof_top, (. - start_top)


