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
#include "2bconsts.h"
 
#define xcode_peek(val1) .byte 0x2; .long val1 ; .long 0x0 ;
#define xcode_poke(val1,val2) .byte 0x3; .long val1 ; .long val2 ;
#define xcode_pciout(val1,val2) .byte 0x4; .long val1 ; .long val2 ;
#define xcode_pciin_a(val1) .byte 0x5; .long val1 ; .long 0x0 ;

#define xcode_bittoggle(val1,val2) .byte 0x6; .long val1 ; .long val2 ;

#define xcode_ifgoto(val1,val2) .byte 0x8; .long val1 ; .long (9*(val2-1)) ;

#define xcode_outb(val1,val2) .byte 0x11; .long val1 ; .long val2 ;			    
#define xcode_inb(val1) .byte 0x12; .long val1 ; .long 0x0 ;			    

#define xcode_poke_a(val1) .byte 0x7; .long 0x3; .long val1 ;  
#define xcode_pciout_a(val1) .byte 0x7; .long 0x4; .long val1 ;  
#define xcode_outb_a(val1) .byte 0x7; .long 0x11; .long val1 ;  

#define xcode_goto(val1) .byte 0x9; .long 0x0; .long (9*(val1-1));

#define xcode_END(val1) .byte 0xEE; .long val1 ; .long 0x0;

.code32

.section .low_rom, "ax"

