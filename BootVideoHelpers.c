
#include  "boot.h"
#include "string.h"
#include "font/fontx16.h"  // brings in font struct

// These are helper functions for displaying bitmap video
// includes an antialiased (4bpp) proportional bitmap font (n x 16 pixel)

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// 2002-09-10  agreen@warmcat.com  created

#define WIDTH_SPACE_PIXELS 5

// returns number of x pixels taken up by ascii character bCharacter

unsigned int BootVideoGetCharacterWidth(BYTE bCharacter)
{
	unsigned int nStart, nWidth;

		// we only have glyphs for 0x21 through 0x7e inclusive

	if(bCharacter<0x21) return WIDTH_SPACE_PIXELS;
	if(bCharacter>0x7e) return WIDTH_SPACE_PIXELS;

	nStart=waStarts[bCharacter-0x21];
	nWidth=waStarts[bCharacter-0x20]-nStart;

	return nWidth;
}

// returns number of x pixels taken up by string

unsigned int BootVideoGetStringTotalWidth(const char * szc) {
	unsigned int nWidth=0;
	while(*szc) { nWidth+=BootVideoGetCharacterWidth(*szc++); }
	return nWidth;
}

// convert pixel count to size of memory in bytes required to hold it, given the character height

unsigned int BootVideoFontWidthToBitmapBytecount(unsigned int uiWidth)
{
	return (uiWidth << 2) * uiPixelsY;
}

// 2D memcpy

void BootVideoBlit(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	DWORD * pdwTopLeftSource,
	DWORD dwCountBytesPerLineSource,
	DWORD dwCountLines
) {
	while(dwCountLines--) {
		memcpy(pdwTopLeftDestination, pdwTopLeftSource, dwCountBytesPerLineSource);
		pdwTopLeftDestination+=dwCountBytesPerLineDestination>>2;
		pdwTopLeftSource+=dwCountBytesPerLineSource>>2;
	}
}

// usable for direct write or for prebuffered write
// returns width of character in pixels
// RGBA .. full-on RED is opaque --> 0xFF0000FF <-- red

int BootVideoOverlayCharacter(
	DWORD * pdwaTopLeftDestination,
	DWORD m_dwCountBytesPerLineDestination,
	RGBA rgbaColourAndOpaqueness,
	BYTE bCharacter
) {
	unsigned int n, nStart, nWidth, y,
		nOpaquenessMultiplied,
		nTransparentnessMultiplied
	;
	BYTE b=0, b1, *pbColour=(BYTE *)&rgbaColourAndOpaqueness;
	BYTE * pbaDestStart;

		// we only have glyphs for 0x21 through 0x7e inclusive

	if(bCharacter<'!') return 5;
	if(bCharacter>'~') return 5;

	nStart=waStarts[bCharacter-(' '+1)];
	nWidth=waStarts[bCharacter-' ']-nStart;

	pbaDestStart=((BYTE *)pdwaTopLeftDestination);

	for(y=0;y<uiPixelsY;y++) {
		BYTE * pbaDest=pbaDestStart;

		for(n=0;n<nWidth;n++) {
			if((n&1)==0) {
				b=baCharset[nStart+(n>>1)];
				b1=b>>4;
			} else {
				b1=b&0x0f;
			}

			nOpaquenessMultiplied=(((int)(unsigned int)(pbColour[3]))*(int)b1)/15;
			nTransparentnessMultiplied=(0xff-nOpaquenessMultiplied);

			*pbaDest=(BYTE)(((nOpaquenessMultiplied * (unsigned int)pbColour[0]) + (nTransparentnessMultiplied * (unsigned int)*pbaDest))/255); pbaDest++;
			*pbaDest=(BYTE)(((nOpaquenessMultiplied * (unsigned int)pbColour[1]) + (nTransparentnessMultiplied * (unsigned int)*pbaDest))/255); pbaDest++;
			*pbaDest=(BYTE)(((nOpaquenessMultiplied * (unsigned int)pbColour[2]) + (nTransparentnessMultiplied * (unsigned int)*pbaDest))/255); pbaDest++;
			*pbaDest++=0xff;

		}
		nStart+=(uiPixelsX+1)/2;
		pbaDestStart+=m_dwCountBytesPerLineDestination;
	}

	return nWidth;
}

// usable for direct write or for prebuffered write
// returns width of string in pixels

int BootVideoOverlayString(DWORD * pdwaTopLeftDestination, DWORD m_dwCountBytesPerLineDestination, BYTE bOpaqueness, const char * szString)
{
	unsigned int uiWidth=0;
	while(*szString) {
		uiWidth+=BootVideoOverlayCharacter(pdwaTopLeftDestination, m_dwCountBytesPerLineDestination, bOpaqueness, *szString);
		szString++;
	}
	return uiWidth;
}


