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

//	__asm__ __volatile__ ( "nop; nop; nop; nop; nop; nop ; wbinvd" );

//
// Bus 0, Device 1, Function 0 = nForce HUB Interface - ISA Bridge
//
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x6a, 0x03);
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x6c, 0x0e065491);
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x64, 0x00000b0c);
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x81, PciReadByte(BUS_0, DEV_1, FUNC_0, 0x81)|8);


//
// Bus 0, Device 9, Function 0 = nForce ATA Controller
//

	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x20, 0x0000ff61);	// (BMIBA) Set Busmaster regs I/O base address 0xff60
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 4, 0x00b00005 );
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 8, 0x01018ab1 ); // was fffffaff
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x40, 0x000084bb); // new
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x50, 0x00000003);  // without this there is no register footprint at IO 1F0


//
// Bus 0, Device 4, Function 0 = nForce MCP Networking Adapter
//
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 4, PciReadDword(BUS_0, DEV_4, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x10, 0xfef00000); // memory base address 0xfef00000
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x14, 0x0000e001); // I/O base address 0xe000
	PciWriteDword(BUS_0, DEV_4, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_4, FUNC_0, 0x3c) &0xffff0000) | 0x0004 );

//
// Bus 0, Device 2, Function 0 = nForce OHCI USB Controller
//

	PciWriteDword(BUS_0, DEV_2, FUNC_0, 4, PciReadDword(BUS_0, DEV_2, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x10, 0xfed00000);	// memory base address 0xfed00000
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_2, FUNC_0, 0x3c) &0xffff0000) | 0x0001 );
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x50, 0x0000000f);

//
// Bus 0, Device 3, Function 0 = nForce OHCI USB Controller
//

	PciWriteDword(BUS_0, DEV_3, FUNC_0, 4, PciReadDword(BUS_0, DEV_3, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x10, 0xfed08000);	// memory base address 0xfed08000
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_3, FUNC_0, 0x3c) &0xffff0000) | 0x0009 );
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x50, 0x00000030);

//
// Bus 0, Device 6, Function 0 = nForce Audio Codec Interface
//

	PciWriteDword(BUS_0, DEV_6, FUNC_0, 4, PciReadDword(BUS_0, DEV_6, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x3c) &0xffff0000) | 0x0006 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x10, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x10) &0xffff0000) | 0xd001 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x14, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x14) &0xffff0000) | 0xd201 );
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x18, 0xfec00000);	// memory base address 0xfec00000

//
// Bus 0, Device 5, Function 0 = nForce MCP APU
//
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 4, PciReadDword(BUS_0, DEV_5, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 0x3c, (PciReadDword(BUS_0, DEV_5, FUNC_0, 0x3c) &0xffff0000) | 0x0005 );
	PciWriteDword(BUS_0, DEV_5, FUNC_0, 0x10, 0xfe800000);	// memory base address 0xfe800000

//
// Bus 0, Device 1, Function 0 = nForce HUB Interface - ISA Bridge
//
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x8c, 0x40000000 ); // from after pic challenge originally
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x8c, (PciReadDword(BUS_0, DEV_1, FUNC_0, 0x8c) &0xfbffffff) | 0x08000000 );

//
// ACPI pin init
//
	IoOutputByte(0x80cd, 0x08); // Set PRDY pin on ACPI to be PRDY function


//
// Bus 0, Device 6, Function 0 = nForce Audio Codec Interface
//
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x4c, (PciReadDword(BUS_0, DEV_6, FUNC_0, 0x4c) ) | 0x01010000 );

//
// Bus 0, Device 1e, Function 0 = nForce AGP Host to PCI Bridge
//
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 4, PciReadDword(BUS_0, DEV_1e, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x18, (PciReadDword(BUS_0, DEV_1e, FUNC_0, 0x18) &0xff000000) | 0x010100);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x20, 0xfe70fc70);
	PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x24, 0xf440ec30);


//
// Bus 0, Device 0, Function 0 = PCI Bridge Device - Host Bridge
//
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x48, 0x00000114);

	PciWriteByte(BUS_0, DEV_0, 0, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_0, 1, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_0, 2, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_0, 3, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_1, 0, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_1, 1, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_1, 2, 0x42, 0x17);
	PciWriteByte(BUS_0, DEV_1, 3, 0x42, 0x17);

	PciWriteByte(BUS_0, DEV_0, FUNC_0, 0x4b,0x00);

//
// Bus 1, Device 0, Function 0 = NV2A GeForce3 Integrated GPU
//
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 4, PciReadDword(BUS_1, DEV_0, FUNC_0, 4) | 7 );
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x3c, (PciReadDword(BUS_1, DEV_0, FUNC_0, 0x3c) &0xffff0000) | 0x0103 );
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x4c, 0x00000114);

#if 1
		__asm__ __volatile__(

		" push %edx \n"
		" push %eax \n"

		" mov $0x80000854, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" in %dx, %eax \n"
		" orl $0x88000000, %eax \n"
		" outl	%eax, %dx \n"

		" mov $0x80000064, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" in %dx, %eax \n"
		" orl $0x88000000, %eax \n"
		" outl	%eax, %dx \n"

		" mov $0x8000006c, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" in %dx, %eax \n"
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

		" mov $0x8000088C, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" movl $0x40000000, %eax \n"
		" outl	%eax, %dx \n"

		" pop %eax\n"
		" pop %edx \n"
//		" sti\n"

		);

#endif



}

