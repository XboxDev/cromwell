/*
 *
 *  BIOS ROM Startup Assembler
 *  (C)2002 Andy, Michael, Paul, Steve
 * Original top and bottom ROM code by Steve from an idea by Michael
 * -- NOTE: Comment removed, the top / bottom Code changed to turnaround code.
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
	Rewritten from Original .bin linking to compiler system by Lehner Franz (franz@caos.at)
	Rewritten to Dual Boot concept for 2BL loading
	New written CPU Inits by Lehner Franz (franz@caos.at)
	Written New Working Xcodes + Xcode compiler by Lehner Franz (franz@caos.at)
	Focus support by Lehner Franz (franz@caos.at)
	Xcalibur support by Lehner Franz (franz@caos.at)
*/

#include "consts.h"

.code32

.section .low_rom, "ax"

.org 0x0
	/* MCPX Magic Values - clock timings*/

	.long 0xff000009
	.long 0xff000008
	.long 0x2b16d065
	.long 0x3346322d
	.long 0x01010101
	.long 0x08080808
	.long 0x00000801

	#ifndef MCPXREVD5
	.long 0xc8fc7c8a	// MCPX =< D5
	.long 0x44290213
	.long 0x90004998
	.long 0x00000000
	#else
	.long 0xc8b4588a	// MCPX 1.6 > D5
	.long 0x00100000
	.long 0x00050aa7
	.long 0xf0000000
	#endif

	.long 0xffffffff
	.long 0xffffffff

	.org 0x40		// imagebld temporary values
	.long _start_checksum
	.long _reset_vector_base

	.org 0x6c
	.long 0x00000107

	.org 0x70		// MCPX Config Area
	.long 0x0000000f
	.long 0x40004400

	#ifndef MCPXREVD5
	.long 0x12d10070
	.long 0x00000c90
	#else
	.long 0x16ce0090
	.long 0x00000dc0
	#endif

	//The bytecode interpreter begins here
	.org 0x80
#include "Xcodes.h"

	// Note: never change this from offset 0x1000 ....
	// This is the Main Entry point ....
        .org 0x1000

       	// Good Morning CPU

	// NOTE:
	/*
		We come here form the high rom section Jump
	*/

        // Clear Intel Interrupts in Processor Register
	// Everytime very good, specially when comming out of a running envoronment

	movl	$0x1b, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr

	// Interrupts now Dead

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

	// Invalidate TLB
	xor	%eax, %eax
	mov	%eax, %cr3

	// Clear Memory Type register
	movl	$0x2ff, %ecx
	xor	%eax, %eax
	xor	%edx, %edx
	wrmsr

/*
  We are setting the Ram Init's now to set up the Regions in the Ram

*/

	// MTRR for RAM
	// from address 0, Writeback Caching, 128MB range
        movl	$0x200, %ecx
	movl	$0x00000000, %edx
	movl	$0x00000006, %eax 	// == WB_CACHE == 6
	//movl	$0x00000004, %eax 	// Temporary, as USB development
	wrmsr

	// MASK0 set to 0xf8000[000] == 128M
        movl	$0x201, %ecx
	movl	$0x0000000f, %edx
	movl	$0xf8000800, %eax
	wrmsr

	// MTRR for shadow RAM
	// from address 0xf0000000, Write-combining Caching, 128MB range
        movl	$0x202, %ecx
	movl	$0x00000000, %edx
	movl	$0xf0000001, %eax 	// Write-Combining == 1
	wrmsr

	// MASK0 set to 0xf8000[000] == 128M
        movl	$0x203, %ecx
	movl	$0x0000000f, %edx
	movl	$0xf8000800, %eax
	wrmsr

     // MTRR for FLASH
	movl	$0x204, %ecx
	movl	$0x00000000, %edx
	movl	$0xFff00000, %eax 	    // We set to Uncacheable
	wrmsr

        movl	$0x205, %ecx
	movl	$0x0000000f, %edx
	movl	$0xfff00800, %eax
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
	movl	$0x800, %eax //Enable MTRRs
	wrmsr

	/* turn on normal cache */
	//  bit 30 + 29 = CD + NW
	// CD = Cache Disable (disable = 1)
	// NW Not write through (disable = 1)
	movl	%cr0, %eax
	mov 	%eax, %ebx
	andl	$0x9FFFFFFF,%eax
	movl	%eax, %cr0

        cld

	// Set the stack pointer to give us a valid stack
	movl $0x1ffff0, %esp

	// Perform SMC challenge response
	call smbus_smc_challenge_response

	// Copy 2BL to RAM
	mov 	$_ram_location, %edi
	mov 	$_start_ramcopy, %esi
	mov 	$(_size_ramcopy + 100), %ecx
	shr	$2,%ecx    // We dividy / 4, as we copy Dword oriented
	rep 	movsl

        jmp   initaftermemcpy


