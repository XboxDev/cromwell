
#include  "boot.h"
#include "string.h"
#include "fontx16.h"  // brings in font struct

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

// 2D memcpy

void BootVideoBlit(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	DWORD * pdwTopLeftSource,
	DWORD dwCountBytesPerLineSource,
	DWORD dwCountLines
) {
	DWORD dwBytesPerLineToCopy=dwCountBytesPerLineSource;
	if(dwCountBytesPerLineDestination<dwBytesPerLineToCopy) dwBytesPerLineToCopy=dwCountBytesPerLineDestination;
	while(dwCountLines--) {
		memcpy(pdwTopLeftDestination, pdwTopLeftSource, dwBytesPerLineToCopy);
		pdwTopLeftDestination+=dwCountBytesPerLineDestination>>2;
		pdwTopLeftSource+=dwCountBytesPerLineSource>>2;
	}
}

void BootVideoJpegBlitBlend(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	JPEG * pJpeg,
	DWORD * pdwTopLeftInJpegBitmap,
	RGBA m_rgbaTransparent,
	DWORD * pdwTopLeftBackground,
	DWORD dwCountBytesPerLineBackground,
	DWORD dwCountBytesPerPixelBackground,
	int x,
	int y
) {
	int n=0;

	int nTransAsByte=m_rgbaTransparent>>24;
	int nBackTransAsByte=255-nTransAsByte;

	m_rgbaTransparent|=0xff000000;
	m_rgbaTransparent&=0xffc0c0c0;

	while(y--) {
		BYTE *pbDest=(BYTE *)pdwTopLeftDestination;
		BYTE *pbBackground=(BYTE *)pdwTopLeftBackground;
		BYTE *pData=((BYTE *)pdwTopLeftInJpegBitmap);

		if(dwCountBytesPerPixelBackground!=4) { // jpeg backdrop
			for(n=0;n<x;n++) {
				DWORD dw=((*((DWORD *)&pData[0]))|0xff000000)&0xffc0c0c0;
				if(dw!=m_rgbaTransparent) {
					pbDest[2]=((pData[0]*nTransAsByte)+(pbBackground[0]*nBackTransAsByte))>>8;
					pbDest[1]=((pData[1]*nTransAsByte)+(pbBackground[1]*nBackTransAsByte))>>8;
					pbDest[0]=((pData[2]*nTransAsByte)+(pbBackground[2]*nBackTransAsByte))>>8;
				}
				pbDest+=4;
				pData+=pJpeg->m_nBytesPerPixel;
				pbBackground+=dwCountBytesPerPixelBackground;
			}
		} else {  // RGBA backdrop
			for(n=0;n<x;n++) {
				DWORD dw=((*((DWORD *)&pData[0]))|0xff000000)&0xffc0c0c0;
				if(dw!=m_rgbaTransparent) {
					pbDest[2]=((pData[0]*nTransAsByte)+(pbBackground[2]*nBackTransAsByte))>>8;
					pbDest[1]=((pData[1]*nTransAsByte)+(pbBackground[1]*nBackTransAsByte))>>8;
					pbDest[0]=((pData[2]*nTransAsByte)+(pbBackground[0]*nBackTransAsByte))>>8;
				}
				pbDest+=4;
				pData+=pJpeg->m_nBytesPerPixel;
				pbBackground+=dwCountBytesPerPixelBackground;
			}
		}
		pdwTopLeftInJpegBitmap+=(pJpeg->m_nWidth*pJpeg->m_nBytesPerPixel)>>2;
		pdwTopLeftDestination+=dwCountBytesPerLineDestination>>2;
		pdwTopLeftBackground+=dwCountBytesPerLineBackground>>2;
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



//			nOpaquenessMultiplied=(((int)(unsigned int)(pbColour[3]))*(int)b1)/15;
//			nTransparentnessMultiplied=(0xff-nOpaquenessMultiplied);

			if(b1) {
				*pbaDest=(BYTE)((b1*(rgbaColourAndOpaqueness&0xff))>>4); pbaDest++;
				*pbaDest=(BYTE)((b1*((rgbaColourAndOpaqueness>>8)&0xff))>>4); pbaDest++;
				*pbaDest=(BYTE)((b1*((rgbaColourAndOpaqueness>>16)&0xff))>>4); pbaDest++;
				*pbaDest++=0xff;
			} else {
				pbaDest+=4;
			}
//			*pbaDest++=0x80;


//			*pbaDest=(BYTE)0xff; pbaDest++;
//			*pbaDest=(BYTE)(((nOpaquenessMultiplied * (unsigned int)pbColour[0]) + (nTransparentnessMultiplied * (unsigned int)*pbaDest))/255); pbaDest++;
//			*pbaDest=(BYTE)(((nOpaquenessMultiplied * (unsigned int)pbColour[1]) + (nTransparentnessMultiplied * (unsigned int)*pbaDest))/255); pbaDest++;
//			*pbaDest=(BYTE)(((nOpaquenessMultiplied * (unsigned int)pbColour[2]) + (nTransparentnessMultiplied * (unsigned int)*pbaDest))/255); pbaDest++;

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
	while(*szString) {
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


bool BootVideoJpegUnpackAsRgb(
	BYTE *pbaJpegFileImage,
	int nFileLength,
	JPEG * pJpeg
)
{
	struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  BYTE *pbaResultAsRgb;
  BYTE *pbaResultAsRgbStart;
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */

	void jpeg_direct_src (j_decompress_ptr pcinfo, char * pbStartOfBuffer, unsigned int nLength);

  pJpeg->m_pBitmapData=NULL;
	cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_decompress(&cinfo);
  jpeg_direct_src(&cinfo, pbaJpegFileImage, nFileLength);
  (void) jpeg_read_header(&cinfo, TRUE);
  (void) jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * cinfo.output_components;
	(BYTE *)pbaResultAsRgb=(BYTE *)malloc(row_stride * cinfo.output_height);
	(BYTE *)pbaResultAsRgbStart=pbaResultAsRgb;

	//printk("0x%x, 0x%x, %d, %d, %d, 0x%x, 0x%x\n",
	//	(DWORD)pbaJpegFileImage, nFileLength, cinfo.output_width,  cinfo.output_height, cinfo.output_components,
	//	row_stride, (DWORD)pbaResultAsRgb
	//);

	buffer = (*(cinfo.mem->alloc_sarray))((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

	pJpeg->m_nWidth=cinfo.output_width;
	pJpeg->m_nHeight=cinfo.output_height;
	pJpeg->m_nBytesPerPixel=cinfo.output_components;
	pJpeg->m_pBitmapData=pbaResultAsRgbStart;

//	printk("jpeg bpp=%d\n",pJpeg->m_nBytesPerPixel);

  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(pbaResultAsRgb, buffer[0], row_stride);
		pbaResultAsRgb+=row_stride;
  }
  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

	if(jerr.num_warnings==0) return true;
	free(pbaResultAsRgbStart);
	return false;
}

BYTE * BootVideoGetPointerToEffectiveJpegTopLeft(JPEG * pJpeg)
{
	DWORD dwCountPixelsHorzOffsetIntoJpeg=(pJpeg->m_nWidth-currentvideomodedetails.m_dwWidthInPixels)/2;
	DWORD dwCountLinesVertOffsetIntoJpeg=(pJpeg->m_nHeight-currentvideomodedetails.m_dwHeightInLines)/2;
	return ((BYTE *)pJpeg->m_pBitmapData)+
			(dwCountLinesVertOffsetIntoJpeg * pJpeg->m_nWidth * pJpeg->m_nBytesPerPixel)+
			(dwCountPixelsHorzOffsetIntoJpeg * pJpeg->m_nBytesPerPixel)
		;
}

void BootVideoClearScreen(JPEG * pJpeg, int nStartLine, int nEndLine)
{
	VIDEO_CURSOR_POSX=currentvideomodedetails.m_dwMarginXInPixelsRecommended;
	VIDEO_CURSOR_POSY=currentvideomodedetails.m_dwMarginYInLinesRecommended;

//	bprintf("%x, %d, %d\n", FRAMEBUFFER_START, currentvideomodedetails.m_dwWidthInPixels, currentvideomodedetails.m_dwHeightInLines);

	if(nEndLine>=currentvideomodedetails.m_dwHeightInLines) nEndLine=currentvideomodedetails.m_dwHeightInLines-1;

	{
		if(pJpeg->m_pBitmapData!=NULL) {
			DWORD *pdw=((DWORD *)FRAMEBUFFER_START)+currentvideomodedetails.m_dwWidthInPixels*nStartLine;
			int n1=pJpeg->m_nBytesPerPixel * pJpeg->m_nWidth * nStartLine;
			BYTE *pbJpegBitmapAdjustedDatum=BootVideoGetPointerToEffectiveJpegTopLeft(pJpeg) ;

			while(nStartLine++<nEndLine) {
				int n;
				for(n=0;n<currentvideomodedetails.m_dwWidthInPixels;n++) {
					pdw[n]=0xff000000|
						((pbJpegBitmapAdjustedDatum[n1+2]))|
						((pbJpegBitmapAdjustedDatum[n1+1])<<8)|
						((pbJpegBitmapAdjustedDatum[n1])<<16)
					;
					n1+=pJpeg->m_nBytesPerPixel;
				}
				n1+=pJpeg->m_nBytesPerPixel * (pJpeg->m_nWidth - currentvideomodedetails.m_dwWidthInPixels);
				pdw+=currentvideomodedetails.m_dwWidthInPixels; // adding DWORD footprints
			}
		} else{
			printk("null jpg");
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
void BootVideoChunkedPrint(char * szBuffer, WORD wLength) {
	int n, nDone=0;

	szBuffer[wLength]='\0';

	for(n=0;n<(int)(wLength+1);n++) {
		if((szBuffer[n]=='\n') || (szBuffer[n]=='\0')) {
			bool f=(n<wLength);
			szBuffer[n]='\0';
			if(n!=nDone) {
				VIDEO_CURSOR_POSX+=BootVideoOverlayString(
					(DWORD *)((FRAMEBUFFER_START) + VIDEO_CURSOR_POSY * (currentvideomodedetails.m_dwWidthInPixels*4) + VIDEO_CURSOR_POSX),
					currentvideomodedetails.m_dwWidthInPixels*4, VIDEO_ATTR, &szBuffer[nDone]
				)<<2;
				nDone=n+1;
			}
			if(f) { VIDEO_CURSOR_POSY+=16; VIDEO_CURSOR_POSX=currentvideomodedetails.m_dwMarginXInPixelsRecommended<<2; }
		}
	}
//	__asm__ __volatile__ ( "wbinvd" );
}

int printk(const char *szFormat, ...) {  // printk displays to video and filtror if enabled
	char szBuffer[512];
	WORD wLength=0;
	va_list argList;
	va_start(argList, szFormat);
	wLength=(WORD) vsprintf(szBuffer, szFormat, argList);
//	wLength=strlen(szFormat); // temp!
//	memcpy(szBuffer, szFormat, wLength);
	va_end(argList);

	szBuffer[wLength]='\0';

	#if INCLUDE_SERIAL
	serialprint(&szBuffer[0]);
	#endif
	#if INCLUDE_FILTROR
//	BootFiltrorSendArrayToPcModal(&szBuffer[0], wLength);
	#endif

	BootVideoChunkedPrint(szBuffer, wLength);

	return wLength;
}

#if INCLUDE_SERIAL
int serialprint(const char *szFormat, ...) {
	char szBuffer[512];
	WORD wLength=0;
	WORD wSerialLength=0;
	va_list argList;
	va_start(argList, szFormat);
	wLength=(WORD) vsprintf(szBuffer, szFormat, argList);
	va_end(argList);

	while(wSerialLength < wLength) {
		IoOutputByte(0x03f8, szBuffer[wSerialLength]);
		wSerialLength++;
	}

	return wLength;
}
#endif


