#ifndef video_h
#define video_h

#include "stdlib.h"
#include "jpeglib.h"

// video helpers

void BootVideoBlit(
	DWORD * pdwTopLeftDestination,
	DWORD dwCountBytesPerLineDestination,
	DWORD * pdwTopLeftSource,
	DWORD dwCountBytesPerLineSource,
	DWORD dwCountLines
);

void BootVideoVignette(
	DWORD * pdwaTopLeftDestination,
	DWORD m_dwCountBytesPerLineDestination,
	DWORD m_dwCountLines,
	RGBA rgbaColour1,
	RGBA rgbaColour2,
	DWORD dwStartLine,
	DWORD dwEndLine
);


typedef struct {
	BYTE * m_pBitmapData;
	int m_nWidth;
	int m_nHeight;
	int m_nBytesPerPixel;
} JPEG;

int BootVideoOverlayString(DWORD * pdwaTopLeftDestination, DWORD m_dwCountBytesPerLineDestination, RGBA rgbaOpaqueness, const char * szString);
void BootVideoChunkedPrint(const char * szBuffer);
int VideoDumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable);
unsigned int BootVideoGetStringTotalWidth(const char * szc);
void BootVideoClearScreen(JPEG * pJpeg, int nStartLine, int nEndLine);
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
);

bool BootVideoJpegUnpackAsRgb(
	BYTE *pbaJpegFileImage,
	int nFileLength,
	JPEG * pJpeg
);

void BootVideoEnableOutput(BYTE bAvPack);
BYTE * BootVideoGetPointerToEffectiveJpegTopLeft(JPEG * pJpeg);

extern BYTE baBackdrop[60*72*4];
extern JPEG jpegBackdrop;

#endif /* #ifndef video_h */
