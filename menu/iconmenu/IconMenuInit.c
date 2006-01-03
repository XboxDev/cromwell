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
#include "include/config.h"
#include "BootIde.h"
#include "IconMenu.h"
#include "MenuActions.h"

void InitFatXIcons(void);
void InitNativeIcons(void);

void IconMenuInit(void) {
	int i=0;
	ICON *iconPtr=NULL;
	for (i=0; i<2; ++i) {
		//Add the cdrom icon - if you have two cdroms, you'll get two icons!
		if (tsaHarddiskInfo[i].m_fAtapi) {
			char *driveName=malloc(sizeof(char)*14);
			sprintf(driveName,"CD-ROM (hd%c)",i ? 'b':'a');
			iconPtr = malloc(sizeof(ICON));
			iconPtr->iconSlot = ICON_SOURCE_SLOT2;
			iconPtr->szCaption = driveName;
			iconPtr->functionPtr = BootFromCD;
			iconPtr->functionDataPtr = malloc(sizeof(int));
			*(int*)iconPtr->functionDataPtr = i;
			AddIcon(iconPtr);
		}
	}
	//Load the config file from FATX and native, and add the icons, if found.
	InitFatXIcons();
	InitNativeIcons();
	
#ifdef ETHERBOOT
	//Etherboot icon - if it's compiled in, it's always available.
	iconPtr = malloc(sizeof(ICON));
	iconPtr->iconSlot = ICON_SOURCE_SLOT3;
	iconPtr->szCaption = "Etherboot";
	iconPtr->functionPtr = BootFromEtherboot;
	AddIcon(iconPtr);
#endif	

#ifdef ADVANCED_MENU
	iconPtr = malloc(sizeof(ICON));
	iconPtr->iconSlot = ICON_SOURCE_SLOT0;
	iconPtr->szCaption = "Advanced";
	iconPtr->functionPtr = AdvancedMenu;
	iconPtr->functionDataPtr = (void *)TextMenuInit();
	AddIcon(iconPtr);
#endif
	//Set this to point to the icon you want to be selected by default.
	//Otherwise, leave it alone, and the first icon will be selected.
	//selectedIcon = iconPtr;
}

void InitFatXIcons(void) {
	ICON *iconPtr=NULL;
	u8 ba[512];
	int driveId=0;
	
	if (tsaHarddiskInfo[driveId].m_fDriveExists && !tsaHarddiskInfo[driveId].m_fAtapi) {
		memset(ba,0x00,512);
		BootIdeReadSector(driveId, ba, 3, 0, 512);
		if (!strncmp("BRFR",ba,4)) {
			//Got a FATX formatted HDD
			CONFIGENTRY *entry = (CONFIGENTRY*)LoadConfigFatX();
			if (entry !=NULL) {
				//There is a config file present.
				iconPtr = malloc(sizeof(ICON));
		   		iconPtr->iconSlot = ICON_SOURCE_SLOT4;
				iconPtr->szCaption="FatX (E:)";
				iconPtr->functionPtr = DrawBootMenu;
				iconPtr->functionDataPtr = (void *)entry;
		   		AddIcon(iconPtr);
				//If we have fatx, mark it as default.
				//If there are natives, they'll get priority shortly
				selectedIcon = iconPtr;
			}
		}
	}
}

void InitNativeIcons(void) {
	ICON *iconPtr=NULL;
	u8 ba[512];
	int driveId;	

	for (driveId=0; driveId<2; driveId++) {
		if (tsaHarddiskInfo[driveId].m_fDriveExists && !tsaHarddiskInfo[driveId].m_fAtapi) {
			volatile u8 *pb;
			int n=0, nPos=0;
			
			memset(ba,0x00,512);
			BootIdeReadSector(driveId, ba, 0, 0, 512);
			        
			//See if there is an MBR - no MBR means no native boot options for this drive.
			if( !(ba[0x1fe]==0x55) || !(ba[0x1ff]==0xaa)) continue;
	
			pb=&ba[0x1be];
			//Check the primary partitions
			for (n=0; n<4; n++,pb+=16) {
				if(pb[0]&0x80) {
					//Bootable flag IS set on this partition.
					CONFIGENTRY *entry = (CONFIGENTRY*)LoadConfigNative(driveId, n);
					if (entry!=NULL) {
						//There is a valid config file here.
						//Add an icon for this partition 
						iconPtr = malloc(sizeof(ICON));
			  			iconPtr->iconSlot = ICON_SOURCE_SLOT1;
						iconPtr->szCaption=malloc(10);
						sprintf(iconPtr->szCaption, "hd%c%d", driveId+'a', n);
						iconPtr->functionPtr = DrawBootMenu;
						iconPtr->functionDataPtr = (void *)entry;
			  			AddIcon(iconPtr);
						selectedIcon = iconPtr;
					}
				}
			}
			
		}
	}
}
