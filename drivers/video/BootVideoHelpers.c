/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// 2002-09-10  agreen@warmcat.com  created

// These are helper functions for displaying bitmap video
// includes an antialiased (4bpp) proportional bitmap font (n x 16 pixel)


#include  "boot.h"
#include "video.h"
#include "memory_layout.h"
//#include "string.h"
#include "fontx16.h"  // brings in font struct
#include <stdarg.h>
#include "decode-jpg.h"
#define WIDTH_SPACE_PIXELS 5

// returns number of x pixels taken up by ascii character bCharacter

unsigned int BootVideoGetCharacterWidth(BYTE bCharacter, bool fDouble)
{
	unsigned int nStart, nWidth;
	int nSpace=WIDTH_SPACE_PIXELS;
	
	if(fDouble) nSpace=8;

		// we only have glyphs for 0x21 through 0x7e inclusive

	if(bCharacter<0x21) return nSpace;
	if(bCharacter>0x7e) return nSpace;

	nStart=waStarts[bCharacter-0x21];
	nWidth=waStarts[bCharacter-0x20]-nStart;

	if(fDouble) return nWidth<<1; else return nWidth;
}

// returns number of x pixels taken up by string

unsigned int BootVideoGetStringTotalWidth(const char * szc) {
	unsigned int nWidth=0;
	bool fDouble=false;
	while(*szc) {
		if(*szc=='\2') {
			fDouble=!fDouble;
			szc++;
		} else {
			nWidth+=BootVideoGetCharacterWidth(*szc++, fDouble);
		}
	}
	return nWidth;
}

// convert pixel count to size of memory in bytes required to hold it, given the character height

unsigned int BootVideoFontWidthToBitmapBytecount(unsigned int uiWidth)
{
	return (uiWidth << 2) * uiPixelsY;
}

void BootVideoJpegBlitBlend(
	BYTE *pDst,
	DWORD dst_width,
	JPEG * pJpeg,
	BYTE *pFront,
	RGBA m_rgbaTransparent,
	BYTE *pBack,
	int x,
	int y
) {
	int n=0;

	int nTransAsByte=m_rgbaTransparent>>24;
	int nBackTransAsByte=255-nTransAsByte;
	DWORD dw;

	m_rgbaTransparent|=0xff000000;
	m_rgbaTransparent&=0xffc0c0c0;

	while(y--) {

		for(n=0;n<x;n++) {
			
			dw = ((*((DWORD *)pFront))|0xff000000)&0xffc0c0c0;

			if(dw!=m_rgbaTransparent) {
				pDst[2]=((pFront[0]*nTransAsByte)+(pBack[0]*nBackTransAsByte))>>8;
				pDst[1]=((pFront[1]*nTransAsByte)+(pBack[1]*nBackTransAsByte))>>8;
				pDst[0]=((pFront[2]*nTransAsByte)+(pBack[2]*nBackTransAsByte))>>8;
			}
			pDst+=4;
			pFront+=pJpeg->bpp;
			pBack+=pJpeg->bpp;
		}
		pBack+=(pJpeg->width*pJpeg->bpp) -(x * pJpeg->bpp);
		pDst+=(dst_width * 4) - (x * 4);
		pFront+=(pJpeg->width*pJpeg->bpp) -(x * pJpeg->bpp);
	}
}

// usable for direct write or for prebuffered write
// returns width of character in pixels
// RGBA .. full-on RED is opaque --> 0xFF0000FF <-- red

