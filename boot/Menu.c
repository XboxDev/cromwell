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

enum {
	ICON_FATX = 0,
	ICON_NATIVE,
	ICON_CD,
#ifdef ETHERBOOT
	ICON_ETHERBOOT,
#endif
	ICONCOUNT // always last
};

/* Need to turn this into a linked list */
//and enumerate icons.
typedef struct {
	int nDestX;
	int nDestY;
	int nSrcX;
	int nSrcLength;
	int nSrcHeight;
	int nTextX;
	int nTextY;
	int nEnabled;
	int nSelected;
	char *szCaption;
	void (*functionPtr) (void);
} ICON;

ICON icon[ICONCOUNT];

void BootIcons(int nXOffset, int nYOffset, int nTextOffsetX, int nTextOffsetY) {
	memset(icon,0,sizeof(ICON) * ICONCOUNT);
	icon[ICON_FATX].nDestX = nXOffset + 120;
	icon[ICON_FATX].nDestY = nYOffset - 74;
	icon[ICON_FATX].nSrcX = ICON_WIDTH*4;
	icon[ICON_FATX].nSrcLength = ICON_WIDTH;
	icon[ICON_FATX].nSrcHeight = ICON_HEIGH;
	icon[ICON_FATX].nTextX = (nTextOffsetX+118)<<2;;
	icon[ICON_FATX].nTextY = nTextOffsetY;
	icon[ICON_FATX].szCaption = "FatX (E:)";
	icon[ICON_FATX].functionPtr = BootFromFATX;
	
	icon[ICON_NATIVE].nDestX = nXOffset + 232;
	icon[ICON_NATIVE].nDestY = nYOffset - 74;
	icon[ICON_NATIVE].nSrcX = ICON_WIDTH;
	icon[ICON_NATIVE].nSrcLength = ICON_WIDTH;
	icon[ICON_NATIVE].nSrcHeight = ICON_HEIGH;
	icon[ICON_NATIVE].nTextX = (nTextOffsetX+230)<<2;;
	icon[ICON_NATIVE].nTextY = nTextOffsetY;
	icon[ICON_NATIVE].szCaption = "HDD";
	icon[ICON_NATIVE].functionPtr = BootFromNative;
	
	icon[ICON_CD].nDestX = nXOffset + 344;
	icon[ICON_CD].nDestY = nYOffset - 74;
	icon[ICON_CD].nSrcX = ICON_WIDTH*2;
	icon[ICON_CD].nSrcLength = ICON_WIDTH;
	icon[ICON_CD].nSrcHeight = ICON_HEIGH;
	icon[ICON_CD].nTextX = (nTextOffsetX+340)<<2;
	icon[ICON_CD].nTextY = nTextOffsetY;
	icon[ICON_CD].szCaption = "CD-ROM";
	icon[ICON_CD].functionPtr = BootFromCD;

#ifdef ETHERBOOT
	icon[ICON_ETHERBOOT].nDestX = nXOffset + 456;
	icon[ICON_ETHERBOOT].nDestY = nYOffset - 74;
	icon[ICON_ETHERBOOT].nSrcX = ICON_WIDTH*3;
	icon[ICON_ETHERBOOT].nSrcLength = ICON_WIDTH;
	icon[ICON_ETHERBOOT].nSrcHeight = ICON_HEIGH;
	icon[ICON_ETHERBOOT].nTextX = (nTextOffsetX+451)<<2;
	icon[ICON_ETHERBOOT].nTextY = nTextOffsetY;
	icon[ICON_ETHERBOOT].szCaption = "Etherboot";
	icon[ICON_ETHERBOOT].functionPtr = 0l;
#endif	
}