.org 0x0
	/* MCPX Magic Values */
	
	.long 0xff000009
	.long 0xff000008
	.long 0x2b16d065
	.long 0x3346322d
	.long 0x01010101
	.long 0x08080808
	.long 0x00000801
	.long 0xc8fc7c8a
	
	.long 0x44290213
	.long 0x90004998		
	
	.long 0x0
	.long 0xffffffff
	.long 0xffffffff
	
	.org 0x40
	.long _start_checksum	// This number will be overwritten  
				// With imagebld, but we need a temp value
	
	.org 0x6c
	.long 0x00000107
	
	.org 0x70		// MCPX Config Area
	.long 0x0000000f
	.long 0x40004400
	
	.long 0x12d10070
	.long 0x00000c90
		
	
	// Here, the Bytecode interpretor starts
	.org 0x80

	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000804, 0x00000003);
	xcode_outb(0x00008049, 0x00000008);
	xcode_outb(0x000080d9, 0x00000000);
	xcode_outb(0x00008026, 0x00000001);
	xcode_pciout(0x8000f04c, 0x00000001);
	xcode_pciout(0x8000f018, 0x00010100);
	xcode_pciout(0x80000084, 0x07ffffff);
	xcode_pciout(0x8000f020, 0x0ff00f00);
	xcode_pciout(0x8000f024, 0xf7f0f000);
	xcode_pciout(0x80010010, 0x0f000000);
	xcode_pciout(0x80010014, 0xf0000000);
	xcode_pciout(0x80010004, 0x00000007);
	xcode_pciout(0x8000f004, 0x00000007);



	xcode_poke(0x0f0010b0, 0x07633461);
	xcode_poke(0x0f0010cc, 0x66660000);
	

	xcode_peek(0x0f101000);
	
	xcode_bittoggle(0x000c0000,0x00000000);

	xcode_ifgoto(0x00000000,6);
	
	

	xcode_peek(0x0f101000);
	
	xcode_bittoggle(0xe1f3ffff,0x80000000);

	
	xcode_poke_a(0x0f101000);
	xcode_poke(0x0f0010b8, 0xeeee0000);
	xcode_goto(11);
	
	xcode_ifgoto(0x000c0000,6);

	xcode_peek(0x0f101000);
	xcode_bittoggle(0xe1f3ffff,0x860c0000);
	xcode_poke_a(0x0f101000);
	
	xcode_poke(0x0f0010b8, 0xffff0000);
        xcode_goto(5);
	xcode_peek(0x0f101000);
	


	xcode_bittoggle(0xe1f3ffff,0x820c0000);
	xcode_poke_a(0x0f101000);
	xcode_poke(0x0f0010b8, 0xffff0000);
	xcode_poke(0x0f0010d4, 0x00000009);
	xcode_poke(0x0f0010b4, 0x00000000);
	xcode_poke(0x0f0010bc, 0x00005866);
	xcode_poke(0x0f0010c4, 0x0351c858);
	xcode_poke(0x0f0010c8, 0x30007d67);
	xcode_poke(0x0f0010d8, 0x00000000);
	xcode_poke(0x0f0010dc, 0xa0423635);
	xcode_poke(0x0f0010e8, 0x0c6558c6);
	xcode_poke(0x0f100200, 0x03070103);
	xcode_poke(0x0f100410, 0x11000016);
	xcode_poke(0x0f100330, 0x84848888);
	xcode_poke(0x0f10032c, 0xffffcfff);
	xcode_poke(0x0f100328, 0x00000001);
	xcode_poke(0x0f100338, 0x000000df);
	xcode_pciout(0x80000904, 0x00000001);
	xcode_pciout(0x80000914, 0x0000c001);
	xcode_pciout(0x80000918, 0x0000c201);
	
	xcode_outb(0x0000c200, 0x00000070);
	xcode_outb(0x0000c004, 0x0000008a);
	xcode_outb(0x0000c008, 0x000000ba);
	xcode_outb(0x0000c006, 0x0000003f);
	xcode_outb(0x0000c002, 0x0000000a);
        
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1)

	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x0000006c);
	xcode_outb(0x0000c006, 0x00000046);
	xcode_outb(0x0000c002, 0x0000000a);
        
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1)
 
	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x000000b8);
	xcode_outb(0x0000c006, 0x00000000);
	xcode_outb(0x0000c002, 0x0000000a);
	xcode_inb(0x0000c000);
	xcode_ifgoto(0x00000010,-1);

	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x000000ce);
	xcode_outb(0x0000c006, 0x00000019);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);
        
	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x000000c6);
	xcode_outb(0x0000c006, 0x0000009c);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);
        
	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x00000032);
	xcode_outb(0x0000c006, 0x00000008);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);
        
	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x000000c4);
	xcode_outb(0x0000c006, 0x00000001);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);
        
	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c004, 0x00000020);
	xcode_outb(0x0000c008, 0x00000001);
	xcode_outb(0x0000c006, 0x00000000);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);
        
	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c004, 0x00000021);
	xcode_outb(0x0000c008, 0x00000001);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);
        
	xcode_outb(0x0000c000, 0x00000010);
	xcode_inb(0x0000c006);
        xcode_ifgoto(0x00000050,2);
        
        xcode_goto(2);

	xcode_pciout(0x8000036c, 0x01000000);
	xcode_poke(0x0f680500, 0x00011c01);
	xcode_poke(0x0f68050c, 0x000a0400);
	xcode_poke(0x0f001220, 0x00000000);
	xcode_poke(0x0f001228, 0x00000000);
	xcode_poke(0x0f001264, 0x00000000);
	xcode_poke(0x0f001210, 0x00000010);
	xcode_peek(0x0f101000);
	xcode_bittoggle(0x06000000,0x00000000);
	xcode_ifgoto(0x00000000,4);
	xcode_poke(0x0f001214, 0x48480848);
	xcode_poke(0x0f00122c, 0x88888888);
	xcode_goto(7);
	xcode_ifgoto(0x06000000,4);
	xcode_poke(0x0f001214, 0x09090909);
	xcode_poke(0x0f00122c, 0xaaaaaaaa);
	xcode_goto(3);
     

	xcode_poke(0x0f001214, 0x09090909);
	xcode_poke(0x0f00122c, 0xaaaaaaaa);
	xcode_poke(0x0f001230, 0xffffffff);
	xcode_poke(0x0f001234, 0xaaaaaaaa);
	xcode_poke(0x0f001238, 0xaaaaaaaa);
	xcode_poke(0x0f00123c, 0x8b8b8b8b);
	xcode_poke(0x0f001240, 0xffffffff);
	xcode_poke(0x0f001244, 0x8b8b8b8b);
	xcode_poke(0x0f001248, 0x8b8b8b8b);
	xcode_poke(0x0f1002d4, 0x00000001);
	xcode_poke(0x0f1002c4, 0x00100042);
	xcode_poke(0x0f1002cc, 0x00100042);
	xcode_poke(0x0f1002c0, 0x00000011);
	xcode_poke(0x0f1002c8, 0x00000011);
	xcode_poke(0x0f1002c0, 0x00000032);
	xcode_poke(0x0f1002c8, 0x00000032);
	xcode_poke(0x0f1002c0, 0x00000132);
	xcode_poke(0x0f1002c8, 0x00000132);
	xcode_poke(0x0f1002d0, 0x00000001);
	xcode_poke(0x0f1002d0, 0x00000001);
	xcode_poke(0x0f100210, 0x80000000);
	xcode_poke(0x0f00124c, 0xaa8baa8b);
	xcode_poke(0x0f001250, 0x0000aa8b);
	xcode_poke(0x0f100228, 0x081205ff);
	xcode_poke(0x0f001218, 0x00010000);
	
	xcode_pciin_a(0x80000860);
	xcode_bittoggle(0xffffffff,0x00000400);   
	

	xcode_pciout_a(0x80000860);
	

	xcode_pciout(0x8000084c, 0x0000fdde);
	xcode_pciout(0x8000089c, 0x871cc707);
	xcode_pciin_a(0x800008b4);
	xcode_bittoggle(0xfffff0ff,0x00000f00);
	xcode_pciout_a(0x800008b4);
	xcode_pciout(0x80000340, 0xf0f0c0c0);
	xcode_pciout(0x80000344, 0x00c00000);
	xcode_pciout(0x8000035c, 0x04070000);
	xcode_pciout(0x8000036c, 0x00230801);
	xcode_pciout(0x8000036c, 0x01230801);
	xcode_goto(1);
	xcode_goto(1);
	xcode_poke(0x0f100200, 0x03070103);
	xcode_poke(0x0f100204, 0x11448000);
	xcode_pciout(0x8000103c, 0x00000000);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);

	xcode_outb(0x0000c004, 0x0000008a);
	xcode_outb(0x0000c008, 0x000000ba);
	xcode_outb(0x0000c006, 0x0000003f);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);

	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x0000006c);
	xcode_outb(0x0000c006, 0x00000046);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);

	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c004, 0x0000008a);
	xcode_outb(0x0000c008, 0x000000ba);
	xcode_outb(0x0000c006, 0x0000003f);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);   
        xcode_ifgoto(0x00000010,-1);

	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x0000006c);
	xcode_outb(0x0000c006, 0x00000046);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);

	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c004, 0x0000008a);
	xcode_outb(0x0000c008, 0x000000ba);
	xcode_outb(0x0000c006, 0x0000003f);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);

	xcode_outb(0x0000c000, 0x00000010);
	xcode_outb(0x0000c008, 0x0000006c);
	xcode_outb(0x0000c006, 0x00000046);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);   

        

	xcode_outb(0x0000c000, 0x00000010);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_pciout(0x80000884, 0x00008001);
	xcode_outb(0x0000c000, 0x00000010);
	
	xcode_pciin_a(0x8000183c);
	xcode_bittoggle(0x000000ff,0x00000000);

	xcode_outb_a(0x0000c006);
	xcode_outb(0x0000c008, 0x00000012);
	xcode_outb(0x0000c002, 0x0000000a);
        xcode_inb(0x0000c000);
        xcode_ifgoto(0x00000010,-1);
	xcode_outb(0x0000c000, 0x00000010);
	xcode_pciout(0x8000f020, 0xfdf0fd00);
	xcode_pciout(0x80010010, 0xfd000000);
	xcode_poke(0x00000000, 0xfc1000ea);
	xcode_poke(0x00000004, 0x000008ff);

       	xcode_END(0x806);





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

	// We clear the cr3 register
	mov	%eax, %eax
	mov	%eax, %cr3

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
       //201
	// MASK0 set to 0xffc000[000] == 64M    
        inc %ecx  		// IA32_MTRR_PHYS_MASK 0
	movl	$0x0f, %edx 		// 0x0f
	movl	$0xfc000800, %eax  	// 0xfc000800 b11= enable MTRR
	wrmsr
	
	
	//202
	// MTRR for BIOS
	inc %ecx		// IA32_MTRR_PHYS Base 1 
	movl	$0x00000000, %edx 	// 0x00
	movl	$0xFFF00006, %eax 	// == WB cache  NOTE SIZING!  Only last 1M cached!
	wrmsr
        //203
        // MASK0 set to 0xff0000[000] == 16M
        inc %ecx             // IA32_MTRR_PHYS_MASK 1
	movl	$0x0000000f, %edx // 0x0f
	movl	$0xFFF00800, %eax  // 0xff000800
	wrmsr
        
        //204
        // MTRR for Video Memory (last 4MByte of shared Ram)
	// Writethrough type trumps Writeback for overlapping region
      	inc %ecx           	// IA32_MTRR_PHYS Base 2
	movl	$0x00000000, %edx 	// 0x00
	movl	$0x03C00004, %eax 	// == Writethrough == 4
	wrmsr
	//205
	// MASK0 set to 0xfffC00[000] == 4M
	inc %ecx            		// IA32_MTRR_PHYS_MASK 2
	movl	$0x0000000f, %edx // 0x0f
	movl	$0xFFC00800, %eax  // 0xffC00800
	wrmsr
        
        //206
        // PCI Config Space   
	inc %ecx          
	movl    $0x00000000, %edx       // 0x00   
	movl    $0xf0000000, %eax       
	wrmsr   
	//207
	inc %ecx           
	movl    $0x0000000f, %edx       //   
	movl    $0xF0100800, %eax       //   
	wrmsr 


        xor	%eax, %eax
	xor	%edx, %edx
	

	inc %ecx		// IA32_MTRR_PHYS Base 4
        wrmsr                                                   
	inc %ecx		// IA32_MTRR_PHYS_MASK 4
        wrmsr                                                   

	inc %ecx		// IA32_MTRR_PHYS Base 5
        wrmsr                                                   
	inc %ecx		// IA32_MTRR_PHYS_MASK 5
        wrmsr                                                   

	inc %ecx		// IA32_MTRR_PHYS Base 6
        wrmsr                                                   
	inc %ecx		// IA32_MTRR_PHYS_MASK 6
        wrmsr

	inc %ecx		// IA32_MTRR_PHYS Base 7
        wrmsr
	inc %ecx		// IA32_MTRR_PHYS_MASK 7
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