/*
  We leave Flash at this point, we never come back to it anymore.
  the following ASM instructions below, already are linked to memory (look to the ldscript in this case)
*/

.section .text, "ax"

       // Note: We are in Memory here, not in Flash anylonger,
       // we have been copy'd here and linked for memory, as we reside in section .text
       // Look to LDscript

.global	MemoryChecksum
MemoryChecksum:
	// The SHA-1 Hashsum is dumped here after with ImageBLD
      	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00
	.long _size_sha1hash
	.long 0x00  // Will be dumped in by imagebld ->compressed_image_start;
	.long 0x00  // Will be dumped in by imagebld ->compressed_image_size;
	.long 0x00  // Will be dumped in by imagebld ->0 .. 256kb image, 1 .. 1MB image

	.org 0x40  // We have linkbase for this programm = 0x100000 + 0x40 = divisable /4 .. so CPU likes it
tableGdt:

	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x00 dummy
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x08 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x10 code32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00 // 0x18 data32
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0x8f, 0x00 // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
	.byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0x8f, 0x00 // 0x28 data16
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Dummy

tableGdtDescriptor:

	.word 0x30
	.long tableGdt
	.word 0     // fill word for alignment

tableIdtDescriptor:

	.word 2048
	.long 0x400000
	.word 0     // fill word for alignment

initaftermemcpy:
/*
  These Init Codes Can be found in the 2'nd Bootloader

*/
        // We set up a GDT now, this is not necessary needed,
        // but it is preferred, as we have more ability with commands

        // We clear the IDT in RAM
        xor %eax,%eax
	mov $0x5000,%ecx
	mov $0x400000,%edi
	rep stosb

        lidt tableIdtDescriptor
	lgdt tableGdtDescriptor
	// set up selectors for everything


	xor	%eax, %eax
	lldt %ax

	// Reload CS as 0010 from the new GDT using a far jump
	.byte 0xEA          // jmp far 0010:reload_cs
	.long reload_cs
	.word 0x0010
.align 16
reload_cs:

	// CS is now a valid entry in the GDT.  Set SS, DS, and ES to valid
	// descriptors, but clear FS and GS as they are not necessary.

	// Set SS, DS, and ES to a data32 segment with maximum limit.
	movw $0x0018, %ax
	mov %eax, %ss
	mov %eax, %ds
	mov %eax, %es

	// Clear FS and GS
	xor %eax, %eax
	mov %eax, %fs
	mov %eax, %gs

	// Clear out .bss
	xor %eax, %eax
	mov $BSS_SIZE_L, %ecx
	mov $BSS_BASE, %edi
	rep stosl

	mov	$0x8, %al
	mov	$0x61, %dx
	out	%al, %dx

	// Enable IDE and NIC
	mov 	$0x8000088C, %eax
	movw 	$0xcf8, %dx
	outl	%eax, %dx
	movw 	$0xcfc, %dx
	movl 	$0x40000000, %eax
	outl	%eax, %dx

	// CPU Whoami   ? sesless ?
	mov 	$0x80000080, %eax
	movw 	$0xcf8, %dx
	outl	%eax, %dx
	movw 	$0xcfc, %dx
	movl 	$0x100, %eax
	outl	%eax, %dx

	// this can be found in 2bBootStartBios.c
	jmp 	BootStartBiosLoader


/* Initial 16-bit x86 Reset Vector Block */
.section .reset_vector_base, "ax"

