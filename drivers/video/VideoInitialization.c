/**
 * Common library functions for video initialization
 *
 * Oliver Schwartz, May 2003
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifdef JUSTVIDEO
#include <stdio.h>
#include <math.h>
#endif
#include <boot.h>

#include "VideoInitialization.h"

#define FALSE 0
#define TRUE 1

extern BYTE VIDEO_AV_MODE;
// functions defined elsewhere
int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite);
int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite);

// internally used structures

typedef struct {
	long h_blanki;
	long h_blanko;
	long v_blanki;
	long v_blanko;
	long vscale;
} BLANKING_PARAMETER;

#ifndef JUSTVIDEO
static double fabs(double d) {
	if (d > 0) return d;
	else return -d;
}
#endif

static BYTE NvGetCrtc(BYTE * pbRegs, int nIndex) {
	pbRegs[0x6013d4]=nIndex;
	return pbRegs[0x6013d5];
}

static void NvSetCrtc(BYTE * pbRegs, int nIndex, BYTE b) {
	pbRegs[0x6013d4]=nIndex;
	pbRegs[0x6013d5]=b;
}

xbox_tv_encoding DetectVideoStd(void) {
	xbox_tv_encoding videoStd;
	BYTE b=I2CTransmitByteGetReturn(0x54, 0x5A); // the eeprom defines the TV standard for the box

	if(b == 0x40) {
		videoStd = TV_ENC_NTSC;
	} else {
		videoStd = TV_ENC_PALBDGHI;
	}

	return videoStd;
}

xbox_av_type DetectAvType(void) {
	xbox_av_type avType;

	switch (VIDEO_AV_MODE) {
		case 0: avType = AV_SCART_RGB; break;
		case 1: avType = AV_HDTV; break;
		case 2: avType = AV_VGA_SOG; break;
		case 4: avType = AV_SVIDEO; break;
		case 6: avType = AV_COMPOSITE; break;
         	// Missing = NO CALBLE ATTACHED
         	case 7: avType = AV_VGA; break;
		default: avType = AV_COMPOSITE; break;
	}

	return avType;
}

void SetGPURegister(const GPU_PARAMETER* gpu, BYTE *pbRegs) {
	BYTE b;
	DWORD m = 0;

	// NVHDISPEND
	*((DWORD *)&pbRegs[0x680820]) = gpu->crtchdispend - 1;
	// NVHTOTAL
	*((DWORD *)&pbRegs[0x680824]) = gpu->nvhtotal;
	// NVHCRTC
	*((DWORD *)&pbRegs[0x680828]) = gpu->xres - 1;
	// NVHVALIDSTART
	*((DWORD *)&pbRegs[0x680834]) = 0;
	// NVHSYNCSTART
	*((DWORD *)&pbRegs[0x68082c]) = gpu->nvhstart;
	// NVHSYNCEND = NVHSYNCSTART + 32
	*((DWORD *)&pbRegs[0x680830]) = gpu->nvhstart+32;
	// NVHVALIDEND
	*((DWORD *)&pbRegs[0x680838]) = gpu->xres - 1;
	// CRTC_HSYNCSTART = h_total - 32 (heuristic)
	m = gpu->nvhtotal - 32;
	NvSetCrtc(pbRegs, 4, m/8);
	// CRTC_HSYNCEND = CRTC_HSYNCSTART + 16
	NvSetCrtc(pbRegs, 5, (NvGetCrtc(pbRegs, 5)&0xe0) | ((((m + 16)/8)-1)&0x1f) );
	// CRTC_HTOTAL = nvh_total (heuristic)
	NvSetCrtc(pbRegs, 0, (gpu->nvhtotal/8)-5);
	// CRTC_HBLANKSTART = crtchdispend
	NvSetCrtc(pbRegs, 2, ((gpu->crtchdispend)/8)-1);
	// CRTC_HBLANKEND = CRTC_HTOTAL = nvh_total
	NvSetCrtc(pbRegs, 3, (NvGetCrtc(pbRegs, 3)&0xe0) |(((gpu->nvhtotal/8)-1)&0x1f));
	NvSetCrtc(pbRegs, 5, (NvGetCrtc(pbRegs, 5)&(~0x80)) | ((((gpu->nvhtotal/8)-1)&0x20)<<2) );
	// CRTC_HDISPEND
	NvSetCrtc(pbRegs, 0x17, (NvGetCrtc(pbRegs, 0x17)&0x7f));
	NvSetCrtc(pbRegs, 1, ((gpu->crtchdispend)/8)-1);
	NvSetCrtc(pbRegs, 2, ((gpu->crtchdispend)/8)-1);
	NvSetCrtc(pbRegs, 0x17, (NvGetCrtc(pbRegs, 0x17)&0x7f)|0x80);
	// CRTC_LINESTRIDE = (xres / 8) * pixelDepth
	m=(gpu->xres / 8) * gpu->pixelDepth;
	NvSetCrtc(pbRegs, 0x19, (NvGetCrtc(pbRegs, 0x19)&0x1f) | ((m >> 3) & 0xe0));
	NvSetCrtc(pbRegs, 0x13, (m & 0xff));
	// NVVDISPEND
	*((DWORD *)&pbRegs[0x680800]) = gpu->yres - 1;
	// NVVTOTAL
	*((DWORD *)&pbRegs[0x680804]) = gpu->nvvtotal;
	// NVVCRTC
	*((DWORD *)&pbRegs[0x680808]) = gpu->yres - 1;
	// NVVVALIDSTART
	*((DWORD *)&pbRegs[0x680814]) = 0;
	// NVVSYNCSTART
	*((DWORD *)&pbRegs[0x68080c])=gpu->nvvstart;
	// NVVSYNCEND = NVVSYNCSTART + 3
	*((DWORD *)&pbRegs[0x680810])=(gpu->nvvstart+3);
	// NVVVALIDEND
	*((DWORD *)&pbRegs[0x680818]) = gpu->yres - 1;
	// CRTC_VSYNCSTART
	b = NvGetCrtc(pbRegs, 7) & 0x7b;
	NvSetCrtc(pbRegs, 7, b | ((gpu->crtcvstart >> 2) & 0x80) | ((gpu->crtcvstart >> 6) & 0x04));
	NvSetCrtc(pbRegs, 0x10, (gpu->crtcvstart & 0xff));
	// CRTC_VTOTAL
	b = NvGetCrtc(pbRegs, 7) & 0xde;
	NvSetCrtc(pbRegs, 7, b | ((gpu->crtcvtotal >> 4) & 0x20) | ((gpu->crtcvtotal >> 8) & 0x01));
	NvSetCrtc(pbRegs, 6, (gpu->crtcvtotal & 0xff));
	// CRTC_VBLANKEND = CRTC_VTOTAL
	b = NvGetCrtc(pbRegs, 0x16) & 0x80;
	NvSetCrtc(pbRegs, 0x16, b |(gpu->crtcvtotal & 0x7f));
	// CRTC_VDISPEND = yres
	b = NvGetCrtc(pbRegs, 7) & 0xbd;
	NvSetCrtc(pbRegs, 7, b | (((gpu->yres - 1) >> 3) & 0x40) | (((gpu->yres - 1) >> 7) & 0x02));
	NvSetCrtc(pbRegs, 0x12, ((gpu->yres - 1) & 0xff));
	// CRTC_VBLANKSTART
	b = NvGetCrtc(pbRegs, 9) & 0xdf;
	NvSetCrtc(pbRegs, 9, b | (((gpu->yres - 1)>> 4) & 0x20));
	b = NvGetCrtc(pbRegs, 7) & 0xf7;
	NvSetCrtc(pbRegs, 7, b | (((gpu->yres - 1) >> 5) & 0x08));
	NvSetCrtc(pbRegs, 0x15, ((gpu->yres - 1) & 0xff));
	// CRTC_LINECOMP
	m = 0x3ff; // 0x3ff = disable
	b = NvGetCrtc(pbRegs, 7) & 0xef;
	NvSetCrtc(pbRegs, 7, b | ((m>> 4) & 0x10));
	b = NvGetCrtc(pbRegs, 9) & 0xbf;
	NvSetCrtc(pbRegs, 9, b | ((m >> 3) & 0x40));
	NvSetCrtc(pbRegs, 0x18, (m & 0xff));
	// CRTC_REPAINT1
	if (gpu->xres < 1280) {
		b = 0x04;
	}
	else {
		b = 0x00;
	}
	NvSetCrtc(pbRegs, 0x1a, b);
	// Overflow bits
	/*
	b = ((hTotal   & 0x040) >> 2)
		| ((vDisplay & 0x400) >> 7)
		| ((vStart   & 0x400) >> 8)
		| ((vDisplay & 0x400) >> 9)
		| ((vTotal   & 0x400) >> 10);
	*/
	b = (((gpu->nvhtotal / 8 - 5) & 0x040) >> 2)
		| (((gpu->yres - 1) & 0x400) >> 7)
		| ((gpu->crtcvstart & 0x400) >> 8)
		| (((gpu->yres - 1) & 0x400) >> 9)
		| ((gpu->crtcvtotal & 0x400) >> 10);
	NvSetCrtc(pbRegs, 0x25, b);

	b = gpu->pixelDepth;
	if (b >= 3) b = 3;
	/* switch pixel mode to TV */
	b  |= 0x80;
	NvSetCrtc(pbRegs, 0x28, b);

	b = NvGetCrtc(pbRegs, 0x2d) & 0xe0;
	if ((gpu->nvhtotal / 8 - 1) >= 260) {
		b |= 0x01;
	}
	NvSetCrtc(pbRegs, 0x2d, b);
}




