/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "include/boot.h"
#include "TextMenu.h"
#include "ResetMenuActions.h"

void ResetMenuInit(TEXTMENUITEM *parentItem) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	
	menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	menuPtr->szCaption="Reset Menu";
	menuPtr->parentMenu=(struct TEXTMENU*)firstMenu;
	parentItem->childMenu = (struct TEXTMENU*)menuPtr;


	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption = "Reboot (slow)";
	itemPtr->functionPtr=SlowReboot;
	itemPtr->functionDataPtr = NULL;
	TextMenuAddItem(menuPtr, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption = "Reboot (fast)";
	itemPtr->functionPtr=QuickReboot;
	itemPtr->functionDataPtr = NULL;
	TextMenuAddItem(menuPtr, itemPtr);
	
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption = "Power off";
	itemPtr->functionPtr=PowerOff;
	itemPtr->functionDataPtr = NULL;
	TextMenuAddItem(menuPtr, itemPtr);
}
