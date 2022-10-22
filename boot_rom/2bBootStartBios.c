/*

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-04-27 hamtitampti

 */

#include "2bload.h"
#include "sha1.h"
#include "lib/misc/LED.h"

unsigned char *BufferIN;
int BufferINlen;
unsigned char *BufferOUT;
int BufferOUTPos;

extern int decompress_kernel(char *out, char *data, int len);

void BootSystemInitialization(void)
{
    const u32 base = NV2A_MMIO_BASE;
    register u32 res;

    /* check to see if we're an Original Xbox first */
    if (machine_is_xbox()) {
        outb(0x02, 0x00ee); /* Turn LED green on OpenXenium, if connected */
    } else {
        /* do something else if we're not */
        outb(0x03, 0x00ee); /* Turn LED yellow on OpenXenium, if connected */
        outb(0x90, 0x0080);
        while (1);
    }

    /* translated to C from Xcodes.h */
    PciWriteDword(BUS_0, DEV_1,  FUNC_0, 0x84, 0x00008001); /* AMD-768 System Management (PM) IO BAR */
    PciWriteDword(BUS_0, DEV_1,  FUNC_0, 0x10, 0x00008001); /* AMD-768 System Management (PM) IO BAR */
    PciWriteDword(BUS_0, DEV_1,  FUNC_0, 0x04, 0x00000003);
    outb(0x08, 0x8049); /* PM49 - TCO timer halt */
    outb(0x00, 0x80d9); /* PMD9 - GPIO25 input mode */
    outb(0x01, 0x8026); /* PM26 */
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x4c, 0x00000001);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x18, 0x00010100);
    PciWriteDword(BUS_0, DEV_0,  FUNC_0, 0x84, 0x07ffffff);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x20, base | 0x00f00000 | (base >> 16)); /* was 0x0ff00f00 */
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x24, 0xf7f0f000);
    PciWriteDword(BUS_1, DEV_0,  FUNC_0, 0x10, base); /* was 0x0f000000; set NV2A MMIO BAR higher, since we're not X-codes */
    PciWriteDword(BUS_1, DEV_0,  FUNC_0, 0x14, 0xf0000000);
    PciWriteDword(BUS_1, DEV_0,  FUNC_0, 0x04, 0x00000007);
    PciWriteDword(BUS_0, DEV_1e, FUNC_0, 0x04, 0x00000007);
#ifndef MCPXREVD5
    writel(0x07633461, base + 0x0010b0);
#else
    writel(0x01000010, base + 0x0010b0);
