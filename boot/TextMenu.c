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
	//Menu item text
	char *szCaption;
	//Pointer to function to run when menu item selected.
	void (*functionPtr) (void);
	//Child menu, if any, attached to this menu item
	struct TEXTMENU *childMenu;
	//Next / previous menu items within this menu
	struct TEXTMENUITEM *previousMenuItem;
	struct TEXTMENUITEM *nextMenuItem;
} TEXTMENUITEM;

typedef struct {
	//Menu title e.g. "Main Menu"
	char *szCaption;
	//A pointer to the first item of the linked list of menuitems that
	//make up this menu.
	TEXTMENUITEM* firstMenuItem;
	//If 0l, we're a top level menu, otherwise a "BACK" menu item will be created,
	//which takes us back to the parent menu..
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

	int menucount;
	TEXTMENUITEM *item=firstVisibleMenuItem;
	for (menucount=0; menucount<7; menucount++) {
		if (item==0l) {
			//No more menu items to draw
			return;
		}
		//Selected item in red
		if (item == selectedMenuItem) VIDEO_ATTR=0xff0000;
		else VIDEO_ATTR=0xffffff;
		//Font size 2=big.
		printk("\2%s\n",item->szCaption);
		VIDEO_CURSOR_POSY+=15;
		item=(TEXTMENUITEM *)item->nextMenuItem;
	}
}

int BootTextMenu(void) {
	
	unsigned char *textmenusavepage;
	TEXTMENUITEM *previousItemPtr, *newItemPtr;

	firstMenu = malloc(sizeof(TEXTMENU));
	firstMenu->szCaption="Main Menu";
	currentMenu = firstMenu;	

	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test 1";	
	firstMenu->firstMenuItem = newItemPtr;
	newItemPtr->previousMenuItem=0l;
	newItemPtr->nextMenuItem=0l;
	selectedMenuItem = newItemPtr;
	firstVisibleMenuItem = newItemPtr;
	previousItemPtr = newItemPtr;
	
	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	newItemPtr->szCaption="Test 2";	
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;

	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test 3";	
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;


	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test 4";	
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;

	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test 5";	
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;

	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test 6";	
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;

	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test 7";	
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;



	TEXTMENU *testchildmenu = (TEXTMENU*)malloc(sizeof(TEXTMENU));
	testchildmenu->szCaption="Test child menu";
	
	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test 8";	
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;
	newItemPtr->childMenu = (struct TEXTMENU*)testchildmenu;
	
	//TEST CHILD MENU
	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test Child1";	
	newItemPtr->previousMenuItem=0l;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;
	
	testchildmenu->firstMenuItem = newItemPtr;
	
	
	newItemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	previousItemPtr->nextMenuItem = (struct TEXTMENUITEM*)newItemPtr;
	memset(newItemPtr,0x00,sizeof(TEXTMENUITEM));
	newItemPtr->szCaption="Test child2";
	newItemPtr->previousMenuItem=(struct TEXTMENUITEM*)previousItemPtr;
	newItemPtr->nextMenuItem=0l;
	previousItemPtr = newItemPtr;

	//TEST CHILD MENU

	TextMenuDraw();
	
	//Main menu event loop.
	while(1)
	{
		int changed=0;
		USBGetEvents();
		
		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_UP) == 1)
		{
			if (selectedMenuItem->previousMenuItem!=0l) {
				if (selectedMenuItem == firstVisibleMenuItem) {
					firstVisibleMenuItem = (TEXTMENUITEM *)selectedMenuItem->previousMenuItem;
					memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
				}
				
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->previousMenuItem;
				changed=1;
			}
		} 
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_DOWN) == 1) {
			if (selectedMenuItem->nextMenuItem!=0l) {
				TEXTMENUITEM *lastVisibleMenuItem = firstVisibleMenuItem;
				int i=0;
				//6 menu items per page
				for (i=0; i<6; i++) {
					if (lastVisibleMenuItem->nextMenuItem==0l) break;
					lastVisibleMenuItem = (TEXTMENUITEM *)lastVisibleMenuItem->nextMenuItem;
				}
				if (selectedMenuItem == lastVisibleMenuItem) {
					firstVisibleMenuItem = (TEXTMENUITEM *)firstVisibleMenuItem->nextMenuItem;
					memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
				}
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->nextMenuItem;
				changed=1;
			}
		}
			
		else if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1)) {
			memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
			free(textmenusavepage);
			//Menu item selected - invoke function pointer.
			if (selectedMenuItem->functionPtr!=0l) selectedMenuItem->functionPtr();
			//Re-malloc these if the function pointer did return us to the menu.	
			textmenusavepage = malloc(FB_SIZE);
			memcpy(textmenusavepage,(void*)FB_START,FB_SIZE);
			//Display the childmenu, if this menu item has one.	
			if (selectedMenuItem->childMenu!=0l) {
				currentMenu = (TEXTMENU*)selectedMenuItem->childMenu;
				selectedMenuItem = currentMenu->firstMenuItem;
				firstVisibleMenuItem = currentMenu->firstMenuItem;
			}
			changed=1;
		}
		if (changed) {
			TextMenuDraw();
			changed=0;
		}
	}
}