/*
 * This is where the nForce specific table resides & is needed
 * on nForce boards to start execution at the reset vector.
 * Called "nVMB" in vendor BIOS. Structure is not known.
 */

    .byte 0x65, 0xd0, 0x16, 0x2b, 0x3c, 0x34, 0x33, 0x33, 0x03, 0x03, 0x03, 0x03, 0x0e, 0x1c, 0x1c, 0x1c
    .byte 0x03, 0x1c, 0xe0, 0x00, 0x8a, 0x70, 0xe4, 0xf8, 0x31, 0xd3, 0x67, 0x65, 0xd0, 0x0d, 0xe3, 0x00
    .byte 0xc0, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0x03, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0xd9, 0x40, 0x24, 0x00, 0x00, 0xdb, 0x17, 0x1d, 0xd9, 0x40, 0x25, 0x00, 0x00, 0xdb, 0x17, 0x1d
    .byte 0xd9, 0x40, 0x25, 0x00, 0x00, 0xdb, 0x17, 0x1d, 0xd9, 0x40, 0x26, 0x00, 0x00, 0xdb, 0x17, 0x1d
    .byte 0xd9, 0x40, 0x23, 0x00, 0x00, 0xe4, 0x27, 0x25, 0xd9, 0x40, 0x24, 0x00, 0x00, 0xe4, 0x27, 0x25
    .byte 0xd9, 0x40, 0x25, 0x00, 0x00, 0xe4, 0x27, 0x25, 0xd9, 0x40, 0x26, 0x00, 0x00, 0xe4, 0x27, 0x25
    .byte 0xd9, 0x40, 0x20, 0x00, 0x00, 0xe4, 0x17, 0x25, 0xd9, 0x40, 0x21, 0x00, 0x00, 0xe4, 0x17, 0x25
    .byte 0xd9, 0x40, 0x21, 0x00, 0x00, 0xe4, 0x17, 0x25, 0xd9, 0x40, 0x22, 0x00, 0x00, 0xe4, 0x17, 0x25
    .byte 0xd9, 0x40, 0x22, 0x00, 0x00, 0xdb, 0x17, 0x1d, 0xd9, 0x40, 0x23, 0x00, 0x00, 0xdb, 0x17, 0x1d
    .byte 0xd9, 0x40, 0x23, 0x00, 0x00, 0xdb, 0x17, 0x1d, 0xd9, 0x40, 0x24, 0x00, 0x00, 0xdb, 0x17, 0x1d

.section .reset_vector.start, "ax"

.align 16

gdt:

    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 0x00 dummy
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x08 code32
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0xcf, 0x00 // 0x10 code32
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00 // 0x18 data32
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9b, 0x8f, 0x00 // 0x20 code16 (8f indicates 4K granularity, ie, huge limit)
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0x8f, 0x00 // 0x28 data16

gdt_end:

.align 16

gdt_desc:

    .word gdt_end - gdt
    .long gdt
    .word 0              // fill word for alignment

idt_desc:

    .word 0
    .long 0
    .word 0              // fill word for alignment

.align 16

start_32:

    xor   %eax, %eax
    mov   $0x18, %al     // Set SS, DS, and ES to a data32 segment with maximum limit.
    mov   %eax, %ds
    mov   %eax, %es
    mov   %eax, %ss
    xor   %eax, %eax     // Clear FS and GS
    mov   %eax, %fs
    mov   %eax, %gs

    // Use function attribute "naked" if jumping to C,
    // as RAM is not yet available to set a valid stack.

    // this can be found in 2bBootStartBios.c
    jmp   BootSystemInitialization

.align 16

.code16

start_16:

    cli                  // Disable interrupts
    in    $0xee, %al     // Enable A20 (pull A20M# high via OpenXenium QPI, if connected)
    xor   %eax, %eax
    mov   %eax, %cr3     // Invalidate TLB
    lgdtl %cs:gdt_desc   // Load GDT
    lidtl %cs:idt_desc   // Load null IDT
    mov   %cr0, %eax     // Enable "Protected Mode"
    or    $0x1, %al      //  - Bit 0: Protected Mode Enable
    mov   %eax, %cr0
    ljmpl $0x10, $start_32

.section .reset_vector, "ax"

    jmp   start_16

