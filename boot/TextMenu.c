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
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"
#include "MenuActions.h"
#include "config.h"

struct TEXTMENUITEM;
struct TEXTMENU;

typedef struct {
	char *szCaption;
	void (*functionPtr) (void);
	//Child menu, if any
	struct TEXTMENU *childMenu;
	//Next / previous menu items, if linked list
	struct TEXTMENUITEM *previousMenuItem;
	struct TEXTMENUITEM *nextMenuItem;
} TEXTMENUITEM;

typedef struct {
	char *szCaption;
	TEXTMENUITEM* firstMenuItem;
	struct TEXTMENU* parentMenu;
} TEXTMENU;

TEXTMENU *firstMenu=0l;
TEXTMENU *currentMenu=0l;
TEXTMENUITEM *firstVisibleMenuItem=0l;
TEXTMENUITEM *selectedMenuItem=0l;

void TextMenuDraw(void) {
	VIDEO_CURSOR_POSX=25;
	VIDEO_CURSOR_POSY=100;
	if (currentMenu==0l) {
		return;
	}
	//Draw the menu title.
	VIDEO_ATTR=0x0000ff;
	printk("\2%s\n",currentMenu->szCaption);
	VIDEO_CURSOR_POSY+=30;
	
	TEXTMENUITEM *item;
	for (item = currentMenu->firstMenuItem; item!=0l; item=(TEXTMENUITEM *)item->nextMenuItem) {
		if (item == selectedMenuItem) VIDEO_ATTR=0xff0000;
		else VIDEO_ATTR=0xffffff;
		printk("\2%s\n",item->szCaption);
		VIDEO_CURSOR_POSY+=15;
	}
}

int BootTextMenu(void) {
	TEXTMENUITEM *menuItemPtr;

	firstMenu = malloc(sizeof(TEXTMENU));
	firstMenu->szCaption="Main Menu";
	currentMenu = firstMenu;	
	
	menuItemPtr = malloc(sizeof(TEXTMENUITEM));
	menuItemPtr->szCaption="Test 2";	
	firstMenu->firstMenuItem = menuItemPtr;
	menuItemPtr->previousMenuItem=0l;
	selectedMenuItem = menuItemPtr;
	
	TEXTMENUITEM *menuitem2 = malloc(sizeof(TEXTMENUITEM));
	menuItemPtr->nextMenuItem=(struct TEXTMENUITEM *)menuitem2;
	menuitem2->szCaption="Test 1";
	menuitem2->previousMenuItem=(struct TEXTMENUITEM*)menuItemPtr;
	menuitem2->nextMenuItem=0l;
	
	TextMenuDraw();
	
	//Main menu event loop.
	while(1)
	{
		int changed=0;
		USBGetEvents();
		
		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_UP) == 1)
		{
			if (selectedMenuItem->previousMenuItem!=0l) {
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->previousMenuItem;
				changed=1;
			}
			
		} 
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_DOWN) == 1) {
			if (selectedMenuItem->nextMenuItem!=0l) {
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->nextMenuItem;
				changed=1;
			}
		}
			
		if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1)) {
			/*		memcpy((void*)FB_START,videosavepage,FB_SIZE);
			free(videosavepage);
			
			VIDEO_CURSOR_POSX=nTempCursorResumeX;
			VIDEO_CURSOR_POSY=nTempCursorResumeY;
			*/
			//Menu item selected - invoke function pointer.
			if (selectedMenuItem->functionPtr!=0l) selectedMenuItem->functionPtr();
			if (selectedMenuItem->childMenu!=0l) {
				currentMenu = (TEXTMENU*)selectedMenuItem->childMenu;
				selectedMenuItem = currentMenu->firstMenuItem;
			}
			//Should never come back but at least if we do, the menu can
			//continue to work.
			//Setting changed means the icon menu will redraw itself.
			changed=1;
		}
		if (changed) {
		//	BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);
			TextMenuDraw();
			changed=0;
		}
	}
}

