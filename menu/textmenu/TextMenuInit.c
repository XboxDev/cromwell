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
	strcpy(menuPtr->szCaption, "Main Menu");
	menuPtr->firstMenuItem=NULL;
	
	//VIDEO SETTINGS MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	strcpy(itemPtr->szCaption, "Video Settings");
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)VideoMenuInit();
	TextMenuAddItem(menuPtr, itemPtr);

	//HDD MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	strcpy(itemPtr->szCaption, "Hdd Menu");
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)HddMenuInit();
	TextMenuAddItem(menuPtr, itemPtr);
	
#ifdef FLASH
	//FLASH MENU
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	strcpy(itemPtr->szCaption, "Flash Menu");
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)FlashMenuInit();
	TextMenuAddItem(menuPtr, itemPtr);
#endif
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	strcpy(itemPtr->szCaption, "Reset Menu");
	itemPtr->functionPtr=DrawChildTextMenu;
	itemPtr->functionDataPtr = (void *)ResetMenuInit();
	TextMenuAddItem(menuPtr, itemPtr);
	
	return menuPtr;
}