// copy everything into RAM + Verify the Read from the Flash

	mov 	$_ram_location, %edi
	mov 	$_start_ramcopy, %esi
	mov 	$_size_ramcopy, %ecx
loop_read:	
	lodsb
retry_read:
	dec   	%esi
	xchg  	%ebx,%eax
	lodsb
	cmp   	%al,%bl
	jne   	retry_read
	stosb
	dec   	%ecx
	jnz   	loop_read

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
	.long _size_ramcopy
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
	.word 0x0       // fill Word, so we get alliged again

tableIdtDescriptor:

	.word 2048
	.long 0x400000
	.word 0x0	// fill Word, so we get alliged again
		
initaftermemcpy:
/*
  These Init Codes Can be found in the 2'nd Bootloader

*/ 
        // We set up a GDT now, this is not necessary needed, 
        // but i perfeer a GDT, as we have more ability with commands
        
        // We clear the IDT in RAM
        xor %eax,%eax
	mov $0x5000,%ecx
	mov $0x400000,%edi
	rep stosb

        lidt tableIdtDescriptor
	lgdt tableGdtDescriptor
	// set up selectors for everything

	mov	$0x18, %eax
	.byte 0x8e, 0xd8  // from 2bl first init

	movl	$ 0x00080000,%esp
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss
  
  	xor	%eax, %eax
	movw    %ax, %fs
	movw    %ax, %gs
                
 
	mov	$0x8, %al
	mov	$0x61, %dx
	out	%al, %dx

	// FPU SETUP -- Not perfect, soembody should take a close look to it

	clts
	fninit
	

	mov $0x80000810, %eax ;	movw $0xcf8, %dx ; outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x8001, %eax ;	outl	%eax, %dx  // v1.0 ACPI IO region enable
	mov $0x80000884, %eax ;	movw $0xcf8, %dx ; outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x8001, %eax ;	outl	%eax, %dx  // v1.1  "
	movw $0x8026, %dx ;	movw $0x2201, %ax ;	outb %al, %dx // extsmi# able to control power (b0<-0 causes CPU powerdown after couple of seconds)

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

	cld             // clear direction flag

	movl 0x0f101000, %eax
	testl $0x000c0000, %eax
	jnz nota1a

	andl	$0xe1f3ffff, %eax
	orl $0x80000000, %eax
	movl %eax, 0x0f101000
	movl	$0xeeee0000, 0x0f0010b8

	jmp donea1a

