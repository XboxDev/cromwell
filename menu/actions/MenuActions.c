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
	BootLoadConfigFATX((CONFIGENTRY*)configEntry);
	ExittoLinux((CONFIGENTRY*)configEntry);
}

void SetLEDColor(void *color) {
	I2cSetFrontpanelLed(*(u8*)color);
}

//More grub bits
unsigned long saved_drive;
unsigned long saved_partition;
grub_error_t errnum;
unsigned long boot_drive;

extern unsigned long current_drive;

void BootFromNative(void *partitionId) {
	CONFIGENTRY config;
	//This stuff is needed to keep the grub FS code happy.
	char szGrub[256+4];
	int menu=0,selected=0;
	
	memset(szGrub,0x00,sizeof(szGrub));
	szGrub[0]=0xff;
	szGrub[1]=0xff;
	szGrub[2]=*(int*)partitionId;
	szGrub[3]=0x00;
	errnum=0;
	boot_drive=0;
	saved_drive=0;
	saved_partition=0x0001ffff;
	buf_drive=-1;
	current_partition=0x0001ffff;
	current_drive=0xff;
	buf_drive=-1;
	fsys_type = NUM_FSYS;
	disk_read_hook=NULL;
	disk_read_func=NULL;
	BootLoadConfigNative(*(int*)partitionId,&config,false);
	ExittoLinux(&config);
}
