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

void TextMenuDraw(void);
void TextMenuBack(void);

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
unsigned char *textmenusavepage;

void TextMenuAddItem(TEXTMENU *menu, TEXTMENUITEM *newMenuItem) {
	TEXTMENUITEM *menuItem = menu->firstMenuItem;
	TEXTMENUITEM *currentMenuItem=0l;
	
	while (menuItem != 0l) {
		currentMenuItem = menuItem;
		menuItem = (TEXTMENUITEM*)menuItem->nextMenuItem;
	}
	
	if (currentMenuItem==0l) { 
		//This is the first icon in the chain
		menu->firstMenuItem = newMenuItem;
	}
	//Append to the end of the chain
	else currentMenuItem->nextMenuItem = (struct TEXTMENUITEM*)newMenuItem;
	newMenuItem->nextMenuItem = 0l;
	newMenuItem->previousMenuItem = (struct TEXTMENUITEM*)currentMenuItem; 
}

void TextMenuBack(void) {
	currentMenu = (TEXTMENU*)currentMenu->parentMenu;
	selectedMenuItem = currentMenu->firstMenuItem;
	firstVisibleMenuItem = currentMenu->firstMenuItem;
	memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
	TextMenuDraw();
}

void TextMenuDraw(void) {
	VIDEO_CURSOR_POSX=75;
	VIDEO_CURSOR_POSY=125;
	TEXTMENUITEM *item=0l;
	int menucount;
	if (currentMenu==0l) currentMenu = firstMenu;
	if (selectedMenuItem==0l) selectedMenuItem = currentMenu->firstMenuItem;
	if (firstVisibleMenuItem==0l) firstVisibleMenuItem = currentMenu->firstMenuItem;
	
	//Draw the menu title.
	VIDEO_ATTR=0x000000;
	printk("\2%s",currentMenu->szCaption);
	VIDEO_CURSOR_POSY+=30;
	
	//Draw the menu items
	VIDEO_CURSOR_POSX=150;
	item=firstVisibleMenuItem;
	for (menucount=0; menucount<8; menucount++) {
		if (item==0l) {
			//No more menu items to draw
			return;
		}
		//Selected item in red
		if (item == selectedMenuItem) VIDEO_ATTR=0xff0000;
		else VIDEO_ATTR=0xffffff;
		//Font size 2=big.
		printk("\n\2\t%s\n",item->szCaption);
		item=(TEXTMENUITEM *)item->nextMenuItem;
	}
}

int BootTextMenu(void) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	
	//Back up the current framebuffer contents
	textmenusavepage = malloc(FB_SIZE);
	memcpy(textmenusavepage,(void*)FB_START,FB_SIZE);

	//Create the root menu - MANDATORY
	firstMenu = malloc(sizeof(TEXTMENU));
	firstMenu->szCaption="Main Menu\n";
	firstMenu->parentMenu=0l;
	firstMenu->firstMenuItem=0l;
	
	//Add the first Item
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 1";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 2";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 3";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 4";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 5";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 6";	
	TextMenuAddItem(firstMenu, itemPtr);

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 7";	
	TextMenuAddItem(firstMenu, itemPtr);
	
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 8";	
	TextMenuAddItem(firstMenu, itemPtr);
	
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Test 9";	
	TextMenuAddItem(firstMenu, itemPtr);
	
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Child Menu";	
	TextMenuAddItem(firstMenu, itemPtr);

	//Child menu
	menuPtr = (TEXTMENU*)malloc(sizeof(TEXTMENU));
	memset(menuPtr,0x00,sizeof(TEXTMENU));
	menuPtr->szCaption="Test Child Menu";
	menuPtr->parentMenu=(struct TEXTMENU*)firstMenu;
	//itemptr here points to "Test 10", so this child menu is
	//attached to it.
	itemPtr->childMenu = (struct TEXTMENU*)menuPtr;

	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Child Menu Item 1";	
	TextMenuAddItem(menuPtr, itemPtr);
	
	itemPtr = (TEXTMENUITEM*)malloc(sizeof(TEXTMENUITEM));
	memset(itemPtr,0x00,sizeof(TEXTMENUITEM));
	itemPtr->szCaption="Child Menu Item 2";	
	TextMenuAddItem(menuPtr, itemPtr);
	
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
				//8 menu items per page.
				for (i=0; i<7; i++) {
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
			//Redraw the page as it was before the menu was displayed.
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
		else if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1)) {
			//B button takes us back up a menu
			if (currentMenu->parentMenu==0l) {
				//If this is the top level menu, save and quit the text menu.
				memcpy((void*)FB_START,textmenusavepage,FB_SIZE);
				free(textmenusavepage);
				return;
			}
			TextMenuBack();
		}
		
		if (changed) {
			TextMenuDraw();
			changed=0;
		}
	}
}

