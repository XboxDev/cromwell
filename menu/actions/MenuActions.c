/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#include "VideoInitialization.h"

void MoveToTextMenu(void *nothing) {
	TextMenuInit();
	TextMenu();
}

void BootFromCD(void *data) {
	CONFIGENTRY *config = (CONFIGENTRY*)BootLoadConfigCD(*(int*)data);
	ExittoLinux(config);
}

#ifdef ETHERBOOT 
extern int etherboot(void);
void BootFromEtherboot(void *data) {
	etherboot();
}
#endif

#ifdef FLASH
void FlashBios(void *data) {
	BootLoadFlashCD();
}
#endif

void BootFromFATX(void *configEntry) {
	LoadKernelFatX((CONFIGENTRY*)configEntry);
	ExittoLinux((CONFIGENTRY*)configEntry);
}

//More grub bits
unsigned long saved_drive;
unsigned long saved_partition;
grub_error_t errnum;
unsigned long boot_drive;

extern unsigned long current_drive;

void BootFromNative(void *config) {
	LoadKernelNative((CONFIGENTRY*)config);
	ExittoLinux((CONFIGENTRY*)config);
}
