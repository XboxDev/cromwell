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

		jmp MachineInit	

		.org 0x6c
		.byte 0x07, 0x01
		.org 0x70

		.byte 0x0f, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x40
		.byte 0x70, 0x00, 0xd1, 0x12, 0x90, 0x0c, 0x00, 0x00

//	.section .text, "ax"
	.text


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

.globl tableGdtDescriptor
tableGdtDescriptor:

	.word 0x30
	.long tableGdt
	.word 0

.globl tableIdtDescriptor
tableIdtDescriptor:

	.word 2048
	.long 0xef000
	.word 0


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


//		jmp MachineInit
		ljmp $8, $MachineInit				// nop's fall through to here which
								// jumps to our code

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


