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
CONFIGENTRY *LoadConfigCD(int);
TEXTMENU *TextMenuInit(void);

void AdvancedMenu(void *textmenu) {
	TextMenu((TEXTMENU*)textmenu, NULL);
}

void BootFromCD(void *data) {
	//We have to go an extra step when the CD icon is selected, as unlike
	//the other boot modes, we have not parsed the linuxboot.cfg file yet.
	int nTempCursorY = VIDEO_CURSOR_POSY; 
	CONFIGENTRY *config = LoadConfigCD(*(int*)data);
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
	TEXTMENUITEM *menuPtr, *defaultMenuItem;
	CONFIGENTRY *configEntry, *currentConfigEntry;
	extern int timedOut;

	defaultMenuItem=NULL;
	configEntry = rootEntry;

	if (configEntry->nextConfigEntry==NULL) {
		//If there is only one option, just boot it.
		BootMenuEntry(configEntry);
		return;
	}

	if (timedOut) {
		//We should be non-interactive, then.
		//If there is a default entry, boot that.
		for (currentConfigEntry = configEntry; currentConfigEntry != NULL; 
			currentConfigEntry = currentConfigEntry->nextConfigEntry) {
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
	strcpy(menu->szCaption, "Boot menu");
  
	for (currentConfigEntry = configEntry; currentConfigEntry != NULL; 
		currentConfigEntry = currentConfigEntry->nextConfigEntry) {
	
		menuPtr = (TEXTMENUITEM *)malloc(sizeof(TEXTMENUITEM*));
		memset(menuPtr, 0x00, sizeof(menuPtr));
		if (currentConfigEntry->title == NULL) {
			strcpy(menuPtr->szCaption,"Untitled");
		}
		else strncpy(menuPtr->szCaption,currentConfigEntry->title,50);
		menuPtr->functionPtr = BootMenuEntry;
		menuPtr->functionDataPtr = (void *)currentConfigEntry;
		//If this config entry is default, mark the menu item as default.
		if (currentConfigEntry->isDefault) defaultMenuItem = menuPtr;
		TextMenuAddItem(menu,menuPtr);
	}
	TextMenu(menu, defaultMenuItem);
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

