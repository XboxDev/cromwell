/*

int BootMenue(CONFIGENTRY *config,int nDrive,int nActivePartition, int nFATXPresent){


	int nIcon;
	int nTempCursorResumeX, nTempCursorResumeY, nTempStartMessageCursorX, nTempStartMessageCursorY;
	int nTempCursorX, nTempCursorY, nTempEntryX, nTempEntryY;
	int nModeDependentOffset=(currentvideomodedetails.m_dwWidthInPixels-640)/2;  // icon offsets computed for 640 modes, retain centering in other modes
	int nShowSelect = false;
	int selected=-1;

	
	#define DELAY_TICKS 72
	#define TRANPARENTNESS 0x30
	#define OPAQUENESS 0xc0
	#define SELECTED 0xff


	
	nTempCursorResumeX=nTempCursorMbrX;
	nTempCursorResumeY=nTempCursorMbrY;
	
	nTempEntryX=VIDEO_CURSOR_POSX;
	nTempEntryY=VIDEO_CURSOR_POSY;
	
	nTempCursorX=VIDEO_CURSOR_POSX;
	nTempCursorY=currentvideomodedetails.m_dwHeightInLines-80;
	
	VIDEO_CURSOR_POSX=((215+nModeDependentOffset)<<2);
	VIDEO_CURSOR_POSY=nTempCursorY-100;
	nTempStartMessageCursorX=VIDEO_CURSOR_POSX;
	nTempStartMessageCursorY=VIDEO_CURSOR_POSY;
	VIDEO_ATTR=0xffc8c8c8;
	printk("Close DVD tray to select\n");
	VIDEO_ATTR=0xffffffff;
	
	BootIcons(nModeDependentOffset, nTempCursorY, nModeDependentOffset, nTempCursorY);
	
	
	for(nIcon = 0; nIcon < ICONCOUNT;nIcon ++) {
		BootStartBiosDoIcon(&icon[nIcon], TRANPARENTNESS);
	}
	
	
	
	for(nIcon = 0; nIcon < ICONCOUNT;nIcon ++) {
		traystate = ETS_OPEN_OR_OPENING;
		nShowSelect = false;
		VIDEO_CURSOR_POSX=icon[nIcon].nTextX;
		VIDEO_CURSOR_POSY=icon[nIcon].nTextY;
	          
		switch(nIcon){
	
			case ICON_FATX:
				if(nFATXPresent) {
					strcpy(config->szKernel, "/vmlinuz"); // fatx default kernel, looked for to detect fs
					if(!BootTryLoadConfigFATX(config)) continue;  // only bother with it if the filesystem exists
					printk("/linuxboot.cfg from FATX\n");
					nShowSelect = true;
				} else {
					continue;
				}
				break;
	
			case ICON_NATIVE:
				if(nDrive != 1) {
					strcpy(config->szKernel, "/boot/vmlinuz");  // Ext2 default kernel, looked for to detect fs
					if(!BootLodaConfigNative(nActivePartition, config, true)) continue; // only bother with it if the filesystem exists
					printk("/dev/hda\n");
					nShowSelect=true;
				} else {
					continue;
				}
				break;
	
			case ICON_CD:
				printk("/dev/hdb\n");
				nShowSelect = true;  // always might want to boot from CD
				break;
	
			case ICON_SETUP:
				printk("Flash\n");
				#ifdef FLASH
				nShowSelect = true;
				#else
				continue;
				#endif
				break;
				
		}
	
	
	
		if(nShowSelect) {
	
			DWORD dwTick=BIOS_TICK_COUNT;
	       		

			while(
				(BIOS_TICK_COUNT<(dwTick+DELAY_TICKS)) &&
				(traystate==ETS_OPEN_OR_OPENING)
			) {
				BootStartBiosDoIcon(
					&icon[nIcon], OPAQUENESS-((OPAQUENESS-TRANPARENTNESS)
					 *(BIOS_TICK_COUNT-dwTick))/DELAY_TICKS );
			}
			dwTick=BIOS_TICK_COUNT;
	
	
			if(traystate!=ETS_OPEN_OR_OPENING) {  // tray went in, specific choice made
			
				VIDEO_CURSOR_POSX=icon[nIcon].nTextX;
				VIDEO_CURSOR_POSY=icon[nIcon].nTextY;
				RecoverMbrArea();
				BootStartBiosDoIcon(&icon[nIcon], SELECTED);
				selected = nIcon;
				break;
			}
	
				// timeout
	
			BootStartBiosDoIcon(&icon[nIcon], TRANPARENTNESS);
		}
	
		BootVideoClearScreen(&jpegBackdrop, nTempCursorY, VIDEO_CURSOR_POSY+1);
	}
	
	
	BootVideoClearScreen(&jpegBackdrop, nTempCursorResumeY, nTempCursorResumeY+100);
	BootVideoClearScreen(&jpegBackdrop, nTempStartMessageCursorY, nTempCursorY+16);
	
	VIDEO_CURSOR_POSX=nTempCursorResumeX;
	VIDEO_CURSOR_POSY=nTempCursorResumeY;
        
      
        
        // We return the selected Menue , -1 if nothing selected
	return selected;
	
}

 */