#endif
    writel(0x66660000, base + 0x0010cc);
    res  = readl(base + 0x101000);
    res &= 0x000c0000;
    if (!res) {
        res  = readl(base + 0x101000);
        res &= 0xe1f3ffff;
        res |= 0x80000000;
        writel(res, base + 0x101000);
        writel(0xeeee0000, base + 0x0010b8);
    } else if (res == 0x000c0000) {
        res  = readl(base + 0x101000);
        res &= 0xe1f3ffff;
        res |= 0x860c0000;
        writel(res, base + 0x101000);
        writel(0xffff0000, base + 0x0010b8);
    } else {
        res  = readl(base + 0x101000);
        res &= 0xe1f3ffff;
        res |= 0x820c0000;
        writel(res, base + 0x101000);
        writel(0x11110000, base + 0x0010b8);
    }
    writel(0x00000009, base + 0x0010d4);
    writel(0x00000000, base + 0x0010b4);
    writel(0x00005866, base + 0x0010bc);
    writel(0x0351c858, base + 0x0010c4);
    writel(0x30007d67, base + 0x0010c8);
    writel(0x00000000, base + 0x0010d8);
    writel(0xa0423635, base + 0x0010dc);
    writel(0x0c6558c6, base + 0x0010e8);
    writel(0x03070103, base + 0x100200);
    writel(0x11000016, base + 0x100410);
    writel(0x84848888, base + 0x100330);
    writel(0xffffcfff, base + 0x10032c);
    writel(0x00000001, base + 0x100328);
    writel(0x000000df, base + 0x100338);

    /* initialize SMBus controller */
    PciWriteDword(BUS_0, DEV_1, FUNC_1, 0x04, 0x00000001);
    PciWriteDword(BUS_0, DEV_1, FUNC_1, 0x14, SMBUS | 1);
    PciWriteDword(BUS_0, DEV_1, FUNC_1, 0x18, SMBUS+0x200 | 1);
    outb(0x70, SMBUS+0x200);

    /* initialize video encoder */
    /*
     * It is necessary to write to the video encoder, as the PIC
     * snoops the I2C traffic and will reset us if it doesn't see what
     * it judges as 'appropriate' traffic. Conexant is the most urgent,
     * as on v1.0 Xboxes, the PIC was very strict and reset us earlier
     * than later models.
     */
    do {
        /* Conexant (CX25871) video encoder */
        smbus_write_addr(SMBUS_CONEXANT_ADDR);
        smbus_write_start(0xba, 0x3f); /* SLAVER | DACOFF | DACDIS{D,C,B,A} */
        if (smbus_cycle_completed()) {
            smbus_write(0x6c, 0x46); /* EN_REG_RD | EACTIVE | FLD_MODE[1] */
            smbus_write(0xb8, 0x00); /* autoconfiguration register */
            smbus_write(0xce, 0x19); /* OUT_MUXC[0] | OUT_MUXB[1] | OUT_MUXA[0] */
            smbus_write(0xc6, 0x9c); /* EN_BLANKO | {V,H}SYNCI | IN_MODE[2] */
            smbus_write(0x32, 0x08); /* IN_MODE[3] */
            smbus_write(0xc4, 0x01); /* EN_OUT */
            break;
        }

        /* Focus (FS454) video encoder */
        smbus_write_addr(SMBUS_FOCUS_ADDR);
        smbus_write_start(0x0c, 0x00);
        if (smbus_cycle_completed()) {
            smbus_write(0x0d, 0x20);
            break;
        }

        /* Xcalibur video encoder */
        smbus_write_addr(SMBUS_XCALIBUR_ADDR);
        smbus_write_start(0x00, 0x00);
        if (smbus_cycle_completed()) {
            smbus_write(0xb8, 0x00);
            break;
        }
    } while (0);

    smbus_write_addr(SMBUS_SMC_ADDR);
    smbus_write(0x01, 0x00);
    smbus_read_addr(SMBUS_SMC_ADDR);
    res = smbus_read(0x01); /* if SMC version does not match ... ????? */

    writel(0x00011c01, base + 0x680500);
    writel(0x000a0400, base + 0x68050c);
    writel(0x00000000, base + 0x001220);
    writel(0x00000000, base + 0x001228);
    writel(0x00000000, base + 0x001264);
    writel(0x00000010, base + 0x001210);
    res  = readl(base + 0x101000);
    res &= 0x06000000;
    if (!res) {
        writel(0x48480848, base + 0x001214);
        writel(0x88888888, base + 0x00122c);
    } else {
        writel(0x09090909, base + 0x001214);
        writel(0xaaaaaaaa, base + 0x00122c);
    }
    writel(0xffffffff, base + 0x001230);
    writel(0xaaaaaaaa, base + 0x001234);
    writel(0xaaaaaaaa, base + 0x001238);
    writel(0x8b8b8b8b, base + 0x00123c);
    writel(0xffffffff, base + 0x001240);
    writel(0x8b8b8b8b, base + 0x001244);
    writel(0x8b8b8b8b, base + 0x001248);
    writel(0x00000001, base + 0x1002d4);
    writel(0x00100042, base + 0x1002c4);
    writel(0x00100042, base + 0x1002cc);
    writel(0x00000011, base + 0x1002c0);
    writel(0x00000011, base + 0x1002c8);
    writel(0x00000032, base + 0x1002c0);
    writel(0x00000032, base + 0x1002c8);
    writel(0x00000132, base + 0x1002c0);
    writel(0x00000132, base + 0x1002c8);
    writel(0x00000001, base + 0x1002d0);
    writel(0x00000001, base + 0x1002d0);
    writel(0x80000000, base + 0x100210);
    writel(0xaa8baa8b, base + 0x00124c);
    writel(0x0000aa8b, base + 0x001250);
    writel(0x081205ff, base + 0x100228);
    writel(0x00010000, base + 0x001218);
    res  = PciReadDword(BUS_0, DEV_1, FUNC_0, 0x60);
    res |= 0x00000400;
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x60, res);
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x4c, 0x0000fdde);
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x9c, 0x871cc707);
    res  = PciReadDword(BUS_0, DEV_1, FUNC_0, 0xb4);
    res |= 0x00000f00;
    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0xb4, res);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x40, 0xf0f0c0c0);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x44, 0x00c00000);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x5c, 0x04070000);
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x6c, 0x00230801); /* FSB clock speed == 133 MHz; no override */
    PciWriteDword(BUS_0, DEV_0, FUNC_3, 0x6c, 0x01230801); /* FSB clock speed == 133 MHz; override */
    writel(0x03070103, base + 0x100200);
    writel(0x11448000, base + 0x100204);
    PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x3c, 0x00000000);

    /* report memory size to SMC scratch register */
    smbus_write_addr(SMBUS_SMC_ADDR);
    smbus_write(0x13, 0x0f);
    smbus_write(0x12, 0xf0);

    /* execution continues in 2bBootStartup.S */
    goto *0xfffc1000;
}

