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

void TextMenuInit(void) {
	
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	
	//Create the root menu - MANDATORY
	firstMenu = malloc(sizeof(TEXTMENU));
	firstMenu->szCaption="Main Menu\n";
	firstMenu->parentMenu=NULL;
	firstMenu->firstMenuItem=NULL;
	
	//VIDEO SETTINGS MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Video Settings";	
	TextMenuAddItem(firstMenu, itemPtr);
	VideoMenuInit(itemPtr);

	//HDD MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Hdd Menu";	
	TextMenuAddItem(firstMenu, itemPtr);
	HddMenuInit(itemPtr);

	
#ifdef FLASH
	//FLASH MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Flash Menu";	
	TextMenuAddItem(firstMenu, itemPtr);
	FlashMenuInit(itemPtr);
#endif
	//RESET MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Reset Menu";	
	TextMenuAddItem(firstMenu, itemPtr);
	ResetMenuInit(itemPtr);
}
