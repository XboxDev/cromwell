/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Copyright (C) 2004-2006 Xbox Linux authors                            *
 *   Copyright (C) 2019 Stanislav Motylkov                                 *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"
#include "video.h"
#include "memory_layout.h"
#include <shared.h>
#include <filesys.h>
#include "rc4.h"
#include "sha1.h"
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"
#include "BootParser.h"
#include "config.h"
#include "iso_fs.h"

CONFIGENTRY* DetectSystemNative(int drive, int partition) {
	return LoadConfigNative(drive, partition);
}

int BootFromNative(CONFIGENTRY *config) {
	return LoadKernelNative(config);
}

CONFIGENTRY* DetectSystemFatX(void) {
	return LoadConfigFatX();
}

int BootFromFatX(CONFIGENTRY *config) {
	return LoadKernelFatX(config);
}

CONFIGENTRY *DetectSystemCD(int cdromId) {
	return LoadConfigCD(cdromId);
}

int BootFromCD(CONFIGENTRY *config) {
	return LoadKernelCdrom(config);
}

int BootFromDevice(CONFIGENTRY *config) {
	int result = 0;

	switch (config->bootType) {
	case BOOT_CDROM:
		result = BootFromCD(config);
		break;
	case BOOT_FATX:
		result = BootFromFatX(config);
		break;
	case BOOT_NATIVE:
		result = BootFromNative(config);
		break;
	default:
		break;
	}

	if (result) ExittoLinux(config);
	return result;
}

#ifdef FLASH
int BootLoadFlashCD(int cdromId) {

	u32 dwConfigSize = 0;
	int n;
	int cdPresent = 0;
	struct SHA1Context context;
	unsigned char SHA1_result[20];
	unsigned char checksum[20];

	memset((u8 *)KERNEL_SETUP, 0, 4096);

	//See if we already have a CD in the drive
	//Try for 4 seconds.
	DVDTrayClose();
	for (n = 0; n < 16; ++n) {
		if ((BootIso9660GetFile(cdromId, "/image.bin", (u8 *)KERNEL_PM_CODE, 0x10)) >= 0) {
			cdPresent = 1;
			break;
		}
		wait_ms(250);
	}

	if (!cdPresent) {
		//Needs to be changed for non-xbox drives, which don't have an eject line
		//Need to send ATA eject command.
		DVDTrayEject();
		wait_ms(2000); // Wait for DVD to become responsive to inject command

		VIDEO_ATTR = 0xffeeeeff;

		printk("Please insert CD with image.bin file on, and press Button A\n");

		while (1) {
			if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) {
				DVDTrayClose();
				wait_ms(500);
				break;
			}
			USBGetEvents();
			wait_ms(10);
		}

		VIDEO_ATTR = 0xffffffff;

		// wait until the media is readable
		while (1) {
			if ((BootIso9660GetFile(cdromId, "/image.bin", (u8 *)KERNEL_PM_CODE, 0x10)) >= 0) {
				break;
			}
			wait_ms(200);
		}
	}
	printk("CD: ");
	printk("Loading BIOS image from /image.bin. \n");
	dwConfigSize = BootIso9660GetFile(cdromId, "/image.bin", (u8 *)KERNEL_PM_CODE, 256*1024);

	if (dwConfigSize < 0) { //It's not there
		printk("image.bin not found on CD... Halting\n");
		while (1);
	}

	printk("Image size: %i\n", dwConfigSize);
	if (dwConfigSize != 256 * 1024) {
		printk("Image is not a 256kB image - aborted\n");
		while (1);
	}
	SHA1Reset(&context);
	SHA1Input(&context, (u8 *)KERNEL_PM_CODE, dwConfigSize);
	SHA1Result(&context, SHA1_result);
	memcpy(checksum, SHA1_result, 20);
	printk("Result code: %d\n", BootReflashAndReset((u8*)KERNEL_PM_CODE, (u32)0, (u32)dwConfigSize));
	SHA1Reset(&context);
	SHA1Input(&context, (void *)LPCFlashadress, dwConfigSize);
	SHA1Result(&context, SHA1_result);
	if (!memcmp(checksum, SHA1_result, 20)) {
		printk("Checksum in flash matches - Flash successful.\nRebooting.");
		wait_ms(2000);
		I2CRebootSlow();
	} else {
		printk("Checksum in Flash not matching - MISTAKE - Reflashing!\n");
		printk("Result code: %d\n", BootReflashAndReset((u8*)KERNEL_PM_CODE, (u32)0, (u32)dwConfigSize));
	}
	return 0;
}
#endif //Flash
