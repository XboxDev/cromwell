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
#include <filesys.h>
#include "rc4.h"
#include "sha1.h"
#include "BootFATX.h"
#include "xbox.h"
#include "BootFlash.h"
#include "cpu.h"
#include "BootIde.h"
#include "MenuActions.h"
#include "config.h"

#define ICON_SOURCE_SLOT0 0
#define ICON_SOURCE_SLOT1 ICON_WIDTH
#define ICON_SOURCE_SLOT2 ICON_WIDTH*2
#define ICON_SOURCE_SLOT3 ICON_WIDTH*3
#define ICON_SOURCE_SLOT4 ICON_WIDTH*4
#define ICON_SOURCE_SLOT5 ICON_WIDTH*5
#define ICON_SOURCE_SLOT6 ICON_WIDTH*6
#define ICON_SOURCE_SLOT7 ICON_WIDTH*7
#define ICON_SOURCE_SLOT8 ICON_WIDTH*8

#define TRANSPARENTNESS 0x30
#define SELECTED 0xff

struct ICON;

typedef struct {
	int iconSlot;
	int nTextX;
	int nTextY;
	char *szCaption;
	void (*functionPtr) (void);
	struct ICON *previousIcon;
	struct ICON *nextIcon;
} ICON;

ICON *firstIcon;
ICON *selectedIcon;
ICON *firstVisibleIcon;

void BootIcons(int nTextOffsetX, int nTextOffsetY) {
	//Root node
	firstIcon = (ICON*)malloc(sizeof(ICON));

	ICON* iconPtr = firstIcon;
	iconPtr->previousIcon=0l;

	//FATX icon
	iconPtr->iconSlot = ICON_SOURCE_SLOT4;
	iconPtr->nTextX = (nTextOffsetX+118)<<2;;
	iconPtr->nTextY = nTextOffsetY;
	iconPtr->szCaption = "FatX (E:)";
	iconPtr->functionPtr = BootFromFATX;
	
	//Native icon
	iconPtr->nextIcon = malloc(sizeof(ICON));
	iconPtr=(ICON *)iconPtr->nextIcon;
	iconPtr->iconSlot = ICON_SOURCE_SLOT1;
	iconPtr->nTextX = (nTextOffsetX+230)<<2;;
	iconPtr->nTextY = nTextOffsetY;
	iconPtr->szCaption = "HDD";
	iconPtr->functionPtr = BootFromNative;
	
	//CD icon
	iconPtr->nextIcon = malloc(sizeof(ICON));
	iconPtr=(ICON *)iconPtr->nextIcon;
	iconPtr->iconSlot = ICON_SOURCE_SLOT2;
	iconPtr->nTextX = (nTextOffsetX+340)<<2;
	iconPtr->nTextY = nTextOffsetY;
	iconPtr->szCaption = "CD-ROM";
	iconPtr->functionPtr = BootFromCD;
	iconPtr->nextIcon=0l;

#ifdef ETHERBOOT
	//Etherboot icon
	iconPtr->nextIcon = malloc(sizeof(ICON));
	iconPtr=(ICON *)iconPtr->nextIcon;
	iconPtr->iconSlot = ICON_SOURCE_SLOT3;
	iconPtr->nTextX = (nTextOffsetX+451)<<2;
	iconPtr->nTextY = nTextOffsetY;
	iconPtr->szCaption = "Etherboot";
	iconPtr->functionPtr = BootFromEtherboot;
	iconPtr->nextIcon=0l;
#endif	
	//Work through the list to set up the 'previous icon' pointers.
	{
		ICON *previousIcon, *nextIcon;
		previousIcon = firstIcon;
		
		while (nextIcon = (ICON *)previousIcon->nextIcon) {
			nextIcon->previousIcon = (struct ICON*)previousIcon;
			previousIcon = nextIcon;
		}
	}
}

void IconMenuDraw(int nXOffset, int nYOffset, int nTextOffsetX, int nTextOffsetY) {
	ICON *iconPtr = firstVisibleIcon;
	int iconcount;
	//There are four 'bays' for displaying icons in - we only draw the four.
	for (iconcount=0; iconcount<4; iconcount++) {
		if (iconPtr==0l) {
			//No more icons to draw
			return;
		}
		BYTE opaqueness;
		if (iconPtr==selectedIcon) {
			//Selected icon has less transparency
			//and has a caption drawn underneath it
			opaqueness = SELECTED;
			VIDEO_CURSOR_POSX=iconPtr->nTextX;
			VIDEO_CURSOR_POSY=iconPtr->nTextY;
			printk("%s\n",iconPtr->szCaption);
		}
		else opaqueness = TRANSPARENTNESS;
		
		BootVideoJpegBlitBlend(
			(BYTE *)(FB_START+((vmode.width * (nYOffset-74))+nXOffset+(112*(iconcount+1))) * 4),
			vmode.width, // dest bytes per line
			&jpegBackdrop, // source jpeg object
			(BYTE *)(jpegBackdrop.pData+(iconPtr->iconSlot * jpegBackdrop.bpp)),
			0xff00ff|(((DWORD)opaqueness)<<24),
			(BYTE *)(jpegBackdrop.pBackdrop + ((jpegBackdrop.width * (nYOffset-74)) + nXOffset+(112*(iconcount+1))) * jpegBackdrop.bpp),
			ICON_WIDTH, ICON_HEIGHT
		);
		iconPtr = (ICON *)iconPtr->nextIcon;
	}
}

