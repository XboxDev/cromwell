#ifndef video_h
#define video_h

#include "stdlib.h"

// video helpers
typedef struct {
	u8 * pData;
	u8 * pBackdrop;
	int width;
	int height;
	int bpp;
} JPEG;

int BootVideoOverlayString(DWORD * pdwaTopLeftDestination, DWORD m_dwCountBytesPerLineDestination, RGBA rgbaOpaqueness, const char * szString);
void BootVideoChunkedPrint(const char * szBuffer);
int VideoDumpAddressAndData(DWORD dwAds, const u8 * baData, DWORD dwCountBytesUsable);
unsigned int BootVideoGetStringTotalWidth(const char * szc);
void BootVideoClearScreen(JPEG * pJpeg, int nStartLine, int nEndLine);

void BootVideoJpegBlitBlend(
	u8 *pDst,
	DWORD dst_width,
	JPEG * pJpeg,
	u8 *pFront,
	RGBA m_rgbaTransparent,
	u8 *pBack,
	int x,
	int y
);

bool BootVideoJpegUnpackAsRgb(
	u8 *pbaJpegFileImage,
	JPEG * pJpeg
);

void BootVideoEnableOutput(u8 bAvPack);
u8 * BootVideoGetPointerToEffectiveJpegTopLeft(JPEG * pJpeg);

extern u8 baBackdrop[60*72*4];
extern JPEG jpegBackdrop;

#endif /* #ifndef video_h */
