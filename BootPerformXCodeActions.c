/****************************************************************************************
 * Hardware initialization as done by the original Xcode
 *
 *  (C)2002 Michael Steil
 *
 ****************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"

/****************************
 * Memory and IO regions
 ****************************/
#define ISA_IO        0x8000
#define VGA_MMIO      0x0F000000
#define VGA_MEM       0xF0000000
#define SMBUS_IO      0xC000
#define SMBUS_STATUS  SMBUS_IO
#define SMBUS_CONTROL SMBUS_IO + 2
#define SMBUS_ADDRESS SMBUS_IO + 4
#define SMBUS_DATA    SMBUS_IO + 6
#define SMBUS_COMMAND SMBUS_IO + 8
#define SMBUS_IO2     0xC200
#define SMBUS_PIC     0x10
#define SMBUS_VIDEO   0x45

/****************************
 * Port I/O
 ****************************/

/*
static __inline void outb (unsigned short port, unsigned char value) {
    __asm__ __volatile__ ("outb %b0,%w1": :"a" (value), "Nd" (port));
}

static __inline void outl (unsigned short port, unsigned int value) {
    __asm__ __volatile__ ("outl %0,%w1": :"a" (value), "Nd" (port));
}

static __inline unsigned char inb (unsigned short port) {
  unsigned char _v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (_v):"Nd" (port));
  return _v;
}

static __inline unsigned int inl (unsigned short port) {
  unsigned int _v;

  __asm__ __volatile__ ("inl %w1,%0":"=a" (_v):"Nd" (port));
  return _v;
}
*/



/****************************
 * Memory access
 ****************************/
static __inline void pokel(unsigned int a, unsigned int d) {
    *((volatile unsigned int*)(a)) = d;
}

static __inline unsigned char peekb(unsigned int a) {
    return (*((volatile unsigned char*)(a)));
}

static __inline unsigned int peekl(unsigned int a) {
    return (*((volatile unsigned int*)(a)));
}

/****************************
 * PCI
 ****************************/
#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA    0xcfc

#define PCI_COMMAND 4

#define PCI_CMD_BUSMASTER 4
#define PCI_CMD_MEM       2
#define PCI_CMD_IO        1

#define PCI_HOST  PCI(0,0,0)
#define PCI_MEM   PCI(0,0,3)
#define PCI_ISA   PCI(0,1,0)
#define PCI_SMBUS PCI(0,1,1)
#define PCI_USB1  PCI(0,2,0)
#define PCI_USB2  PCI(0,3,0)
#define PCI_AGP   PCI(0,30,0)
#define PCI_VGA   PCI(1,0,0)

#define PCI(bus,dev,fn) 0x80000000 | (bus << 16) | (dev << 11) | (fn << 8)

static __inline void pci_pokel(unsigned int bdf, unsigned char reg, unsigned int data) {
    IoOutputDword(PCI_CONFIG_ADDRESS, bdf | (reg & ~3));
    IoOutputDword(PCI_CONFIG_DATA, data);
}

static __inline unsigned int pci_peekl(unsigned int bdf, unsigned char reg) {
    IoOutputDword(PCI_CONFIG_ADDRESS, bdf | (reg & ~3));
    return IoInputDword(PCI_CONFIG_DATA);
}

/****************************
 * VGA MMIO
 ****************************/
static __inline void vga_pokel(unsigned int a, unsigned int d) {
    pokel(VGA_MMIO + a, d);
}

static __inline unsigned int vga_peekl(unsigned int a) {
    return peekl(VGA_MMIO + a);
}

/****************************
 * Memory detection
 ****************************/
#define MAXADDR_64MB 0x3FFFFFF
#define MAXADDR_128MB 0x7FFFFFF

#define MEM_PATTERN1 0xAAAAAAAA
#define MEM_PATTERN2 0x5A5A5A5A
#define MEM_PATTERN3 0x55555555

void mem_detect(unsigned int a, unsigned char b, unsigned char *mem1, unsigned char *mem2) {
    pokel(a, MEM_PATTERN1);
    pokel(a + 0x02000000, MEM_PATTERN2);
    if (peekl(a + 0x02000000)==MEM_PATTERN2) {
        if (peekl(a)==MEM_PATTERN1)
            *mem1 |= b;
        pokel(a, MEM_PATTERN3);
        if (peekl(a)==MEM_PATTERN3) return;
    }
    *mem2 |= b;
}

/****************************
 * MAIN
 ****************************/
