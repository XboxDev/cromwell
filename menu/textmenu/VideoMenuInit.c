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

TEXTMENU *VideoMenuInit(void) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;

	menuPtr = malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	strcpy(menuPtr->szCaption, "Video Settings Menu");


	itemPtr = malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	if(((u8 *)&eeprom)[0x96]&0x01) {
		strcpy(itemPtr->szCaption, "Display Size: Widescreen");
	}
	else {
		strcpy(itemPtr->szCaption, "Display Size: Normal");
	}
	itemPtr->functionPtr=SetWidescreen;
	itemPtr->functionDataPtr = itemPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);
	
	
	itemPtr = malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	
	switch(*((VIDEO_STANDARD *)&eeprom.VideoStandard)) {
		case NTSC_M:
			strcpy(itemPtr->szCaption, "TV Standard: NTSC-USA");
			break;
		case NTSC_J:
			strcpy(itemPtr->szCaption, "TV Standard: NTSC-Japan");
			break;
		case PAL_I:
			strcpy(itemPtr->szCaption, "TV Standard: PAL");
			break;
		default:
			strcpy(itemPtr->szCaption, "TV Standard: Unknown");
		break;
	}
	itemPtr->functionPtr=SetVideoStandard;
	itemPtr->functionDataPtr = itemPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	return menuPtr;
}
