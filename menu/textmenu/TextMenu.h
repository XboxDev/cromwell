#ifndef _TEXTMENU_H_
#define _TEXTMENU_H_

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "boot.h"
#include "video.h"
#include "memory_layout.h"
#include <shared.h>
#include "BootFATX.h"
#include "xbox.h"
#include "cpu.h"
#include "BootIde.h"
#include "MenuActions.h"
#include "config.h"

struct TEXTMENUITEM;
struct TEXTMENU;

#define MENUCAPTIONSIZE 50
extern int breakOutOfMenu;

typedef struct TEXTMENUITEM {
	//Menu item text
	char szCaption[MENUCAPTIONSIZE+1];
	//Pointer to function to run when menu item selected.
	//If NULL, menuitem will not do anything when selected
	void (*functionPtr) (void *);
	//Pointer to data used by the function above.
	void *functionDataPtr;
	//Next / previous menu items within this menu
	struct TEXTMENUITEM *previousMenuItem;
	struct TEXTMENUITEM *nextMenuItem;
} TEXTMENUITEM;

typedef struct TEXTMENU {
	//Menu title e.g. "Main Menu"
	char szCaption[MENUCAPTIONSIZE+1];
	//A pointer to the first item of the linked list of menuitems that
	//make up this menu.
	TEXTMENUITEM* firstMenuItem;
} TEXTMENU;

#endif
