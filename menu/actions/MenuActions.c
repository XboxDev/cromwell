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
#include "TextMenu.h"
void MoveToTextMenu(void *nothing) {
	TEXTMENU *parentMenu = (TEXTMENU*)TextMenuInit();
	TextMenu(parentMenu);
}

void BootFromCD(void *data) {
	int nTempCursorY = VIDEO_CURSOR_POSY; 
	CONFIGENTRY *config = (CONFIGENTRY*)LoadConfigCD(*(int*)data);
	if (config==NULL) {
		printk("Boot from CD failed.\nCheck that linuxboot.cfg exists.\n");
		wait_ms(2000);
		//Clear the screen and return to the menu
		BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);	
		return;
	}
	DrawBootMenu(config);
}

void DrawBootMenu(void *rootEntry) {
	//entry is the pointer to the root config entry
	TEXTMENU *menu;
	TEXTMENUITEM *menuPtr;
	CONFIGENTRY *configEntry, *currentConfigEntry;
	extern int timedOut;
	
	configEntry = (CONFIGENTRY*)rootEntry;

	if (configEntry->nextConfigEntry==NULL) {
		//If there is only one option, just boot it.
		BootMenuEntry(configEntry);
		return;
	}

	if (timedOut) {
		//We should be non-interactive, then.
		//If there is a default entry, boot that.
		for (currentConfigEntry = configEntry; currentConfigEntry != NULL; 
			currentConfigEntry = (CONFIGENTRY*)currentConfigEntry->nextConfigEntry) {
			if (currentConfigEntry->isDefault) {
				BootMenuEntry(currentConfigEntry);
				return;
			}
		}
		//There wasn't a default entry, so just boot the first in the list
		BootMenuEntry(configEntry);
		return;
	}
	
	menu = malloc(sizeof(TEXTMENU));
	memset(menu,0x00,sizeof(TEXTMENU));
	menu->szCaption="Boot menu\n";
  
	for (currentConfigEntry = configEntry; currentConfigEntry != NULL; 
		currentConfigEntry = (CONFIGENTRY*)currentConfigEntry->nextConfigEntry) {
	
		menuPtr = (TEXTMENUITEM *)malloc(sizeof(TEXTMENUITEM*));
		memset(menuPtr, 0x00, sizeof(menuPtr));
		if (currentConfigEntry->title == NULL) {
			menuPtr->szCaption="Untitled";
		}
		else menuPtr->szCaption=currentConfigEntry->title;
		menuPtr->functionPtr = BootMenuEntry;
		menuPtr->functionDataPtr = (void *)currentConfigEntry;
		TextMenuAddItem(menu,menuPtr);
	}
	TextMenu(menu);
}

void BootMenuEntry(void *entry) {
	CONFIGENTRY *config = (CONFIGENTRY*)entry;
	switch (config->bootType) {
		case BOOT_CDROM:
			LoadKernelCdrom(config);
			break;
		case BOOT_FATX:
			LoadKernelFatX(config);
			break;
		case BOOT_NATIVE:
			LoadKernelNative(config);
			break;
	}
	ExittoLinux(config);
}

void DrawChildTextMenu(void *menu) {
	TextMenu((TEXTMENU*)menu);
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

