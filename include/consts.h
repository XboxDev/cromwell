#ifndef _Consts_H_
#define _Consts_H_

/*
 *
 * includes for startup code in a form usable by the .S files
 *
 */

  /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define VERSION "1.26 dev"

#define PCI_CFG_ADDR 0x0CF8
#define PCI_CFG_DATA 0x0CFC

#define MTRR_DEF 0x2ff
#define MTRR_DEF_TYPE 0x800
#define MTRR_PHYSBASE 0x200
#define MTRR_LAST 0x20F
#define WB_CACHE 6
#define BASE0_H 0
#define BASE0_L WB_CACHE
#define MASK0_H 0x0F
#define MASK0_L 0xFC000800
#define BASE1_H 0
#define BASE1_L 0xFFF80005 // 0x0FFF80005
#define MASK1_H 0x0F
#define MASK1_L 0x0FFF80800

#define I2C_IO_BASE 0xc000

#define BUS_0 0
#define BUS_1 1

#define DEV_0 0
#define DEV_1 1
#define DEV_2 2
#define DEV_3 3
#define DEV_4 4
#define DEV_5 5
#define DEV_6 6
#define DEV_7 7
#define DEV_8 8
#define DEV_9 9
#define DEV_a 0xa
#define DEV_b 0xb
#define DEV_c 0xc
#define DEV_d 0xd
#define DEV_e 0xe
#define DEV_f 0xf
#define DEV_10 0x10
#define DEV_11 0x11
#define DEV_12 0x12
#define DEV_13 0x13
#define DEV_14 0x14
#define DEV_15 0x15
#define DEV_16 0x16
#define DEV_17 0x17
#define DEV_18 0x18
#define DEV_19 0x19
#define DEV_1a 0x1a
#define DEV_1b 0x1b
#define DEV_1c 0x1c
#define DEV_1d 0x1d
#define DEV_1e 0x1e
#define DEV_1f 0x1f

#define FUNC_0 0
/*
#define boot_post_macro(value)                     \
		movb    $(value), %al                           ;\
		outb    %al, $0x80 
*/
/* Filtror debug stuff  4K block used for communications */
#define FILT_DEBUG_BASE 0xff0fe000
#define FILT_DEBUG_FOOTPRINT 0x1000
#define FILT_DEBUG_MAX_DATA ((FILT_DEBUG_FOOTPRINT/2)-4)
#define FILT_DEBUG_TOPC_START (FILT_DEBUG_BASE+0)
#define FILT_DEBUG_FROMPC_START (FILT_DEBUG_BASE+(FILT_DEBUG_FOOTPRINT/2))
#define FILT_DEBUG_TOPC_LEN (FILT_DEBUG_BASE+(FILT_DEBUG_FOOTPRINT/2)-2)
#define FILT_DEBUG_FROMPC_LEN (FILT_DEBUG_BASE+FILT_DEBUG_FOOTPRINT-2)
#define FILT_DEBUG_TOPC_CHECKSUM (FILT_DEBUG_BASE+(FILT_DEBUG_FOOTPRINT/2)-4)
#define FILT_DEBUG_FROMPC_CHECKSUM (FILT_DEBUG_BASE+FILT_DEBUG_FOOTPRINT-4)

#define MEMORYMANAGERSTART 	0x01000000
#define MEMORYMANAGERSIZE 	 0x1000000 // 16 MB
#define MEMORYMANAGEREND 	0x01FFFFFF

#endif // _Consts_H_

/*

Memory Mapping Cromwell
 	
 	0x0000 0000 -> free ?
 	
 	0x0009 0000 Kernel Info Header( 0x10000 size = 65536Bytes)
 	0x0009 FFFF Kernel Info Header
 	
 	0x0010 0000 Kompressed Kernel ( 0x200000 size = 2 MB)
 	0x002F FFFF Kompressed Kernel

	0x0030 0000 Linked Rambase in Memory
	?? until ?

 	0x0100 0000 Memorymanager for Cromwell (0x1000000 size = 16 MB)
 	0x01FF FFFF Memorymanager
 	
 	0x0200 0000 Linux Ramdisk Starting address (0x1B00000 size = 28MB)
 	0x03AF FFFF Linux Ramdisk End address
 	
 	0x03B0 0000 c/x romwell Ramcopy Start (0x100000 size = 1MB) 
 	0x03BF FFFF c/x romwell Ramcopy End

 	0x03C0 0000 Video Memory Start (4MB)
 	0x03FF FFFF Video Memory End
 	
 	
 	0x0400 0000 Physical Ram End
 	
 	0xFFF0 0000 Flash Copy(0) Start
 	0xFFF3 FFFF Flash Copy(0) End
 	
 	0xFFF4 0000 Flash Copy(1) Start
 	0xFFF7 FFFF Flash Copy(1) End
 		
 	0xFFF8 0000 Flash Copy(2) Start
 	0xFFFB FFFF Flash Copy(2) End 
 	0xFFFC 0000 Flash Copy(3) Start
 	0xFFFF FFFF Flash Copy(3) End 
 	
hamtitampti & ed

*/
