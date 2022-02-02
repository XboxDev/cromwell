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

#define PCI_CFG_ADDR 0x0CF8
#define PCI_CFG_DATA 0x0CFC


#define TIMER_IO   0x8008
#define TIMER_FREQ 0x369E99

#define I2C_IO_BASE 0xC000

#define SERIAL_PORT 0x3F8
#define SERIAL_IRQ  4
#define SERIAL_THR  0
#define SERIAL_LSR  5

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

#endif // _Consts_H_