int BootMenu(CONFIGENTRY *config,int nDrive,int nActivePartition, int nFATXPresent){
	
	extern int nTempCursorMbrX, nTempCursorMbrY;
	int old_nIcon = 0;
	int nSelected = -1;
	unsigned int menu=0;
	int change=0;

	int nTempCursorResumeX, nTempCursorResumeY ;
	int nTempCursorX, nTempCursorY;
	int nModeDependentOffset=(vmode.width-640)/2;  // icon offsets computed for 640 modes, retain centering in other modes
	int nShowSelect = false;
        unsigned char *videosavepage;
        
        DWORD COUNT_start;
        DWORD HH;
        DWORD temp=1;
        
	nTempCursorResumeX=nTempCursorMbrX;
	nTempCursorResumeY=nTempCursorMbrY;

	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=vmode.height-80;
	
	// We save the complete Video Page to a memory (we restore at exit)
	videosavepage = malloc(FB_SIZE);
	memcpy(videosavepage,(void*)FB_START,FB_SIZE);
	
	VIDEO_CURSOR_POSX=((252+nModeDependentOffset)<<2);
	VIDEO_CURSOR_POSY=nTempCursorY-100;
	
	VIDEO_ATTR=0xffc8c8c8;
	printk("Select from Menu\n");
	VIDEO_ATTR=0xffffffff;
	
	BootIcons(nModeDependentOffset, nTempCursorY);
	//For now, mark the first icon as selected.
	selectedIcon = firstIcon;
	firstVisibleIcon = firstIcon;
	IconMenuDraw(nModeDependentOffset, nTempCursorY, nModeDependentOffset, nTempCursorY);
	COUNT_start = IoInputDword(0x8008);

	//Main menu event loop.
	while(1)
	{
		int changed=0;
		USBGetEvents();
		
		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_RIGHT) == 1)
		{
			if (selectedIcon->nextIcon!=0l) {
				//A bit ugly, but need to find the last visible icon, and see if 
				//we are moving further right from it.
				ICON *lastVisibleIcon=firstVisibleIcon;
				int i=0;
				for (i=0; i<3; i++) {
					if (lastVisibleIcon->nextIcon==0l) break;
					lastVisibleIcon = (ICON *)lastVisibleIcon->nextIcon;
				}
				if (selectedIcon == lastVisibleIcon) { 
					//We are moving further right, so slide all the icons along. 
					firstVisibleIcon = (ICON *)firstVisibleIcon->nextIcon;	
					//As all the icons have moved, we need to refresh the entire page.
					memcpy((void*)FB_START,videosavepage,FB_SIZE);
				}
				selectedIcon = (ICON *)selectedIcon->nextIcon;
				changed=1;
			}
			temp=0;
		}
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_LEFT) == 1)
		{
			if (selectedIcon->previousIcon!=0l) {
				if (selectedIcon == firstVisibleIcon) {
					//We are moving further left, so slide all the icons along. 
					firstVisibleIcon = (ICON*)selectedIcon->previousIcon;
					//As all the icons have moved, we need to refresh the entire page.
					memcpy((void*)FB_START,videosavepage,FB_SIZE);
				}
				selectedIcon = (ICON *)selectedIcon->previousIcon;
				changed=1;
			}
			temp=0;
		}
		//If anybody has toggled the xpad left/right, disable the timeout.
		if (temp!=0) {
			HH = IoInputDword(0x8008);
			temp = HH-COUNT_start;
		}
		
		if ((risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1) || (DWORD)(temp>(0x369E99*BOOT_TIMEWAIT))) {
			memcpy((void*)FB_START,videosavepage,FB_SIZE);
			free(videosavepage);
			
			VIDEO_CURSOR_POSX=nTempCursorResumeX;
			VIDEO_CURSOR_POSY=nTempCursorResumeY;
			//Icon selected - invoke function pointer.
			selectedIcon->functionPtr();
			//Should never come back but at least if we do, the menu can
			//continue to work.
		}
		if (changed) {
			BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);
			IconMenuDraw(nModeDependentOffset, nTempCursorY, nModeDependentOffset, nTempCursorY);
			changed=0;
		}
	}
}