int BootPerformXCodeActions() {
    unsigned char mem1, mem2;
    int i;

    /* configure ISA bridge */
    pci_pokel(PCI_ISA, 0x10, ISA_IO | 1);	/* set I/O to 0x8000 */
    pci_pokel(PCI_ISA, PCI_COMMAND, PCI_CMD_MEM | PCI_CMD_IO); /* activate I/O and memory */
    IoOutputByte(ISA_IO + 0x49, 8);
    IoOutputByte(ISA_IO + 0xD9, 0);
    IoOutputByte(ISA_IO + 0x26, 1);

    /* configure AGP bridge and VGA */
    /* ??? */
    pci_pokel(PCI_AGP, 0x4c, 1);	/* same as nForce contents */
    /* Bus: primary=00, secondary=01, subordinate=01, sec-latency=0 */
    pci_pokel(PCI_AGP, 0x18, 0x10100);
    /* set amount of memory to 128 MB */
    pci_pokel(PCI_HOST, 0x84, MAXADDR_128MB);	/* nForce: total RAM minus VGA! */
    /* Memory behind bridge: 0x0F000000-0x0FFFFFFF */
    pci_pokel(PCI_AGP, 0x20, 0x0FF00F00);
    /* Prefetchable memory behind bridge: 0xF0000000-0xF7FFFFFF */
    pci_pokel(PCI_AGP, 0x24, 0xF7F0F000);
    /* set VGA MMIO */
    pci_pokel(PCI_VGA, 0x10, VGA_MMIO);
    /* set VGA video ram */
    pci_pokel(PCI_VGA, 0x14, VGA_MEM);
    /* activate I/O, memory, busmaster */
    pci_pokel(PCI_VGA, PCI_COMMAND, PCI_CMD_BUSMASTER | PCI_CMD_MEM | PCI_CMD_IO);
    /* activate I/O, memory, busmaster */
    pci_pokel(PCI_AGP, PCI_COMMAND, PCI_CMD_BUSMASTER | PCI_CMD_MEM | PCI_CMD_IO);

    if (peekb(VGA_MMIO)==0xA1) {
        vga_pokel(0x10B0, 0x7633451);
        vga_pokel(0x10CC, 0);
        vga_pokel(0x10B8, 0xFFFF0000);
        vga_pokel(0x10D4, 5);
    } else {
        vga_pokel(0x10B0, 0x7633461);
        vga_pokel(0x10CC, 0x66660000);
        vga_pokel(0x10B8, 0xFFFF0000);
        vga_pokel(0x10D4, 9);
    }
    vga_pokel(0x10B4, 0);
    vga_pokel(0x10BC, 0x5866);
    vga_pokel(0x10C4, 0x351C858);
    vga_pokel(0x10C8, 0x30007D67);
    vga_pokel(0x10D8, 0);
    vga_pokel(0x10DC, 0xA0423635);
    vga_pokel(0x10E8, 0xC6558C6);
    vga_pokel(0x100200, 0x3070103);
    vga_pokel(0x100410, 0x11000016);
    vga_pokel(0x100330, 0x84848888);
    vga_pokel(0x10032C, 0xFFFFCFFF);
    vga_pokel(0x100328, 0x1);
    vga_pokel(0x100338, 0xDF);
    if (peekb(VGA_MMIO)==0xA1) {
        vga_pokel(0x101000, 0x803D4401);
    }

    /* configure SMBus controller */
    pci_pokel(PCI_SMBUS, PCI_COMMAND, PCI_CMD_IO);	/* activate I/O */
    pci_pokel(PCI_SMBUS, 0x14, SMBUS_IO | 1);		/* set I/O to 0xC000 */
    pci_pokel(PCI_SMBUS, 0x18, SMBUS_IO2 | 1);		/* set I/O to 0xC200 */
    IoOutputByte(SMBUS_IO2, 0x70);

    /* initialize video encoder */
    I2CTransmit2Bytes(SMBUS_VIDEO, 0xBA, 0x3F);
    I2CTransmit2Bytes(SMBUS_VIDEO, 0x6C, 0x46);
    I2CTransmit2Bytes(SMBUS_VIDEO, 0xB8, 0);
    I2CTransmit2Bytes(SMBUS_VIDEO, 0xCE, 0x19);
    I2CTransmit2Bytes(SMBUS_VIDEO, 0xC6, 0x9C);
    I2CTransmit2Bytes(SMBUS_VIDEO, 0x32, 8);
    I2CTransmit2Bytes(SMBUS_VIDEO, 0xC4, 1);

    /* ??? */
    I2CTransmit2Bytes(SMBUS_PIC, 1, 0);
    if (I2CTransmitByteGetReturn(SMBUS_PIC, 1)!=0x50) pci_pokel(PCI_MEM, 0x6C, 0x01000000);

    /* some more VGA init */
    if (peekb(VGA_MMIO)==0xA1)
        vga_pokel(0x680500, 0x11701);
    else
        vga_pokel(0x680500, 0x11C01);

    vga_pokel(0x68050C, 0xA0400);
    vga_pokel(0x1220, 0);
    vga_pokel(0x1228, 0);
    vga_pokel(0x1264, 0);
    vga_pokel(0x1210, 0x10);

    if ((vga_peekl(0x101000) & 0xC0000) == 0)
        vga_pokel(0x1214, 0x10101010);
    else
        vga_pokel(0x1214, 0x12121212);

    vga_pokel(0x122C, 0xAAAAAAAA);
    vga_pokel(0x1230, 0xAAAAAAAA);
    vga_pokel(0x1234, 0xAAAAAAAA);
    vga_pokel(0x1238, 0xAAAAAAAA);
    vga_pokel(0x123C, 0x8B8B8B8B);
    vga_pokel(0x1240, 0x8B8B8B8B);
    vga_pokel(0x1244, 0x8B8B8B8B);
    vga_pokel(0x1248, 0x8B8B8B8B);
    vga_pokel(0x1002D4, 1);
    vga_pokel(0x1002C4, 0x100042);
    vga_pokel(0x1002CC, 0x100042);
    vga_pokel(0x1002C0, 0x11);
    vga_pokel(0x1002C8, 0x11);
    vga_pokel(0x1002C0, 0x32);
    vga_pokel(0x1002C8, 0x32);
    vga_pokel(0x1002C0, 0x132);
    vga_pokel(0x1002C8, 0x132);
    vga_pokel(0x1002D0, 0x1);
    vga_pokel(0x1002D0, 0x1);
    vga_pokel(0x100210, 0x80000000);
    vga_pokel(0x124C, 0xAA8BAA8B);
    vga_pokel(0x1250, 0xAA8B);
    vga_pokel(0x100228, 0x81205FF);
    vga_pokel(0x1218, 0x10000);

    pci_pokel(PCI_ISA, 0x60, (pci_peekl(PCI_ISA, 0x60) & 0xFFFF) | 0x400);
    pci_pokel(PCI_ISA, 0x4C, 0xFDDE);
    pci_pokel(PCI_ISA, 0x9C, 0x871CC707);
    pci_pokel(PCI_ISA, 0xB4, (pci_peekl(PCI_ISA, 0xB4) & 0xF0FF) | 0xF00);

    pci_pokel(PCI_MEM, 0x40, 0xF0F0C0C0);
    pci_pokel(PCI_MEM, 0x44, 0xC00000);
    pci_pokel(PCI_MEM, 0x5C, 0x4070000);

    if (peekb(VGA_MMIO)==0xA1);	/* makes no sense other than "nop" - or ack? */

    pci_pokel(PCI_MEM, 0x6C, 0x230801);
    pci_pokel(PCI_MEM, 0x6C, 0x1230801);

    vga_pokel(0x100200, 0x3070103);
    vga_pokel(0x100204, 0x11448000); /* shared memory? value for 128 MB machines */

    /* memory detection */
    mem1 = 0; mem2 = 0;
    for (i=0; i<8; i++) mem_detect(0x00555548 + ((i & 3) << 4) + ((i & 4) << 24), 1<<i, &mem1, &mem2);

    /* setup of VGA shared memory depending on memory size */
    vga_pokel(0x100200, 0x3070003);
    pci_pokel(PCI_HOST, 0x84, MAXADDR_64MB);
    switch (mem1) {
        case 0x00: /* 64 MB */
            vga_pokel(0x100200, 0x3070103);
            vga_pokel(0x100204, 0x11338000); /* shared memory? value for 64 MB machines */
        case 0xff: /* 128 MB */
            vga_pokel(0x100200, 0x3070103);
            pci_pokel(PCI_HOST, 0x84, MAXADDR_128MB);
    }

    /* send memory size/status to PIC */
    I2CTransmit2Bytes(SMBUS_PIC, 0x13, mem1);
    I2CTransmit2Bytes(SMBUS_PIC, 0x12, mem2);

    /* Memory behind bridge: 0xFD000000-0xFDFFFFFF */
    pci_pokel(PCI_AGP, 0x20, 0xFDF0FD00);
    /* set VGA MMIO to what kernel and libraries expect */
    pci_pokel(PCI_VGA, 0x10, 0xFD000000);
		
		return 0;

}

