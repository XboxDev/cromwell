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
    .org 0x00

    .long mcpx_magic + 1  // b0 of this has to be set, or we are forced to boot on LPC

mcpx_magic:  // needs to be DWORD aligned
    .long 0x2b16d065, 0x3346322d
    .byte 1, 1, 1, 1, 8, 8

/* Xcode */
    .org 0x80
    .byte 0x04; .long 0x88000880, 0x00000002 /* POKEPCI 0:1:0/80, 00000002h */

		// this X-Code instruction has the effect of paging out the MCPX ROM
		// that makes us fall out of the ROM near the end of our ROM image at symbol jmp_code

		.org 0x100
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
	.globl start_linux
start_linux:

		// boot_post_macro(0x01)
		movl	$MTRR_DEF, %ecx
		xor		%eax, %eax
		xor		%edx, %edx
		wrmsr				// disables all MTRRs and sets all mem to UC

		movl	$MTRR_PHYSBASE, %ecx

		/* sets up ram area */
		movl	$BASE0_H, %edx
		movl	$BASE0_L, %eax
		wrmsr
		inc		%ecx		// this is the MASK0
		movl	$MASK0_H, %edx
		movl	$MASK0_L, %eax
		wrmsr

		/* sets up rom area */
		inc		%ecx		// this is the BASE1
		movl	$BASE1_H, %edx
		movl	$BASE1_L, %eax
		wrmsr
		inc		%ecx		// this is the MASK1
		movl	$MASK1_H, %edx
		movl	$MASK1_L, %eax
		wrmsr

		inc		%ecx

		/* we disable the rest of the mtrrs */
		xor		%eax, %eax
		xor		%edx, %edx
.mtrr_loop:
		wrmsr
		inc		%ecx
		cmpl	$MTRR_LAST,%ecx
		jbe 	.mtrr_loop

		/* enable mtrrs */
		movl	$MTRR_DEF,%ecx
		movl	$MTRR_DEF_TYPE,%eax
		wrmsr

		// boot_post_macro(0x02)

		/* turn on normal cache */
		movl	%cr0,%eax
		andl	$0x9FFFFFFF,%eax
		movl	%eax,%cr0

		// boot_post_macro(0x03)

		/* setup the segments and the stack */
		movw    $0x10, %ax
		movw    %ax, %ds
		movw    %ax, %es
		movw    %ax, %ss
		movl	$RAM_BASE,%esp
		xor		%eax, %eax
		movw    %ax, %fs
		movw    %ax, %gs

        // copy initiliazed data to ram

        cld             // clear direction flag
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




/*	   Emergency I2C spamming code in case of no boot
retry:

	movb $0x55, %al
	movl $0xc004, %edx
	shl	$1, %al
	out %al, %dx

	movb $0xaa, %al
	add $4, %edx
	out %al, %dx

	movb $0xbb, %al
	sub $0x2, %edx
	out %al, %dx

	sub $6, %edx
	in %dx, %ax
	out %ax, %dx
	add $2, %edx
	movb $0xa, %al
	out %al, %dx
	sub $0x2, %dx
spin:
	in %dx, %al
	test $8, %al
	jnz spin
	test $8, %al
	jnz retry
	test $24, %al

	jmp retry

*/



/* we are flying now.  Jump into the C which takes over from here */

		jmp BootResetAction  // this can be found in BootResetAction.c

				.align 4,0x90,3
idt_table:
	.word	0				# idt limit = 0
	.word	0, 0				# idt base = 0L

		.global	IntHandlerTimer0

IntHandlerTimer0:
//	incl 0x46c // this is the old PC BIOS tick count position
	push	%eax
	push	%edx
	
	call	IntHandlerCTimer0

	mov	$0x20, %al
	mov	$0x20, %dx
	out	%al, %dx
	pop	%edx
	pop	%eax
	iret

/************************************************************
	the linker locates this file at the top
	of the rom and fills the space with nops.
*/
		.section .high_rom, "ax"
		.code32
		.global sizeof_top
		.equ start_top, .

// ---------------------------------------------------------------------

jmp_code:	ljmp $0x08, $start_linux				// nop's fall through to here which
								// jumps to our code earlier in this file at start_linux
// ---------------------------------------------------------------------

/*
	alignment is needed to put the gdt table
	at the right spot; fill with nops; don't skip more
	than 3, ie, don't jump 4 to align.
*/

		.align 4,0x90,3

		.code16
gdt_table:	.quad 0			// beginning of gdt
		.quad 0x0CF9B000000FFFF	// segment 0x08 offset
								// code segment execute/read, base 0 limit 4GB
		.quad 0x0CF93000000FFFF	// segment 18h offset
								//data segment r/w base	0 limit	4GB

// ---------------------------------------------------------------------
		jmp	jmp_code
// ---------------------------------------------------------------------
		nop
		nop
gdt_ptr:
		.word 0x18
		.long gdt_table		//pointer to gdt table
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
.equ sizeof_top, (end_top - start_top)


