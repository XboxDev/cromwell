/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* This is where you should customise the menu, by adding your own icons.
 * The code in IconMenu.c should normally be left alone.
 */
#include "BootIde.h"
#include "IconMenu.h"
#include "MenuActions.h"

void InitFatXIcons(void);
void InitNativeIcons(void);

void IconMenuInit(void) {
	int i=0;
	ICON *iconPtr=0l;
	for (i=0; i<2; ++i) {
		//Add the cdrom icon - if you have two cdroms, you'll get two icons!
		if (tsaHarddiskInfo[i].m_fAtapi) {
			char *driveName=malloc(sizeof(char)*14);
			sprintf(driveName,"CD-ROM (hd%c)",i ? 'b':'a');
			iconPtr = (ICON *)malloc(sizeof(ICON));
			iconPtr->iconSlot = ICON_SOURCE_SLOT2;
			iconPtr->szCaption = driveName;
			iconPtr->functionPtr = BootFromCD;
			iconPtr->functionDataPtr = malloc(sizeof(int));
			iconPtr->functionDataPtr=(void*)i;
			AddIcon(iconPtr);
		}
	}
	//Load the config file from FATX and native, and add the icons for it.
	InitFatXIcons();
	InitNativeIcons();
	
#ifdef ETHERBOOT
	//Etherboot icon - if it's compiled in, it's always available.
	iconPtr = (ICON *)malloc(sizeof(ICON));
	iconPtr->iconSlot = ICON_SOURCE_SLOT3;
	iconPtr->szCaption = "Etherboot";
	iconPtr->functionPtr = BootFromEtherboot;
	AddIcon(iconPtr);
#endif	
	//Uncomment this one to test the new text menu system.
	//It's NOT production ready.
	/*
	iconPtr = (ICON *)malloc(sizeof(ICON));
	iconPtr->iconSlot = ICON_SOURCE_SLOT0;
	iconPtr->szCaption = "Advanced";
	iconPtr->functionPtr = BootTextMenu;
	AddIcon(iconPtr);
	*/

	//Set this to point to the icon you want to be selected by default.
	//Otherwise, leave it alone, and the first icon will be selected.
	//selectedIcon = iconPtr;
}

void InitFatXIcons(void) {
	ICON *iconPtr=0l;
	BYTE ba[512];
	memset(ba,0x00,512);
	BootIdeReadSector(0, &ba[0], 3, 0, 512);
	if (!strncmp("BRFR",&ba[0],4)) {
           //FATX icon - this is inadequate.
	   //a) Needs to check linuxboot.cfg or whatever exists
	   //b) Should add more icons per entry.
	   iconPtr = (ICON *)malloc(sizeof(ICON));
	   iconPtr->iconSlot = ICON_SOURCE_SLOT4;
	   iconPtr->szCaption = "FatX (E:)";
	   iconPtr->functionPtr = BootFromFATX;
	   AddIcon(iconPtr);
	}
}

void InitNativeIcons(void) {
	extern int nActivePartitionIndex;
	ICON *iconPtr=0l;
	BYTE ba[512];
	memset(ba,0x00,512);

	//This needs enhancing to check multiple HDDs, and support multiple
	//boot entries.
	BootIdeReadSector(0, &ba[0], 3, 0, 512);
	        
	//Is there an MBR here?
	if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) {
		volatile BYTE * pb;
		int n=0, nPos=0;
		(volatile BYTE *)pb=&ba[0x1be];
		//Check the first four partitions (this isn't good enough!)
		for (n=0; n<4; n++,pb+=16) {
			//Is this partition bootable?
			if(pb[0]&0x80) {
				nActivePartitionIndex=n;
				iconPtr = (ICON *)malloc(sizeof(ICON));
				iconPtr->iconSlot = ICON_SOURCE_SLOT1;
				iconPtr->szCaption = "HDD";
				iconPtr->functionPtr = BootFromNative;
				AddIcon(iconPtr);
			}
		}
	}
}
