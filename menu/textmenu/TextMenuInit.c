/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "include/config.h"
#include "TextMenu.h"

#include "VideoInitialization.h"

TEXTMENU *TextMenuInit(void) {
	
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	
	//Create the root menu - MANDATORY
	menuPtr = malloc(sizeof(TEXTMENU));
	menuPtr->szCaption="Main Menu\n";
	menuPtr->firstMenuItem=NULL;
	
	//VIDEO SETTINGS MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Video Settings";	
	TextMenuAddItem(menuPtr, itemPtr);
	menuPtr->firstMenuItem = itemPtr;
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)VideoMenuInit();

	//HDD MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Hdd Menu";	
	TextMenuAddItem(menuPtr, itemPtr);
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)HddMenuInit();
	
#ifdef FLASH
	//FLASH MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Flash Menu";	
	TextMenuAddItem(menuPtr, itemPtr);
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)FlashMenuInit();
#endif
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Reset Menu";	
	TextMenuAddItem(menuPtr, itemPtr);
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)ResetMenuInit();
	
	return menuPtr;
}