void IconMenuDrawIcon(ICON *icon, BYTE bOpaqueness)
{
	BootVideoJpegBlitBlend(
		(BYTE *)(FB_START+((vmode.width * icon->nDestY)+icon->nDestX) * 4),
		vmode.width, // dest bytes per line
		&jpegBackdrop, // source jpeg object
		(BYTE *)(jpegBackdrop.pData+(icon->nSrcX * jpegBackdrop.bpp)),
		0xff00ff|(((DWORD)bOpaqueness)<<24),
		(BYTE *)(jpegBackdrop.pBackdrop + ((jpegBackdrop.width * icon->nDestY) + icon->nDestX) * jpegBackdrop.bpp),
		icon->nSrcLength, 
		icon->nSrcHeight
	);
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
        
	#define TRANPARENTNESS 0x30
	#define SELECTED 0xff

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
	
	BootIcons(nModeDependentOffset, nTempCursorY, nModeDependentOffset, nTempCursorY);
	
	// Display The Icons
	for(menu = 0; menu < ICONCOUNT;menu ++) {
		IconMenuDrawIcon(&icon[menu], TRANPARENTNESS);
	}
	
	// Look which Icons are enabled or disabled	
        for(menu = 0; menu < ICONCOUNT;menu ++) {
        
		icon[menu].nEnabled = 0;		
        	switch(menu){
	
			case ICON_FATX:
				if(nFATXPresent) {
					if(BootTryLoadConfigFATX(config)){
						icon[menu].nEnabled = 1;
						if(nSelected == -1) nSelected = menu;
					}
				}
				break;
	
			case ICON_NATIVE:
				if(nDrive != 1) {
					if(BootLoadConfigNative(nActivePartition, config, true)) {
						icon[menu].nEnabled = 1;
						if(nSelected == -1) nSelected = menu;
					}
				}
				break;
	
			case ICON_CD:
				//Check we have a cdrom attached
				if(tsaHarddiskInfo[0].m_fAtapi || tsaHarddiskInfo[1].m_fAtapi) {
					icon[menu].nEnabled = 1;
					if(nSelected == -1) nSelected = menu;
				}
				break;
#ifdef ETHERBOOT	
			case ICON_ETHERBOOT:
				icon[menu].nEnabled = 1;
				if(nSelected == -1) nSelected = menu;
				break;
#endif
		}
	}	
        
        // Initial Selected Icon
        menu = nSelected;
        old_nIcon = nSelected;
	icon[menu].nSelected = 1;
	change = 1;
	COUNT_start = IoInputDword(0x8008);

	//Main menu event loop.
	while(1)
	{
		int n;
		USBGetEvents();
		
		if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_LEFT) == 1)
		{
			icon[menu].nSelected = 0;
			if (menu>0) menu--;
			while (menu>0 && !icon[menu].nEnabled) 
				menu--;
			/* If there are no more enabled icons this way,
			 * leave the currently enabled icon enabled.
			 * Hence, no change */
			if (!icon[menu].nEnabled) menu = old_nIcon;
			else change = 1;
			icon[menu].nSelected = 1;
			temp=0;
		}
		else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_RIGHT) == 1)
		{
			icon[menu].nSelected = 0;
			if (menu<ICONCOUNT-1) menu++;		
			while ((menu<ICONCOUNT-1) && !icon[menu].nEnabled) 
				menu++;
			/* If there are no more enabled icons this way,
			 * leave the currently enabled icon enabled.
			 * Hence, no change */
			if (!icon[menu].nEnabled) menu = old_nIcon;
			else change = 1;
			icon[menu].nSelected = 1;
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
			if (icon[menu].functionPtr) icon[menu].functionPtr();
			//Should never come back but at least if we do, the menu can
			//continue to work.
		}
		
		if (change) 
		{
		        BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);
			IconMenuDrawIcon(&icon[old_nIcon], TRANPARENTNESS);
			old_nIcon = menu;
			IconMenuDrawIcon(&icon[menu], SELECTED);

                        VIDEO_CURSOR_POSX=icon[menu].nTextX;
			VIDEO_CURSOR_POSY=icon[menu].nTextY;
			
			printk("%s\n",icon[menu].szCaption);
		}

		change=0;	    
	}

}