nota1a:

	andl	$0xe1f3ffff, %eax
	orl 	$0x860c0000, %eax
	movl 	%eax, 0x0f101000
	movl	$0xffff0000, 0x0f0010b8

donea1a:

	movl 	$0x0, 0x0f0010b4
	movl 	$0x5866, 0x0f0010bc
	movl 	$0x0351c858, 0x0f0010c4
	movl 	$0x30007d67, 0x0f0010c8
	movl 	$0x0, 0x0f0010d8
	movl 	$0xa0423635, 0x0f0010dc
	movl 	$0x0c6558c6, 0x0f0010e8

	movl 	$0x03070103, 0x0f100200

	movl 	$0x11000016, 0x0f100410
	movl 	$0x11000016, 0x0f100410
	movl 	$0x84848888, 0x0f100330

	movl 	$0xffffcfff, 0x0f10032c
	movl 	$0x00000001, 0x0f100328
	movl 	$0x000000df, 0x0f100338

	movb 	0x0f000000, %al
	cmpb 	$0xa1, %al
	jnz 	nota1b

	mov 	$0x803d4401, %eax 
	mov 	0x0f101000, %eax

nota1b:

	mov $0x80000904, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x00000001, %eax ;	outl	%eax, %dx
	mov $0x80000914, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0000c001, %eax ;	outl	%eax, %dx
	mov $0x80000918, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	mov	$0x0000c201, %eax ;	outl	%eax, %dx

	mov $0x8000093c, %eax ;	movw $0xcf8, %dx ;	outl	%eax, %dx ;	movw $0xcfc, %dx ;	inl %dx, %eax ; orl	$0x7, %eax ;	outl	%eax, %dx // I2C int=7

	movw $0xc200, %dx ;	movb $0x70, %al ;	outb %al, %dx
