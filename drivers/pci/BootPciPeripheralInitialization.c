#include "boot.h"


// by ozpaulb@hotmail.com 2002-07-14

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
void WATCHDOG() {
	 __asm__ __volatile__ (
	 	" movw $0x80cf, %%dx ; mov $5, %%al ; out %%al, %%dx ; mov $10000, %%eax ; 0: dec %%eax ; cmp $0, %%eax ; jnz 0b ; mov $4, %%al ; out %%al, %%dx  "
		: : : "%eax", "%edx"
		);
}
*/

BYTE PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
		return IoInputByte(0xcfc + (reg_off & 3));
}

void PciWriteByte (unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg_off, unsigned char byteval)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
		IoOutputByte(0xcfc + (reg_off & 3), byteval);
}


WORD PciReadWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfe)));
		return IoInputWord(0xcfc + (reg_off & 1));
}


void PciWriteWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, WORD w)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
		IoOutputWord(0xcfc + (reg_off & 1), w);

}

DWORD PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
		return IoInputDword(0xcfc + (reg_off & 3));
}


void PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, DWORD dw)
{
	DWORD base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
		IoOutputDword(0xcfc + (reg_off & 3), dw);

}


void BootPciPeripheralInitialization()
{

	__asm__ __volatile__ ( "cli" );

		__asm__ __volatile__(
			" mov $0x1b, %%ecx ; rdmsr ; and $0xFFFFF7FF, %%eax ; wrmsr  \n"  // turn off APIC
				: : : "%ecx", "%edx", "%eax"
		);

	PciWriteDword(BUS_0, DEV_1, 0, 0x80, 2);  // v1.1 2BL kill ROM area
	if(PciReadByte(BUS_0, DEV_1, 0, 0x8)>=0xd1) { // check revision
		PciWriteDword(BUS_0, DEV_1, 0, 0xc8, 0x8f00);  // v1.1 2BL <-- death
	}

	PciWriteDword(BUS_0, DEV_0, 3, 0x58, 0x00008000);
	PciWriteDword(BUS_0, DEV_0, 3, 0x40, 0x0017cc00);


	IoOutputByte(0x2e, 0x55);
	IoOutputByte(0x2e, 0x26);

	IoOutputByte(0x61, 0xff);
	IoOutputByte(0x92, 0x01);
//	IoOutputByte(0xcf9, 0x08);

	IoOutputByte(0x70, 0x0a);
	IoOutputByte(0x71, 0x2d);

	IoOutputByte(0x70, 0x0b);
	IoOutputByte(0x71, (IoInputByte(0x71)&1)|2);

	IoOutputByte(0x70, 0x0c);
	IoInputByte(0x71);
	IoOutputByte(0x70, 0x0d);
	IoInputByte(0x71);


//#ifndef XBE
			// configure ACPI hardware to generate interrupt on PIC-chip pin6 action (via EXTSMI#)

	IoOutputByte(0x80ce, 0x08);  // from 2bl RI#
	IoOutputByte(0x80c0, 0x08);  // from 2bl SMBUSC
	IoOutputByte(0x8004, IoInputByte(0x8004)|1);  // KERN: SCI enable == SCI interrupt generated
	IoOutputWord(0x8022, IoInputByte(0x8022)|2);  // KERN: Interrupt enable register, b1 RESERVED in AMD docs
	IoOutputWord(0x8023, IoInputByte(0x8023)|2);  // KERN: Interrupt enable register, b1 RESERVED in AMD docs
	IoOutputByte(0x8002, IoInputByte(0x8002)|1);  // KERN: Enable SCI interrupt when timer status goes high
	IoOutputWord(0x8028, IoInputByte(0x8028)|1);  // KERN: setting readonly trap event???

//#endif
	/*
			// Setup paging tables

	{ int n=0; volatile DWORD * pdw=(volatile DWORD *)0xf800; DWORD dw=0xe3;
		for(n=0; n<0x40; n++) { *pdw++=dw; dw+=0x400000; }
		for(n=0;n<0x1c0;n++) { *pdw++=0; }

		pdw=(volatile DWORD *)0xfc00;
		*pdw=0xf063;
		pdw=(volatile DWORD *)0xfffc;
		*pdw=0x0FFC000E3;
		pdw=(volatile DWORD *)(0xf000+(0x0FD0000FB>>20));
		*pdw++=0x0FD0000FB;
		*pdw++=0x0FD4000FB;
		*pdw++=0x0FD8000FB;
		*pdw++=0x0FDC000FB;
	}
*/

// Bus 0, Device 1, Function 0 = nForce HUB Interface - ISA Bridge
//
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x6c, 0x0e065491);
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x6a, 0x0003); // kern ??? gets us an int3?  vsync req
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x64, 0x00000b0c);
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x81, PciReadByte(BUS_0, DEV_1, FUNC_0, 0x81)|8);


