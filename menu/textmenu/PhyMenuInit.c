/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Copyright (C) 2019 Stanislav Motylkov                                 *
 *                                                                         *
 ***************************************************************************/

#include "include/boot.h"
#include "TextMenu.h"
#include "PhyMenuActions.h"

TEXTMENU *PhyMenuInit(void) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;

	menuPtr = malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	strcpy(menuPtr->szCaption, "Peripherals Menu");

	CurrentSerialState = IsSerialEnabled();

	itemPtr = malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	strcpy(itemPtr->szCaption, "Serial COM1: ");
	strcat(itemPtr->szCaption, (CurrentSerialState) ? "Enabled" : "Disabled");
	itemPtr->functionPtr = SetSerialEnabled;
	itemPtr->functionDataPtr = itemPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	CurrentSerialIRQState = HasSerialIRQ();

	itemPtr = malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	strcpy(itemPtr->szCaption, "Serial COM1 IRQ: ");
	strcat(itemPtr->szCaption, (CurrentSerialIRQState) ? "4 (conflicts with NIC)" : "Disabled");
	itemPtr->functionPtr = SetSerialIRQ;
	itemPtr->functionDataPtr = itemPtr->szCaption;
	TextMenuAddItem(menuPtr, itemPtr);

	return menuPtr;
}