/*void SetTvModeParameter(const TV_MODE_PARAMETER* mode, BYTE *pbRegs) {
	ULONG v_activeo = 0;
	ULONG h_clko = 0;
	double hoc = 0;
	double voc = 0;
	double  tto = vidstda[mode->nVideoStd].m_dSecHsyncPeriod;
	double  ato = tto - (vidstda[mode->nVideoStd].m_dSecBlankBeginToHsync + vidstda[mode->nVideoStd].m_dSecActiveBegin);
	BLANKING_PARAMETER blanks;
	GPU_PARAMETER gpu;

	// Let's go
	CalcBlankings (mode, &blanks);
	v_activeo = CalcV_ACTIVEO(mode);
	h_clko = CalcH_CLKO(mode);
	hoc = 1 - ((2* (double)mode->h_active / (double)h_clko) / (ato / tto));
	voc = 1- ((double)v_activeo / vidstda[mode->nVideoStd].m_dwALO);
#ifdef JUSTVIDEO
	printf("Computed horizontal overscan factor=%.2f%%\n", hoc * 100);
	printf("Computed vertical overscan factor=%.2f%%\n", voc * 100);
	printf("H_CLKO: %ld\n", h_clko/2);
	printf("V_ACTIVEO: %ld\n", v_activeo);
	printf("H_BLANKI: %ld\n", blanks.h_blanki);
	printf("H_BLANKO: %ld\n", blanks.h_blanko);
	printf("V_BLANKI: %ld\n", blanks.v_blanki);
	printf("V_BLANKO: %ld\n", blanks.v_blanko);
#endif
	gpu.xres = mode->h_active;
	gpu.crtchdispend = mode->h_active + 8;
	gpu.nvhstart = mode->h_clki - blanks.h_blanki - 7;
	gpu.nvhtotal =  mode->h_clki - 1;
	gpu.yres = mode->v_activei;
	gpu.nvvstart = mode->v_linesi - blanks.v_blanki + 1;
	gpu.crtcvstart = mode->v_activei + 32;
	// CRTC_VTOTAL = v_linesi - yres + CRTC_VSYNCSTART = v_linesi + 32
	gpu.crtcvtotal = mode->v_linesi + 32;
	gpu.nvvtotal =  mode->v_linesi - 1;
	gpu.pixelDepth = (mode->bpp+1) / 8;

	if (VideoEncoder == VIDEO_CONEXANT) SetTVConexantRegister(mode, &blanks, pbRegs);
	SetGPURegister(&gpu, pbRegs);
}

void SetVgaHdtvModeParameter(const VGA_MODE_PARAMETER* mode, BYTE *pbRegs) {
	double f_h;
	double f_v;
	GPU_PARAMETER gpu;
	BYTE pll_int = (BYTE)(mode->pixclock * 6.0 / dPllBaseClockFrequency + 0.5);
	f_h = (dPllBaseClockFrequency * pll_int) / (6.0 * mode->htotal);
	f_v = f_h / mode->vtotal;

#ifdef JUSTVIDEO
	printf("Horizontal sync frequency: %.2f kHz\n", f_h/1000);
	printf("Vertical sync frequency: %.2f Hz\n", f_v);
#endif
	gpu.xres = mode->xres;
	gpu.nvhstart = mode->hsyncstart;
	gpu.nvhtotal = mode->htotal;
	gpu.yres = mode->yres;
	gpu.nvvstart = mode->vsyncstart;
	gpu.nvvtotal = mode->vtotal;
	gpu.pixelDepth = (mode->bpp + 1) / 8;
	gpu.crtchdispend = mode->xres;
	gpu.crtcvstart = mode->vsyncstart;
	gpu.crtcvtotal = mode->vtotal;
    if (DetectAvType() == AV_HDTV) {
		xbox_hdtv_mode hdtv_mode = HDTV_480p;
		if (mode->yres > 800) {
			hdtv_mode = HDTV_1080i;
		}
		else if (mode->yres > 600) {
			hdtv_mode = HDTV_720p;
		}
		SetHDTVConexantRegister(hdtv_mode, pll_int, pbRegs);
	}
	else {
		SetVGAConexantRegister(pll_int, pbRegs);
	}
	SetGPURegister(&gpu, pbRegs);
}*/