int BootVideoOverlayCharacter(
	DWORD * pdwaTopLeftDestination,
	DWORD m_dwCountBytesPerLineDestination,
	RGBA rgbaColourAndOpaqueness,
	BYTE bCharacter,
	bool fDouble
) {
	int nSpace;
	unsigned int n, nStart, nWidth, y, nHeight
//		nOpaquenessMultiplied,
//		nTransparentnessMultiplied
	;
	BYTE b=0, b1; // *pbColour=(BYTE *)&rgbaColourAndOpaqueness;
	BYTE * pbaDestStart;

		// we only have glyphs for 0x21 through 0x7e inclusive

	if(bCharacter=='\t') {
		DWORD dw=((DWORD)pdwaTopLeftDestination) % m_dwCountBytesPerLineDestination;
		DWORD dw1=((dw+1)%(32<<2));  // distance from previous boundary
		return ((32<<2)-dw1)>>2;
	}
	nSpace=WIDTH_SPACE_PIXELS;
	if(fDouble) nSpace=8;
	if(bCharacter<'!') return nSpace;
	if(bCharacter>'~') return nSpace;

	nStart=waStarts[bCharacter-(' '+1)];
	nWidth=waStarts[bCharacter-' ']-nStart;
	nHeight=uiPixelsY;

	if(fDouble) { nWidth<<=1; nHeight<<=1; }

//	nStart=0;
//	nWidth=300;

	pbaDestStart=((BYTE *)pdwaTopLeftDestination);

	for(y=0;y<nHeight;y++) {
		BYTE * pbaDest=pbaDestStart;
		int n1=nStart;

		for(n=0;n<nWidth;n++) {
			b=baCharset[n1>>1];
			if(!(n1&1)) {
				b1=b>>4;
			} else {
				b1=b&0x0f;
			}
			if(fDouble) {
				if(n & 1) n1++;
			} else {
				n1++;
			}

		if(b1) {
				*pbaDest=(BYTE)((b1*(rgbaColourAndOpaqueness&0xff))>>4); pbaDest++;
				*pbaDest=(BYTE)((b1*((rgbaColourAndOpaqueness>>8)&0xff))>>4); pbaDest++;
				*pbaDest=(BYTE)((b1*((rgbaColourAndOpaqueness>>16)&0xff))>>4); pbaDest++;
				*pbaDest++=0xff;
			} else {
				pbaDest+=4;
			}
		}
		if(fDouble) {
			if(y&1) nStart+=uiPixelsX;
		} else {
			nStart+=uiPixelsX;
		}
		pbaDestStart+=m_dwCountBytesPerLineDestination;
	}

	return nWidth;
}

// usable for direct write or for prebuffered write
// returns width of string in pixels

int BootVideoOverlayString(DWORD * pdwaTopLeftDestination, DWORD m_dwCountBytesPerLineDestination, RGBA rgbaOpaqueness, const char * szString)
{
	unsigned int uiWidth=0;
	bool fDouble=0;
	while((*szString != 0) && (*szString != '\n')) {
		if(*szString=='\2') {
			fDouble=!fDouble;
		} else {
			uiWidth+=BootVideoOverlayCharacter(
				pdwaTopLeftDestination+uiWidth, m_dwCountBytesPerLineDestination, rgbaOpaqueness, *szString, fDouble
				);
		}
		szString++;
	}
	return uiWidth;
}

void BootVideoVignette(
	DWORD * pdwaTopLeftDestination,
	DWORD m_dwCountBytesPerLineDestination,
	DWORD m_dwCountLines,
	RGBA rgbaColour1,
	RGBA rgbaColour2,
	DWORD dwStartLine,
	DWORD dwEndLine
) {
	int x,y;
	BYTE bDiffR, bDiffG, bDiffB;
	bool fPlusR, fPlusG, fPlusB;

	if((rgbaColour1 & 0xff0000) > (rgbaColour2 & 0xff0000) ) {
		bDiffR=((rgbaColour1 & 0xff0000)-(rgbaColour2 & 0xff0000))>>16;
		fPlusR=false;
	} else {
		bDiffR=((rgbaColour2 & 0xff0000)-(rgbaColour1 & 0xff0000))>>16;
		fPlusR=true;
	}
	if((rgbaColour1 & 0xff00) > (rgbaColour2 & 0xff00) ) {
		bDiffG=((rgbaColour1 & 0xff00)-(rgbaColour2 & 0xff00))>>8;
		fPlusG=false;
	} else {
		bDiffG=((rgbaColour2 & 0xff00)-(rgbaColour1 & 0xff00))>>8;
		fPlusG=true;
	}
	if((rgbaColour1 & 0xff) > (rgbaColour2 & 0xff) ) {
		bDiffB=((rgbaColour1 & 0xff)-(rgbaColour2 & 0xff));
		fPlusB=false;
	} else {
		bDiffB=((rgbaColour2 & 0xff)-(rgbaColour1 & 0xff));
		fPlusB=true;
	}
	for(y=0;y<m_dwCountLines;y++) {
		RGBA rgbaThisLine= 0xff000000;

		if((y>=dwStartLine) && (y<dwEndLine)) {

			if(fPlusR) rgbaThisLine|=((rgbaColour1&0xff0000)+(((((bDiffR)*y)/m_dwCountLines)<<16)&0xff0000))&0xff0000; else
				rgbaThisLine|=((rgbaColour1&0xff0000)-(((((bDiffR)*y)/m_dwCountLines)<<16)&0xff0000))&0xff0000;
			if(fPlusG) rgbaThisLine|=((rgbaColour1&0xff00)+(((((bDiffG)*y)/m_dwCountLines)<<8)&0xff00))&0xff00; else
				rgbaThisLine|=((rgbaColour1&0xff00)-(((((bDiffG)*y)/m_dwCountLines)<<8)&0xff00))&0xff00;
			if(fPlusB) rgbaThisLine|=((rgbaColour1&0xff)+(((((bDiffB)*y)/m_dwCountLines))&0xff))&0xff; else
				rgbaThisLine|=((rgbaColour1&0xff)-(((((bDiffB)*y)/m_dwCountLines))&0xff))&0xff;

			for(x=0;x<m_dwCountBytesPerLineDestination>>2;x++) *pdwaTopLeftDestination++=rgbaThisLine;

		} else {
			pdwaTopLeftDestination+=(m_dwCountBytesPerLineDestination>>2);
		}
	}
}