//	movw $0xc002, %dx ;	movb $0x10, %al ;	outb %al, %dx // enable I2C interrupt

	// skipped unnecessary conexant init

	movb 	0x0f000000, %al
	cmpb 	$0xa1, %al
	jnz 	nota1c

	mov 	$0x10101010, %eax 
	mov 	0x0f001214, %eax

	jmp donea1c

nota1c:

	mov $0x12121212, %eax  
	mov 0x0f001214, %eax

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
	movw 	$0xc000, %dx
	inb 	%dx, %al
	cmp 	$0x10, %al
	jnz	spin2

	movw $0xc000, %dx ;	movb $0x10, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0xf0, %al ;	outb %al, %dx
	movw $0xc004, %dx ;	movb $0x20, %al ;	outb %al, %dx
	movw $0xc008, %dx ;	movb $0x12, %al ;	outb %al, %dx
	movw $0xc006, %dx ;	movb $0xf0, %al ;	outb %al, %dx
	movw $0xc002, %dx ;	movb $0x0a, %al ;	outb %al, %dx

		// B76
spin3:
	movw 	$0xc000, %dx
	inb 	%dx, %al
	cmp 	$0x10, %al
	jnz	spin3

	movw 	$0xc000, %dx 
	movb 	$0x10, %al 
	outb 	%al, %dx

	mov 	$0x8000f020, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	mov	$0xfdf0fd00, %eax 
	outl	%eax, %dx
	
	mov 	$0x80010010, %eax 
	movw 	$0xcf8, %dx 
	outl	%eax, %dx 
	movw 	$0xcfc, %dx 
	mov	$0xfd000000, %eax 
	outl	%eax, %dx

	jmp 	BootStartBiosLoader  // this can be found in BootResetAction.c
                         
                         
/*

We finishes here, the code below is for the upper section of the MCPX swapout

*/                         


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
                
                cld
		ljmp $8, $0xffFc1000		// nop's fall through to here which
						// jumps to our code earlier in this file at start_bootloader
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