//
// Bus 0, Device 9, Function 0 = nForce ATA Controller
//

	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x20, 0x0000ff61);	// (BMIBA) Set Busmaster regs I/O base address 0xff60
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 4, PciReadDword(BUS_0, DEV_9, FUNC_0, 4)|5); // 0x00b00005 );
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 8, PciReadDword(BUS_0, DEV_9, FUNC_0, 8)&0xfffffeff); // 0x01018ab1 ); // was fffffaff

	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x58, 0x20202020); // kern1.1
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x60, 0x00000000); // kern1.1
//  PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x40, 0x000084bb); // new - removed by frankenregister comparison
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x50, 0x00000002);  // without this there is no register footprint at IO 1F0

	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x2c, 0x00000000); // frankenregister from xbe boot
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x40, 0x00000000); // frankenregister from xbe boot
		// below reinstated by frankenregister compare with xbe boot
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x60, 0xC0C0C0C0); // kern1.1 <--- this was in kern1.1 but is FATAL for good HDD access

//
// Bus 0, Device 4, Function 0 = nForce MCP Networking Adapter - all verified with kern1.1
//
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 4, PciReadDword(BUS_0, DEV_4, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x10, 0xfef00000); // memory base address 0xfef00000
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x14, 0x0000e001); // I/O base address 0xe000
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x118-0xdc, (PciReadDword(BUS_0, DEV_4, FUNC_0, 0x3c) &0xffff0000) | 0x0004 );

//
// Bus 0, Device 2, Function 0 = nForce OHCI USB Controller - all verified with kern 1.1
//

	PciWriteDword(BUS_0, DEV_2, FUNC_0, 4, PciReadDword(BUS_0, DEV_2, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x10, 0xfed00000);	// memory base address 0xfed00000
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_2, FUNC_0, 0x3c) &0xffff0000) | 0x0001 );
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x50, 0x0000000f);

//
// Bus 0, Device 3, Function 0 = nForce OHCI USB Controller - verified with kern1.1
//

	PciWriteDword(BUS_0, DEV_3, FUNC_0, 4, PciReadDword(BUS_0, DEV_3, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x10, 0xfed08000);	// memory base address 0xfed08000
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_3, FUNC_0, 0x3c) &0xffff0000) | 0x0009 );
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x50, 0x00000030);  // actually BYTE?

//
// Bus 0, Device 6, Function 0 = nForce Audio Codec Interface - verified with kern1.1
//

	PciWriteDword(BUS_0, DEV_6, FUNC_0, 4, PciReadDword(BUS_0, DEV_6, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x10, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x10) &0xffff0000) | 0xd001 );  // MIXER at IO 0xd000
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x14, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x14) &0xffff0000) | 0xd201 );  // BusMaster at IO 0xD200
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x18, 0xfec00000);	// memory base address 0xfec00000
//	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x4c, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x4c) ) | 0x0101 ); // was wrongly 0x01010000

		// frankenregister from working Linux driver

	PciWriteDword(BUS_0, DEV_6, FUNC_0, 8, 0x40100b1 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0xc, 0x800000 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x3c, 0x05020106 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x44, 0x20001 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x4c, 0x107 );

//
// Bus 0, Device 5, Function 0 = nForce MCP APU
//
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 4, PciReadDword(BUS_0, DEV_5, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_5, FUNC_0, 0x3c) &0xffff0000) | 0x0005 );
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 0x10, 0xfe800000);	// memory base address 0xfe800000

//
// Bus 0, Device 1, Function 0 = nForce HUB Interface - ISA Bridge
//
//	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x8c, 0x40000000 ); // from after pic challenge originally
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x8c, (PciReadDword(BUS_0, DEV_1, FUNC_0, 0x8c) &0xfbffffff) | 0x08000000 );

	bprintf("a\n");


//
// ACPI pin init
//
/*
	IoOutputByte(0x80c5, 0x05); // Set CPUSLEEP# pin to low
	IoOutputByte(0x80c6, 0x05); // Set CACHEZZ pin to low
	IoOutputByte(0x80c7, 0x05); // Set CACHEZZ pin to low
	IoOutputByte(0x80c8, 0x04); // Set CACHEZZ pin to low
	IoOutputDword(0x80b4, 0xffff);  // any interrupt resets ACPI system inactivity timer
*/
//#ifndef XBE
	IoOutputDword(0x80b4, 0xffff);  // any interrupt resets ACPI system inactivity timer
	IoOutputByte(0x80cc, 0x08); // Set EXTSMI# pin to be pin function
	IoOutputByte(0x80cd, 0x08); // Set PRDY pin on ACPI to be PRDY function

	IoOutputWord(0x8020, IoInputWord(0x8020)|0x200); // ack any preceding ACPI int
