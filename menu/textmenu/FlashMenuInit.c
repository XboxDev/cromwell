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
#include "FlashMenuActions.h"

void FlashMenuInit(TEXTMENUITEM *parentItem) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	int i=0;

	menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	menuPtr->szCaption="Flash Menu";
	menuPtr->parentMenu=(struct TEXTMENU*)firstMenu;
	parentItem->childMenu = (struct TEXTMENU*)menuPtr;

	for (i=0; i<2; ++i) {
		if (tsaHarddiskInfo[i].m_fDriveExists && tsaHarddiskInfo[i].m_fAtapi) {
   			char *driveName=malloc(sizeof(char)*32);
			itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
			memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
                        sprintf(driveName,"Flash bios from CD-ROM (hd%c)",i ? 'b':'a');
			itemPtr->szCaption = driveName;
			itemPtr->functionPtr= FlashBiosFromCD;
    			itemPtr->functionDataPtr = malloc(sizeof(int));
                        *(int*)itemPtr->functionDataPtr = i;
			TextMenuAddItem(menuPtr, itemPtr);
		}
	}
}
