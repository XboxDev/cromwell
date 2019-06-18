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

#define GRUB_REQUEST_SIZE (256+4)

char *InitGrubRequest(int size, int drive, int partition) {
	char *szGrub;

	szGrub = (char *)malloc(size);
	memset(szGrub, 0, size);

	szGrub[0] = 0xff;
	szGrub[1] = 0xff;
	szGrub[2] = partition;
	szGrub[3] = drive;

	errnum = 0;
	boot_drive = 0;
	saved_drive = 0;
	saved_partition = 0x0001ffff;
	buf_drive = -1;
	current_partition = 0x0001ffff;
	current_drive = drive;
	buf_drive = -1;
	fsys_type = NUM_FSYS;
	disk_read_hook = NULL;
	disk_read_func = NULL;

	return szGrub;
}

void FillConfigEntries(CONFIGENTRY *config, enum BootTypes bootType, int drive, int partition) {
	CONFIGENTRY *currentConfigItem;

	for (currentConfigItem = config; currentConfigItem != NULL; currentConfigItem = currentConfigItem->nextConfigEntry) {
		currentConfigItem->bootType = bootType;

		// Set the drive ID and partition IDs for the returned config items
		switch (bootType) {
		case BOOT_NATIVE:
			currentConfigItem->drive = drive;
			currentConfigItem->partition = partition;
			break;
		case BOOT_FATX:
			break;
		case BOOT_CDROM:
			currentConfigItem->drive = drive;
			break;
		default:
			break;
		}
	}
}

CONFIGENTRY *DetectSystemNative(int drive, int partition) {
	CONFIGENTRY *config;
	char *szGrub;

	szGrub = InitGrubRequest(GRUB_REQUEST_SIZE, drive, partition);
	config = DetectLinuxNative(szGrub);
	free(szGrub);

	FillConfigEntries(config, BOOT_NATIVE, drive, partition);

	return config;
}

int BootFromNative(CONFIGENTRY *config) {
	char *szGrub;
	int result;

	DVDTrayClose();

	szGrub = InitGrubRequest(GRUB_REQUEST_SIZE, config->drive, config->partition);
	result = LoadLinuxNative(szGrub, config);
	free(szGrub);

	return result;
}

CONFIGENTRY *DetectSystemFatX(void) {
	CONFIGENTRY *config;
	FATXPartition *partition;

	partition = OpenFATXPartition(0, SECTOR_STORE, STORE_SIZE);
	if (!partition)
		return NULL;

	config = DetectLinuxFATX(partition);
	CloseFATXPartition(partition);

	FillConfigEntries(config, BOOT_FATX, 0, 0);

	return config;
}

int BootFromFatX(CONFIGENTRY *config) {
	FATXPartition *partition;
	int result;

	DVDTrayClose();

	partition = OpenFATXPartition(0, SECTOR_STORE, STORE_SIZE);
	if (!partition)
		return false;

	result = LoadLinuxFATX(partition, config);
	CloseFATXPartition(partition);

	return result;
}

CONFIGENTRY *DetectSystemCD(int cdromId) {
	int n;
	CONFIGENTRY *config = NULL;
	int nTempCursorX, nTempCursorY;

	printk("\2Please wait\n\n");
	//See if we already have a CD in the drive
	//Try for 8 seconds - takes a while to 'spin up'.
	nTempCursorX = VIDEO_CURSOR_POSX;
	nTempCursorY = VIDEO_CURSOR_POSY;

	while (config == NULL)
	{
		DVDTrayClose();
		printk("Detecting system on CD... \n");
		for (n = 0; n < 32; ++n) {
			config = DetectLinuxCD(cdromId);
			if (config != NULL) {
				break;
			}
			wait_ms(250);
		}

		//We couldn't read the disk, so we eject the drive so the user can insert one.
		if (config == NULL) {
			printk("Boot from CD failed.\nCheck that supported system exists on the CD.\n\n");
			//Needs to be changed for non-xbox drives, which don't have an eject line
			//Need to send ATA eject command.
			DVDTrayEject();
			wait_ms(2000); // Wait for DVD to become responsive to inject command

			VIDEO_ATTR = 0xffeeeeff;
			printk("\2Please insert CD and press Button A\n\n\2Press Button B to return to main menu\n\n");

			while (1) {
				// Retry system detection
				config = DetectLinuxCD(cdromId);

				// Make button 'A' close the DVD tray
				if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) {
					DVDTrayClose();
					wait_ms(500);
					break;
				}
				else if (DVD_TRAY_STATE == DVD_CLOSING) {
					//It's an xbox drive, and somebody pushed the tray in manually
					wait_ms(500);
					break;
				}
				else if (config != NULL) {
					//It isnt an xbox drive, and somebody pushed the tray in manually, and
					//the cd is valid.
					break;
				}
				// Allow to cancel CD boot with button 'B'
				else if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1) {
					// Close DVD tray and return to main menu
					DVDTrayClose();
					wait_ms(500);
					return NULL;
				}
				wait_ms(10);
			}

			wait_ms(250);

			VIDEO_ATTR = 0xffffffff;

			BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY + 1);
			VIDEO_CURSOR_POSX = nTempCursorX;
			VIDEO_CURSOR_POSY = nTempCursorY;
		}
	}

	//Populate the configs with the drive ID
	FillConfigEntries(config, BOOT_CDROM, cdromId, 0);

	return config;
}

int BootFromCD(CONFIGENTRY *config) {
	return LoadLinuxCD(config);
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

	memset((u8 *)KERNEL_SETUP, 0, KERNEL_HDR_SIZE);

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
