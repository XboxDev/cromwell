/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "TextMenu.h"

void TextMenuDraw(void);
void TextMenuBack(void);

TEXTMENUITEM *firstVisibleMenuItem=NULL;
TEXTMENUITEM *selectedMenuItem=NULL;
TEXTMENU *firstMenu=NULL;
TEXTMENU *currentMenu=NULL;
		
unsigned char *textmenusavepage;

void TextMenuAddItem(TEXTMENU *menu, TEXTMENUITEM *newMenuItem) {
	TEXTMENUITEM *menuItem = menu->firstMenuItem;
	TEXTMENUITEM *currentMenuItem=NULL;
	
	while (menuItem != NULL) {
		currentMenuItem = menuItem;
		menuItem = (TEXTMENUITEM*)menuItem->nextMenuItem;
	}
	
	if (currentMenuItem==NULL) { 
		//This is the first icon in the chain
		menu->firstMenuItem = newMenuItem;
	}
	//Append to the end of the chain
	else currentMenuItem->nextMenuItem = (struct TEXTMENUITEM*)newMenuItem;
	newMenuItem->nextMenuItem = NULL;
	newMenuItem->previousMenuItem = (struct TEXTMENUITEM*)currentMenuItem; 
}

void TextMenuBack(void) {
	currentMenu = (TEXTMENU*)currentMenu->parentMenu;
	selectedMenuItem = currentMenu->firstMenuItem;
	firstVisibleMenuItem = currentMenu->firstMenuItem;
	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
	TextMenuDraw();
}

void TextMenuDraw(void) {
	TEXTMENUITEM *item=NULL;
	int menucount;
	
	VIDEO_CURSOR_POSX=75;
	VIDEO_CURSOR_POSY=125;
	
	if (currentMenu==NULL) currentMenu = firstMenu;
	if (selectedMenuItem==NULL) selectedMenuItem = currentMenu->firstMenuItem;
	if (firstVisibleMenuItem==NULL) firstVisibleMenuItem = currentMenu->firstMenuItem;
	
	//Draw the menu title.
	VIDEO_ATTR=0x000000;
	printk("\2          %s",currentMenu->szCaption);
	VIDEO_CURSOR_POSY+=30;
	
	//Draw the menu items
	VIDEO_CURSOR_POSX=150;
	item=firstVisibleMenuItem;
	for (menucount=0; menucount<8; menucount++) {
		if (item==NULL) {
			//No more menu items to draw
			return;
		}
		//Selected item in red
		if (item == selectedMenuItem) VIDEO_ATTR=0xff0000;
		else VIDEO_ATTR=0xffffff;
		//Font size 2=big.
		printk("\n\2               %s\n",item->szCaption);
		item=(TEXTMENUITEM *)item->nextMenuItem;
	}
	VIDEO_ATTR=0xffffff;
}

void TextMenu(void) {
	TEXTMENUITEM *itemPtr;
	TEXTMENU *menuPtr;
	
	//Back up the current framebuffer contents - restore at exit.
	textmenusavepage = malloc(FB_SIZE);
	memcpy(textmenusavepage,(void*)FB_START,FB_SIZE);

	BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
	
	TextMenuDraw();
	
	//Main menu event loop.
	while(1)
	{
		int changed=0;
		wait_ms(75);

		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_UP) == 1)
		{
			if (selectedMenuItem->previousMenuItem!=NULL) {
				if (selectedMenuItem == firstVisibleMenuItem) {
					firstVisibleMenuItem = (TEXTMENUITEM *)selectedMenuItem->previousMenuItem;
					BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
				}
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->previousMenuItem;
				changed=1;
			}
		} 
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_DOWN) == 1) {
			if (selectedMenuItem->nextMenuItem!=NULL) {
				TEXTMENUITEM *lastVisibleMenuItem = firstVisibleMenuItem;
				int i=0;
				//8 menu items per page.
				for (i=0; i<7; i++) {
					if (lastVisibleMenuItem->nextMenuItem==NULL) break;
					lastVisibleMenuItem = (TEXTMENUITEM *)lastVisibleMenuItem->nextMenuItem;
				}
				if (selectedMenuItem == lastVisibleMenuItem) {
					firstVisibleMenuItem = (TEXTMENUITEM *)firstVisibleMenuItem->nextMenuItem;
					BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
				}
				selectedMenuItem=(TEXTMENUITEM*)selectedMenuItem->nextMenuItem;
				changed=1;
			}
		}
			
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1 || risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_START) == 1) {
			BootVideoClearScreen(&jpegBackdrop, 0, 0xffff);
			VIDEO_ATTR=0xffffff;
			
			//Menu item selected - invoke function pointer.
			if (selectedMenuItem->functionPtr!=NULL) selectedMenuItem->functionPtr(selectedMenuItem->functionDataPtr);
			//Display the childmenu, if this menu item has one.	
			if (selectedMenuItem->childMenu!=NULL) {
				currentMenu = (TEXTMENU*)selectedMenuItem->childMenu;
				selectedMenuItem = currentMenu->firstMenuItem;
				firstVisibleMenuItem = currentMenu->firstMenuItem;
			}
			changed=1;
		}
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1 || risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_BACK) == 1) {
			//B or Back button takes us back up a menu
			if (currentMenu->parentMenu==NULL) {
				//If this is the top level menu, replace the original framebuffer contents and quit the text menu.
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