static INLINE void BootAGPBUSInitialization(void)
{
    register u32 res;

    PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x54, PciReadDword(BUS_0, DEV_1, FUNC_0, 0x54) | 0x88000000);
    PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x64, PciReadDword(BUS_0, DEV_0, FUNC_0, 0x64) | 0x88000000);

    res = PciReadDword(BUS_0, DEV_0, FUNC_0, 0x6c);
    outl(res & 0xfffffffe, PCI_CFG_DATA);
    outl(res, PCI_CFG_DATA);

    PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x80, 0x00000100);
}

/* -------------------------  Main Entry for after the ASM sequences ------------------------ */

void BootStartBiosLoader(void)
{
	// do not change this, this is linked to many many scipts
	unsigned int PROGRAMM_Memory_2bl 	= 0x00100000;
	unsigned int CROMWELL_Memory_pos 	= 0x03A00000;
	unsigned int CROMWELL_compress_temploc 	= 0x02000000;

	unsigned int Buildinflash_Flash[4]	= { 0xfff00000,0xfff40000,0xfff80000,0xfffc0000};
        unsigned int cromwellidentify	 	=  1;
        unsigned int flashbank		 	=  3;  // Default Bank
        unsigned int cromloadtry		=  0;

	struct SHA1Context context;
      	unsigned char SHA1_result[20];

        unsigned char bootloaderChecksum[20];
        unsigned int bootloadersize;
        unsigned int loadretry;
	unsigned int compressed_image_start;
	unsigned int compressed_image_size;
	unsigned int Biossize_type;

        int validimage;

	memcpy(&bootloaderChecksum[0],(void*)PROGRAMM_Memory_2bl,20);
	memcpy(&bootloadersize,(void*)(PROGRAMM_Memory_2bl+20),4);
	memcpy(&compressed_image_start,(void*)(PROGRAMM_Memory_2bl+24),4);
	memcpy(&compressed_image_size,(void*)(PROGRAMM_Memory_2bl+28),4);
	memcpy(&Biossize_type,(void*)(PROGRAMM_Memory_2bl+32),4);

      	SHA1Reset(&context);
	SHA1Input(&context,(void*)(PROGRAMM_Memory_2bl+20),bootloadersize-20);
	SHA1Result(&context,SHA1_result);

	if (memcmp(&bootloaderChecksum[0],&SHA1_result[0],20)) {
		// Bad, the checksum does not match, we did not get a valid image copied to RAM, so we stop and display an error.
		setLED("rrrr");
		while(1);
	}

        // Sets the Graphics Card to 60 MB start address
        (*(unsigned int*)0xFD600800) = (0xf0000000 | ((64*0x100000) - 0x00400000));

	BootAGPBUSInitialization();

	(*(unsigned int*)(0xFD000000 + 0x100200)) = 0x03070103 ;
	(*(unsigned int*)(0xFD000000 + 0x100204)) = 0x11448000 ;

        PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  // 128 MB

	// Lets go, we have finished, the Most important Startup, we have now a valid Micro-loder im Ram
	// we are quite happy now

        validimage=0;
        flashbank=3;
	for (loadretry=0;loadretry<100;loadretry++) {
		cromloadtry=0;
		if (Biossize_type==0) {
                	// Means we have a 256 kbyte image
                	 flashbank=3;
                }
               	else if (Biossize_type==1) {
                	// Means we have a 1MB image
                	// If 25 load attempts failed, we switch to the next bank
			switch (loadretry) {
				case 0:
					flashbank=1;
					break;
				case 25:
     	          			flashbank=2;
      					break;
        	        	case 50:
					flashbank=0;
                			break;
	                	case 75:
        	        		flashbank=3;
                			break;
			}
                }
		cromloadtry++;

        	// Copy From Flash To RAM
      		memcpy(&bootloaderChecksum[0],(void*)(Buildinflash_Flash[flashbank]+compressed_image_start),20);

                memcpy((void*)CROMWELL_compress_temploc,(void*)(Buildinflash_Flash[flashbank]+compressed_image_start+20),compressed_image_size);
		memset((void*)(CROMWELL_compress_temploc+compressed_image_size),0x00,20*1024);

		// Lets Look, if we have got a Valid thing from Flash
      		SHA1Reset(&context);
		SHA1Input(&context,(void*)(CROMWELL_compress_temploc),compressed_image_size);
		SHA1Result(&context,SHA1_result);

		if (memcmp(&bootloaderChecksum[0],SHA1_result,20)==0) {
			// The Checksum is good
			// We start the Cromwell immediatly

			setLED("rrrr");

			BufferIN = (unsigned char*)(CROMWELL_compress_temploc);
			BufferINlen=compressed_image_size;
			BufferOUT = (unsigned char*)CROMWELL_Memory_pos;
			decompress_kernel(BufferOUT, BufferIN, BufferINlen);

			// This is a config bit in Cromwell, telling the Cromwell, that it is a Cromwell and not a Xromwell
			flashbank++; // As counting starts with 0, we increase +1
			memcpy((void*)(CROMWELL_Memory_pos+0x20),&cromwellidentify,4);
			memcpy((void*)(CROMWELL_Memory_pos+0x24),&cromloadtry,4);
		 	memcpy((void*)(CROMWELL_Memory_pos+0x28),&flashbank,4);
		 	memcpy((void*)(CROMWELL_Memory_pos+0x2C),&Biossize_type,4);
		 	validimage=1;

		 	break;
		}
	}

	if (validimage==1) {
		setLED("oooo");

		// We now jump to the cromwell, Good bye 2bl loader
		// This means: jmp CROMWELL_Memory_pos == 0x03A00000
		__asm __volatile__ (
		"wbinvd\n"
		"cld\n"
		"ljmp $0x10, $0x03A00000\n"
		);
		// We are not Longer here
	}

	setLED("oooo");

//	I2CTransmitWord(0x10, 0x1901); // no reset on eject
//	I2CTransmitWord(0x10, 0x0c00); // eject DVD tray

        while(1);
}