bool BootVideoJpegUnpackAsRgb(BYTE *pbaJpegFileImage, JPEG * pJpeg) {
  
	struct jpeg_decdata *decdata;
	int size, width, height, depth;
  
	decdata = (struct jpeg_decdata *)malloc(sizeof(struct jpeg_decdata));
	memset(decdata, 0x0, sizeof(struct jpeg_decdata));

	jpeg_get_size(pbaJpegFileImage, &width, &height, &depth);
	size = ((width + 15) & ~15) * ((height + 15) & ~15) * (depth >> 3);

	pJpeg->pData = (unsigned char *)malloc(size);
	memset(pJpeg->pData, 0x0, size);

	pJpeg->width = ((width + 15) & ~15);
	pJpeg->height = ((height +15) & ~ 15);
	pJpeg->bpp = depth >> 3;

	if((jpeg_decode(pbaJpegFileImage, pJpeg->pData, 
		((width + 15) & ~15), ((height + 15) & ~15), depth, decdata)) != 0) {
		printk("Error decode picture\n");
		while(1);
	}
	
	pJpeg->pBackdrop = BootVideoGetPointerToEffectiveJpegTopLeft(pJpeg);
	/*
	BootVideoJpegBlitBlend(
		(BYTE *)FB_START,
		640,
		pJpeg,
		pJpeg->pData,
		0,
		pJpeg->pData,
		pJpeg->width,
		pJpeg->height
	); while(1);
	*/

	free(decdata);
  
	return false;
}

BYTE * BootVideoGetPointerToEffectiveJpegTopLeft(JPEG * pJpeg)
{
	return ((BYTE *)(pJpeg->pData + pJpeg->width * ICON_HEIGH * pJpeg->bpp));
}

void BootVideoClearScreen(JPEG *pJpeg, int nStartLine, int nEndLine)
{
	VIDEO_CURSOR_POSX=vmode.xmargin;
	VIDEO_CURSOR_POSY=vmode.ymargin;

	if(nEndLine>=vmode.height) nEndLine=vmode.height-1;

	{
		if(pJpeg->pData!=NULL) {
			volatile DWORD *pdw=((DWORD *)FB_START)+vmode.width*nStartLine;
			int n1=pJpeg->bpp * pJpeg->width * nStartLine;
			BYTE *pbJpegBitmapAdjustedDatum=pJpeg->pBackdrop;

			while(nStartLine++<nEndLine) {
				int n;
				for(n=0;n<vmode.width;n++) {
					pdw[n]=0xff000000|
						((pbJpegBitmapAdjustedDatum[n1+2]))|
						((pbJpegBitmapAdjustedDatum[n1+1])<<8)|
						((pbJpegBitmapAdjustedDatum[n1])<<16)
					;
					n1+=pJpeg->bpp;
				}
				n1+=pJpeg->bpp * (pJpeg->width - vmode.width);
				pdw+=vmode.width; // adding DWORD footprints
			}
		}
	}
}

int VideoDumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable) { // returns bytes used
	int nCountUsed=0;
	while(dwCountBytesUsable) {

		DWORD dw=(dwAds & 0xfffffff0);
		char szAscii[17];
		char sz[256];
		int n=sprintf(sz, "%08X: ", dw);
		int nBytes=0;

		szAscii[16]='\0';
		while(nBytes<16) {
			if((dw<dwAds) || (dwCountBytesUsable==0)) {
				n+=sprintf(&sz[n], "   ");
				szAscii[nBytes]=' ';
			} else {
				BYTE b=*baData++;
				n+=sprintf(&sz[n], "%02X ", b);
				if((b<32) || (b>126)) szAscii[nBytes]='.'; else szAscii[nBytes]=b;
				nCountUsed++;
				dwCountBytesUsable--;
			}
			nBytes++;
			if(nBytes==8) n+=sprintf(&sz[n], ": ");
			dw++;
		}
		n+=sprintf(&sz[n], "   ");
		n+=sprintf(&sz[n], "%s", szAscii);
//		sz[n++]='\r';
		sz[n++]='\n';
		sz[n++]='\0';

		printk(sz, n);

		dwAds=dw;
	}
	return 1;
}
void BootVideoChunkedPrint(const char * szBuffer) {
	int n=0;
	int nDone=0;

	while (szBuffer[n] != 0)
	{
		if(szBuffer[n]=='\n') {
			BootVideoOverlayString(
				(DWORD *)((FB_START) + VIDEO_CURSOR_POSY * (vmode.width*4) + VIDEO_CURSOR_POSX),
				vmode.width*4, VIDEO_ATTR, &szBuffer[nDone]
			);
			nDone=n+1;
			VIDEO_CURSOR_POSY+=16; 
			VIDEO_CURSOR_POSX=vmode.xmargin<<2;
		}
		n++;
	}
	if (n != nDone)
	{
		VIDEO_CURSOR_POSX+=BootVideoOverlayString(
			(DWORD *)((FB_START) + VIDEO_CURSOR_POSY * (vmode.width*4) + VIDEO_CURSOR_POSX),
			vmode.width*4, VIDEO_ATTR, &szBuffer[nDone]
		)<<2;
		if (VIDEO_CURSOR_POSX > (vmode.width - 
			vmode.xmargin) <<2)
		{
			VIDEO_CURSOR_POSY+=16; 
			VIDEO_CURSOR_POSX=vmode.xmargin<<2;
		}
		
	}

}

int printk(const char *szFormat, ...) {  // printk displays to video and filtror if enabled
	char szBuffer[512*2];
	WORD wLength=0;
	va_list argList;
	va_start(argList, szFormat);
	wLength=(WORD) vsprintf(szBuffer, szFormat, argList);
//	wLength=strlen(szFormat); // temp!
//	memcpy(szBuffer, szFormat, wLength);
	va_end(argList);


	szBuffer[sizeof(szBuffer)-1]=0;
        if (wLength>(sizeof(szBuffer)-1)) wLength = sizeof(szBuffer)-1;
	szBuffer[wLength]='\0';
	        
	#if INCLUDE_SERIAL
	serialprint(&szBuffer[0]);
	#endif
	#if INCLUDE_FILTROR
//	BootFiltrorSendArrayToPcModal(&szBuffer[0], wLength);
	#endif

	BootVideoChunkedPrint(szBuffer);

	return wLength;
}

#if INCLUDE_SERIAL
int serialprint(const char *szFormat, ...) {
	char szBuffer[512*2];
	WORD wLength=0;
	WORD wSerialLength=0;
	va_list argList;
	va_start(argList, szFormat);
	wLength=(WORD) vsprintf(szBuffer, szFormat, argList);
	va_end(argList);
	
	szBuffer[sizeof(szBuffer)-1]=0;
        if (wLength>(sizeof(szBuffer)-1)) wLength = sizeof(szBuffer)-1;
	szBuffer[wLength]='\0';
        
	while(wSerialLength < wLength) {
		IoOutputByte(0x03f8, szBuffer[wSerialLength]);
		wSerialLength++;
	}

	return wLength;
}
#endif

int console_putchar(int c)
{
	char buf[2];
	buf[0] = (char)c;
	buf[1] = 0;
	BootVideoChunkedPrint(buf);
	return (int)buf[0];	
}

// Fix for BSD
#ifdef putchar
#undef putchar
#endif

int putchar(int c)
{
	return console_putchar(c);
}