//#endif

	bprintf("b\n");

//
// Bus 0, Device 1e, Function 0 = nForce AGP Host to PCI Bridge
//
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 4, PciReadDword(BUS_0, DEV_1e, FUNC_0, 4) | 7 );
//	bprintf("Test Smb status3a =%x\n",IoInputWord(I2C_IO_BASE+0));
////	PciWriteWord(BUS_0, DEV_1e, FUNC_0, 0x118-0xf2, 0xf7f0); // was missing  0xf3f0 or 0xf7f0 depending on var
//	bprintf("Test Smb status3b =%x\n",IoInputWord(I2C_IO_BASE+0));
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x18, (PciReadDword(BUS_0, DEV_1e, FUNC_0, 0x18) &0xffffff00));
//	bprintf("Test Smb status3c =%x\n",IoInputWord(I2C_IO_BASE+0));
////	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x14, (PciReadDword(BUS_0, DEV_1e, FUNC_0, 0x14) &0x0000ffff) | 0x01010000); // was missing
//	bprintf("Test Smb status3d =%x\n",IoInputWord(I2C_IO_BASE+0));
//	bprintf("Test Smb status3e =%x\n",IoInputWord(I2C_IO_BASE+0));
//	PciWriteWord(BUS_0, DEV_1e, FUNC_0, 0x20, 0xfd00); // <-- copy of line above to maintain positioning!
//	PciWriteWord(BUS_0, DEV_1e, FUNC_0, 0x1e, 0xfe70); // kills SMB controller sometimes!!!
//	bprintf("Test Smb status3f =%x\n",IoInputWord(I2C_IO_BASE+0));
//	bprintf("Test Smb status3g =%x\n",IoInputWord(I2C_IO_BASE+0));
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x3c, 7);  // trying to get video irq

		// frankenregister xbe load correction to match cromwell load
		// controls requests for memory regions

	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x0c, 0xff019ee7);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x10, 0xbcfaf7e7);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x14, 0x0101fafa);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x1c, 0x02a000f0);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x20, 0xfdf0fd00);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x24, 0xf7f0f000);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x28, 0x8e7ffcff);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x2c, 0xf8bfef87);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x30, 0xdf758fa3);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x38, 0xb785fccc);


//
// Bus 0, Device 0, Function 0 = PCI Bridge Device - Host Bridge
//
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x48, 0x00000114);
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x44, 0x80000000); // new 2003-01-23 ag  trying to get single write actions on TSOP

	PciWriteByte(BUS_0, DEV_0, 3, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_0, FUNC_0, 0x4b,0x00);


//
// Bus 1, Device 0, Function 0 = NV2A GeForce3 Integrated GPU
//
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 4, PciReadDword(BUS_1, DEV_0, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x3c, (PciReadDword(BUS_1, DEV_0, FUNC_0, 0x3c) &0xffff0000) | 0x0103 );  // should get vid irq!!
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x4c, 0x00000114);

	PciWriteByte(BUS_0, DEV_0, FUNC_0, 0x87, 3); // kern 8001FC21

			// frankenregisters so Xromwell matches Cromwell

		PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x0c, 0x0);
		PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x18, 0x08);

#if 1
		__asm__ __volatile__(

		" wbinvd \n"

		" push %edx \n"
		" push %eax \n"

		" mov $0x80000854, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" inl %dx, %eax \n"
		" orl $0x88000000, %eax \n"
		" outl	%eax, %dx \n"

		" mov $0x80000064, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" inl %dx, %eax \n"
		" orl $0x88000000, %eax \n"
		" outl	%eax, %dx \n"

		" mov $0x8000006c, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" inl %dx, %eax \n"
		" push %eax \n"
		" andl $0xfffffffe, %eax \n"
		" outl	%eax, %dx \n"
		" pop %eax\n"
		" outl	%eax, %dx \n"

		" mov $0x80000080, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" movl $0x100, %eax \n"
		" outl	%eax, %dx \n"
/*
		" mov $0x8000088C, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" movl $0x40000000, %eax \n"
		" outl	%eax, %dx \n"
*/
		" pop %eax\n"
		" pop %edx \n"
		"	movl $0x0, 0x0f680600\n"
//		" sti\n"

		);
#endif
	bprintf("c\n");


}

