

/*


void IntHandler3VsyncC(void)  // video VSYNC
{
	DWORD dwTempInt;
	int i;
	
	if(!nInteruptable) goto endvsyncirq;
	
	BootPciInterruptGlobalStackStateAndDisable(&dwTempInt);
	VIDEO_VSYNC_COUNT++;

	if(VIDEO_VSYNC_COUNT>0) {

		if(fSeenPowerdown) {
			if(VIDEO_LUMASCALING) {
				if(VIDEO_LUMASCALING<8) VIDEO_LUMASCALING=0; else VIDEO_LUMASCALING-=8;
				I2CTransmitWord(0x45, 0xac00|((VIDEO_LUMASCALING)&0xff)); // 8c
			}
			if(VIDEO_RSCALING) {
				if(VIDEO_RSCALING<16) VIDEO_RSCALING=0; else VIDEO_RSCALING-=16;
				I2CTransmitWord(0x45, 0xa800|((VIDEO_RSCALING)&0xff)); // 81
			}
			if(VIDEO_BSCALING) {
				if(VIDEO_BSCALING<8) VIDEO_BSCALING=0; else VIDEO_BSCALING-=8;
				I2CTransmitWord(0x45, 0xaa00|((VIDEO_BSCALING)&0xff)); // 49
			}
		} else {

			if(VIDEO_LUMASCALING<currentvideomodedetails.m_bFinalConexantAC) {
				VIDEO_LUMASCALING+=5;
				I2CTransmitWord(0x45, 0xac00|((VIDEO_LUMASCALING)&0xff)); // 8c
			}
			if(VIDEO_RSCALING<currentvideomodedetails.m_bFinalConexantA8) {
				VIDEO_RSCALING+=3;
				I2CTransmitWord(0x45, 0xa800|((VIDEO_RSCALING)&0xff)); // 81
			}
			if(VIDEO_BSCALING<currentvideomodedetails.m_bFinalConexantAA) {
				VIDEO_BSCALING+=2;
				I2CTransmitWord(0x45, 0xaa00|((VIDEO_BSCALING)&0xff)); // 49
			}
		}

	}

	if(VIDEO_VSYNC_COUNT>20) {
#ifndef DEBUG_MODE
		//DWORD dwOld=VIDEO_VSYNC_POSITION;
#endif
		char c=(VIDEO_VSYNC_COUNT*4)&0xff;
		DWORD dw=c;
		if(c<0) dw=((-(int)c)-1);

		switch(VIDEO_VSYNC_DIR) {
			case 0:
#ifndef DEBUG_MODE
				{
					int nTux=(((VIDEO_VSYNC_POSITION * 0x2fff)/currentvideomodedetails.m_dwWidthInPixels));
					dw=(((VIDEO_VSYNC_POSITION * 192)/currentvideomodedetails.m_dwWidthInPixels)+64)<<24;
					VIDEO_VSYNC_POSITION+=2;
					if(VIDEO_VSYNC_POSITION>=(currentvideomodedetails.m_dwWidthInPixels-64-(currentvideomodedetails.m_dwMarginXInPixelsRecommended*2))) VIDEO_VSYNC_DIR=2;
						// manipulate the tux noise
					aesTux.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[0]=nTux/5;
					aesTux.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[1]=nTux/6;
					aesTux.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[2]=nTux/10;
					aesTux.m_dwComputedFundamentalPhaseIncrementFor48kHzSamples=0x10000 * (((350-(nTux>>7)) <<16)/48000);
					aesTux.m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight=(nTux*2);
						// and some noise in there too
					aenTux.m_paudioelement.m_dwVolumeElementMaster7fff0000Max=0x7f000000;
					aenTux.m_sVolumeZeroIsNone7FFFIsFull=nTux/40;
					aenTux.m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight=(nTux*2);
				}
#endif
				break;
			case 1:
				dw+=64; dw<<=24;
				VIDEO_VSYNC_POSITION-=2;
				if((int)VIDEO_VSYNC_POSITION<=0) VIDEO_VSYNC_DIR=0;
				break;
			case 2:
#ifndef DEBUG_MODE
					// manipulate the tux sound
				aesTux.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[0]=(dw<<4);
				aesTux.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[1]=(dw<<5)/12;
				aesTux.m_saVolumePerHarmonicZeroIsNone7FFFIsFull[2]=(dw<<5)/16;
				aesTux.m_dwComputedFundamentalPhaseIncrementFor48kHzSamples=0x10000 * (((230+dw) <<16)/48000);
					// and some noise in there too
				aenTux.m_paudioelement.m_dwVolumeElementMaster7fff0000Max=0x7fff0000;
				aenTux.m_sVolumeZeroIsNone7FFFIsFull=((128-dw)<<3);
				aenTux.m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight=0x7fff;

				dw+=128; dw<<=24;
#endif
				break;
		}

#ifndef DEBUG_MODE
		
		BootVideoJpegBlitBlend(
			(DWORD *)(FRAMEBUFFER_START+currentvideomodedetails.m_dwWidthInPixels*4*currentvideomodedetails.m_dwMarginYInLinesRecommended+currentvideomodedetails.m_dwMarginXInPixelsRecommended*4+(dwOld<<2)),
			currentvideomodedetails.m_dwWidthInPixels * 4, &jpegBackdrop,
			&dwaTitleArea[dwOld+currentvideomodedetails.m_dwMarginXInPixelsRecommended],
			0x00ff00ff,
			&dwaTitleArea[dwOld+currentvideomodedetails.m_dwMarginXInPixelsRecommended],
			currentvideomodedetails.m_dwWidthInPixels*4,
			4,
			ICON_WIDTH, ICON_HEIGH
		);
	
		BootVideoJpegBlitBlend(
			(DWORD *)(FRAMEBUFFER_START+currentvideomodedetails.m_dwWidthInPixels*4*currentvideomodedetails.m_dwMarginYInLinesRecommended+currentvideomodedetails.m_dwMarginXInPixelsRecommended*4+(VIDEO_VSYNC_POSITION<<2)),
			currentvideomodedetails.m_dwWidthInPixels * 4, &jpegBackdrop,
			(DWORD *)((BYTE *)jpegBackdrop.m_pBitmapData),
			0x00ff00ff | dw,
			&dwaTitleArea[VIDEO_VSYNC_POSITION+currentvideomodedetails.m_dwMarginXInPixelsRecommended],
			currentvideomodedetails.m_dwWidthInPixels*4,
			4,
			ICON_WIDTH, ICON_HEIGH
		);
		
#endif
	}

endvsyncirq:

        i=1000;
	*((volatile DWORD *)0xfd600100)=0x1;  // clear VSYNC int
	while ( ((*((volatile DWORD *)0xfd600100)) & 0x1)) {
		i--;
		if (i==0) break;
		}  // We wait, until the Vsync IRQ has been deleted / or the Timeout kills us
	
	BootPciInterruptGlobalPopState(dwTempInt);
} 


*/
