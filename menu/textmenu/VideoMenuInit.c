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

#include "VideoInitialization.h"

void VideoMenuInit(TEXTMENUITEM *parentItem) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;

	
	menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	menuPtr->szCaption="Video Settings Menu";
	menuPtr->parentMenu=(struct TEXTMENU*)firstMenu;
	parentItem->childMenu = (struct TEXTMENU*)menuPtr;


	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption = malloc(20);
	if(((BYTE *)&eeprom)[0x96]&0x01) {
		strcpy(itemPtr->szCaption, "Display Size: Widescreen");
	}
	else {
		strcpy(itemPtr->szCaption, "Display Size: Normal");
	}
	itemPtr->functionPtr=SetWidescreen;
	itemPtr->functionDataPtr = &itemPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);
	
	{
		xbox_tv_encoding  b = DetectVideoStd();
	
		itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
		memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
		itemPtr->szCaption = malloc(20);
		switch (b) {
			case TV_ENC_PALBDGHI:
				strcpy(itemPtr->szCaption, "TV Standard: PAL");
				break;
			case TV_ENC_NTSC:
			default:
				strcpy(itemPtr->szCaption, "TV Standard: NTSC-USA");
				break;
		}
		itemPtr->functionPtr=SetVideoStandard;
		itemPtr->functionDataPtr = &itemPtr->szCaption;
		TextMenuAddItem(menuPtr, itemPtr);
	}
}
