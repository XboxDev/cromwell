/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "include/boot.h"
#include "BootIde.h"
#include "TextMenu.h"
#include "HddMenuActions.h"

TEXTMENU *HddMenuInit(void) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	int i=0;

	menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	strcpy(menuPtr->szCaption, "Hdd Menu");

	for (i=0; i<2; ++i) {
		if (tsaHarddiskInfo[i].m_fDriveExists && !tsaHarddiskInfo[i].m_fAtapi) {
			//If it's not ATAPI, it must be IDE
			if((tsaHarddiskInfo[i].m_securitySettings &0x0004)==0x0004) {
				//This drive is locked - produce an unlock menu
				itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
				memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
				sprintf(itemPtr->szCaption,"Unlock HDD (hd%c)",i ? 'b':'a');
				itemPtr->functionPtr= UnlockHdd;
    				itemPtr->functionDataPtr = malloc(sizeof(int));
                        	*(int*)itemPtr->functionDataPtr = i;
				TextMenuAddItem(menuPtr, itemPtr);
			}
			else {
				//This drive is unlocked - produce an lock menu
				itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
				memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
				sprintf(itemPtr->szCaption,"Lock HDD (hd%c)",i ? 'b':'a');
				itemPtr->functionPtr= LockHdd;
    				itemPtr->functionDataPtr = malloc(sizeof(int));
                        	*(int*)itemPtr->functionDataPtr = i;
				TextMenuAddItem(menuPtr, itemPtr);
			}
		}
	}
	return menuPtr;
}
