/*
 * video-related stuff
 * 2003-02-02  andy@warmcat.com  Major reshuffle, threw out tons of unnecessary init
                                 Made a good start on separating the video mode from the AV cable
																 Consolidated init tables into a big struct (see boot.h)
 * 2002-12-04  andy@warmcat.com  Video now working :-)
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "BootEEPROM.h"
#include "config.h"
#include "BootVideo.h"
#include "VideoInitialization.h"
#include "BootVgaInitialization.h"

DWORD nvRAMDACRegValues_focus_composite_pal[] = {

0x000001df, 0x0000020c, 0x000001df, 0x000001f4, 0x000001f6, 0x00000000, 0x000001df, 0x00000000, 0x0000027f, 0x0000035f, 
0x00000257, 0x000002cf, 0x0000030f, 0x00000000, 0x0000027f, 0x00000000, 0x0068e9d9, 0x00000000, 0x10100111, 0x00801080, 
0x21101100, 0x00000000, 0x00000000, 0x10001000, 0x10000000, 0x10000000, 0x10000000, 0x10000000, 0xf32dd32c, 0xe87e4737, 
0xef0d75c7, 0xbb73a6ff, 0x00007702, 0x0003c20d, 0x00050505, 0x0003c20d, 0x00007702, 0x0003c20d, 0x00060606, 0x0003c20d, 
0x00007702, 0x0003c20d, 0x00060606, 0x0003c20d, 0x00007702, 0x0003c20d, 0x00060606, 0x0003c20d, 0xffff0107, 0x00bffebb, 
0x345ed537, 0x0068e9d9, 0x00000000, 0x40801080, 0x00000002, 0x000dd7c0, 0x00000271, 0x000000be, 0x000002f8, 0x00000000, 
0x02a000a3 

};

BYTE nvCRTRegs_focus_composite_pal[] = {

0x67, 0x4f, 0x4f, 0x8b, 0x59, 0xbb, 0x0b, 0x3e, 0x00, 0x40, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, 0x06, 0xdf, 0x40, 
0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x30, 0x3a, 0x05, 0x00, 0x00, 
0x00, 0x03, 0x29, 0xfe, 0x7f, 0xa1, 0x00, 0x10, 0x13, 0xa3, 
0x83, 0x00, 0x00, 0x95, 0x7c, 0xe0, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x11, 0x02, 0x02, 0x00, 0x30, 0x00, 0xff, 0x5f, 0x26, 
0xf7, 0x00, 0x10, 0x30, 0x00, 0x00 

};

unsigned short focus_composite_pal[] = {

	0x0C01,	0x0D00,	0x1200,	0x1300,	0x1600,	0x1700,	0x18EF,	0x1943,
	0x1C07,	0x1D07,	0x2400,	0x2500,	0x2610,	0x2700,	0x4400,	0x4500,
	0x4D01,	0x5EC8,	0x5F00,	0x6500,	0x7510,	0x8318,	0x8400,	0x8500,
	0x8618,	0x8700,	0x8800,	0x8B9C,	0x8C03,	0x9EE4,	0x9F00,	0xA000,
	0xA102,	0xA200,	0xA300,	0xA400,	0xA500,	0xA600,	0xA700,	0xB6F0,
	0xB700,	0xC2EE,	0xC300,	0x0C01,	0x0D21,	0x0E15,	0x0F00,	0x0092,
	0x0100,	0x0219,	0x0300,	0x0480,	0x0502,	0x06C3,	0x0730,	0x0800,
	0x0910,	0x1054,	0x1100,	0x14C8,	0x1500,	0x1A3F,	0x1B00,	0x38A4,
	0x3900,	0x402A,	0x4109,	0x428A,	0x43CB,	0x468D,	0x4700,	0x487C,
	0x493C,	0x4A9A,	0x4B2F,	0x4C21,	0x4E3F,	0x4F00,	0x503E,	0x5103,
	0x609D,	0x629D,	0x691A,	0x6C1E,	0x7315,	0x7449,	0x7C3E,	0x7D03,
	0x8057,	0x812F,	0x8207,	0x8916,	0x8A16,	0x92C4,	0x9348,	0x9A00,
	0x9B80,	0xB2D7,	0xB305,	0xC000,	0xC100,	0x0C03,	0x0D21,	0x0E15,
	0x0F04,	0x0C00,	0x0D21,	0x8508,	0x8400,	0x8300,	0x8808,	0x8700,
	0x8600

};

/* ------------------------------------------------------------------------------------------ */

DWORD nvRAMDACRegValues_focus_composite_ntsc[] = {

0x000001df, 0x0000020c, 0x000001df, 0x000001f2, 0x000001f4, 0x00000000, 0x000001df, 0x00000000, 0x0000027f, 0x000003a7, 
0x00000257, 0x000002f3, 0x00000333, 0x00000000, 0x0000027f, 0x00000000, 0x00f9da63, 0x00000000, 0x10100111, 0x00801080, 
0x21101100, 0x00000000, 0x00000000, 0x10001000, 0x10000000, 0x10000000, 0x10000000, 0x10000000, 0xe32fd32c, 0x5d9b8f63, 
0x859791c7, 0x98fdf7f9, 0x00007702, 0x0003c20d, 0x00000000, 0x0003c20d, 0x00007702, 0x0003c20d, 0x00010101, 0x0003c20d, 
0x00007702, 0x0003c20d, 0x00010101, 0x0003c20d, 0x00007702, 0x0003c20d, 0x00010101, 0x0003c20d, 0xffff0102, 0x003f7c77, 
0x3ffafdbf, 0x00f9da63, 0x00000000, 0x40801080, 0x00000002, 0x00000750, 0x00000001, 0x0000009b, 0x0000026c, 0x00000000, 
0x02a000a3 

};
BYTE nvCRTRegs_focus_composite_ntsc[] = {

0x70, 0x4f, 0x4f, 0x94, 0x5d, 0xbf, 0x0b, 0x3e, 0x00, 0x40, // fast
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0x04, 0xdf, 0x40, // fast
0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x30, 0x3a, 0x05, 0x00, 0x00, // ident
0x00, 0x03, 0x29, 0xfe, 0x7f, 0xa1, 0x80, 0x10, 0x14, 0xa3, //fast
0x83, 0x00, 0x00, 0x95, 0x7c, 0xe0, 0x00, 0x00, 0x00, 0x00, // ident
0x00, 0x11, 0x02, 0x02, 0x00, 0x30, 0x00, 0xff, 0x5f, 0x26, // ident
0xf7, 0x00, 0x10, 0x30, 0x00, 0x00 // ident

};

unsigned short focus_composite_ntsc[] = {

	0x0C01,	0x0D00,	0x1200,	0x1300,	0x1600,	0x1700,	0x18EF,	0x1943,
	0x1C07,	0x1D07,	0x2400,	0x2500,	0x2610,	0x2700,	0x4400,	0x4500,
	0x4D01,	0x5EC8,	0x5F00,	0x6500,	0x7510,	0x8318,	0x8400,	0x8500,
	0x8618,	0x8700,	0x8800,	0x8B9C,	0x8C03,	0x9EE4,	0x9F00,	0xA000,
	0xA102,	0xA200,	0xA300,	0xA400,	0xA500,	0xA600,	0xA700,	0xB6F0,
	0xB700,	0xC2EE,	0xC300,	0x0C01,	0x0D20,	0x0E15,	0x0F00,	0x00B6,
	0x0100,	0x0218,	0x0300,	0x0480,	0x0502,	0x0600,	0x0700,	0x0800,
	0x0910,	0x1006,	0x1100,	0x1416,	0x1500,	0x1A1F,	0x1B00,	0x3882,
	0x3900,	0x4021,	0x41F0,	0x427C,	0x431F,	0x4689,	0x4700,	0x487C,
	0x4940,	0x4A80,	0x4B3E,	0x4C00,	0x4E46,	0x4F02,	0x503C,	0x5100,
	0x6091,	0x6291,	0x6919,	0x6C24,	0x7314,	0x7404,	0x7C3C,	0x7D00,
	0x8067,	0x8121,	0x820C,	0x8913,	0x8A13,	0x92C4,	0x9348,	0x9A00,
	0x9B00,	0xB255,	0xB305,	0xC000,	0xC100,	0x0C03,	0x0D20,	0x0E15,
	0x0F04,	0x0C00,	0x0D20,	0x8500,	0x8400,	0x8318,	0x8800,	0x8700,
	0x8618
};    

DWORD nvMainRegs[] =
{ 
		0x680800, 0x680804, 0x680808, 0x68080c,
		0x680810, 0x680814, 0x680818, 0x68081c,
		0x680820, 0x680824, 0x680828, 0x68082c,
		0x680830, 0x680834, 0x680838, 0x68083c,
		0x680840, 0x680844, 0x680848, 0x68084c,
		0x680880, 0x680884, 0x680888, 0x68088c,
		0x680890, 0x680894, 0x680898, 0x68089c,
		0x680324, 0x680328, 0x68032c, 0x680330,
		0x680504, 0x680508, 0x680518, 0x680520,
		0x680544, 0x680548, 0x680558, 0x680560,
		0x680584, 0x680588, 0x680598, 0x6805a0,
		0x6805c4, 0x6805c8, 0x6805d8, 0x6805e0,
		0x680604, 0x68060c, 0x680624, 0x680840,
		0x6808C0, 0x6808C4, 0x680630, 0x680680,
		0x680684, 0x680688, 0x68068C, 0x680690,
};



void BootVgaInitializationKernelNG(CURRENT_VIDEO_MODE_DETAILS * pcurrentvideomodedetails) {
	EVIDEOSTD videoStd;
	EAVTYPE avType;
	TV_MODE_PARAMETER parameter;
	BYTE b;
	RIVA_HW_INST riva;

	videoStd = DetectVideoStd();
	
        // Dump to global variable
	VIDEO_AV_MODE=I2CTransmitByteGetReturn(0x10, 0x04);


   	memset((void *)pcurrentvideomodedetails,0,sizeof(CURRENT_VIDEO_MODE_DETAILS));
	pcurrentvideomodedetails->m_nVideoModeIndex =VIDEO_MODE_800x600;


	if(((BYTE *)&eeprom)[0x96]&0x01) { // 16:9 widescreen TV
		pcurrentvideomodedetails->m_nVideoModeIndex=VIDEO_MODE_1024x576;
	} else { // 4:3 TV
		pcurrentvideomodedetails->m_nVideoModeIndex=VIDEO_PREFERRED_MODE;
	}

	pcurrentvideomodedetails->m_pbBaseAddressVideo=(BYTE *)0xfd000000;
	pcurrentvideomodedetails->m_fForceEncoderLumaAndChromaToZeroInitially=1;

        // If the client hasn't set the frame buffer start address, assume
        // it should be at 4M from the end of RAM.

	pcurrentvideomodedetails->m_dwFrameBufferStart = FRAMEBUFFER_START;

        (*(unsigned int*)0xFD600800) = (FRAMEBUFFER_START & 0x0fffffff);

	pcurrentvideomodedetails->m_bAvPack=I2CTransmitByteGetReturn(0x10, 0x04);
	pcurrentvideomodedetails->m_pbBaseAddressVideo=(BYTE *)0xfd000000;
	pcurrentvideomodedetails->m_fForceEncoderLumaAndChromaToZeroInitially=1;
	pcurrentvideomodedetails->m_bBPP = 32;

	b=I2CTransmitByteGetReturn(0x54, 0x5A); // the eeprom defines the TV standard for the box

	// The values for hoc and voc are stolen from nvtv small mode

	if(b != 0x40) {
		pcurrentvideomodedetails->hoc = 13.44;
		pcurrentvideomodedetails->voc = 14.24;
	} else {
		pcurrentvideomodedetails->hoc = 15.11;
		pcurrentvideomodedetails->voc = 14.81;
	}
	pcurrentvideomodedetails->hoc /= 100.0;
	pcurrentvideomodedetails->voc /= 100.0;

	mapNvMem(&riva,pcurrentvideomodedetails->m_pbBaseAddressVideo);
	unlockCrtNv(&riva,0);

	if (xbox_ram == 128) {
		MMIO_H_OUT32(riva.PFB    ,0,0x200,0x03070103);
	} else {
		MMIO_H_OUT32(riva.PFB    ,0,0x200,0x03070003);
	}

	MMIO_H_OUT32 (riva.PCRTC, 0, 0x800, pcurrentvideomodedetails->m_dwFrameBufferStart);

	IoOutputByte(0x80d3, 5);  // definitively kill video out using an ACPI control pin

	MMIO_H_OUT32(riva.PRAMDAC,0,0x880,0);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x884,0x0);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x888,0x0);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x88c,0x10001000);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x890,0x10000000);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x894,0x10000000);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x898,0x10000000);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x89c,0x10000000);
	MMIO_H_OUT32(riva.PRAMDAC,0,0x84c,0x0); // remove magenta borders in RGB mode

	writeCrtNv (&riva, 0, 0x14, 0x00);
	writeCrtNv (&riva, 0, 0x17, 0xe3); // Set CRTC mode register
	writeCrtNv (&riva, 0, 0x19, 0x10); // ?
	writeCrtNv (&riva, 0, 0x1b, 0x05); // arbitration0
	writeCrtNv (&riva, 0, 0x22, 0xff); // ?
	writeCrtNv (&riva, 0, 0x33, 0x11); // ?

	avType = DetectAvType();
	if ((avType == AV_VGA) || (avType == AV_VGA_SOG)) {
		VGA_MODE_PARAMETER mode;
		// Settings for 800x600@56Hz, 35 kHz HSync
		pcurrentvideomodedetails->m_dwWidthInPixels=800;
		pcurrentvideomodedetails->m_dwHeightInLines=600;
		pcurrentvideomodedetails->m_dwMarginXInPixelsRecommended=20;
		pcurrentvideomodedetails->m_dwMarginYInLinesRecommended=20;
		mode.bpp = 32;
		mode.xres = 800;
		mode.hsyncstart = 900;
		mode.htotal =  1028;
		mode.yres = 600;
		mode.vsyncstart = 614;
		mode.vtotal = 630;
		mode.pixclock = 36000000;
		SetVgaModeParameter(&mode, pcurrentvideomodedetails->m_pbBaseAddressVideo);
	}
	else
	{
		switch(pcurrentvideomodedetails->m_nVideoModeIndex) {
			case VIDEO_MODE_640x480:
				pcurrentvideomodedetails->m_dwWidthInPixels=640;
				pcurrentvideomodedetails->m_dwHeightInLines=480;
				pcurrentvideomodedetails->m_dwMarginXInPixelsRecommended=0;
				pcurrentvideomodedetails->m_dwMarginYInLinesRecommended=0;
				break;
			case VIDEO_MODE_640x576:
				pcurrentvideomodedetails->m_dwWidthInPixels=640;
				pcurrentvideomodedetails->m_dwHeightInLines=576;
				pcurrentvideomodedetails->m_dwMarginXInPixelsRecommended=40; // pixels
				pcurrentvideomodedetails->m_dwMarginYInLinesRecommended=40; // lines
				break;
			case VIDEO_MODE_720x576:
				pcurrentvideomodedetails->m_dwWidthInPixels=720;
				pcurrentvideomodedetails->m_dwHeightInLines=576;
				pcurrentvideomodedetails->m_dwMarginXInPixelsRecommended=40; // pixels
				pcurrentvideomodedetails->m_dwMarginYInLinesRecommended=40; // lines
				break;
			case VIDEO_MODE_800x600: // 800x600
				pcurrentvideomodedetails->m_dwWidthInPixels=800;
				pcurrentvideomodedetails->m_dwHeightInLines=600;
				pcurrentvideomodedetails->m_dwMarginXInPixelsRecommended=20;
				pcurrentvideomodedetails->m_dwMarginYInLinesRecommended=20; // lines
				break;
			case VIDEO_MODE_1024x576: // 1024x576
				pcurrentvideomodedetails->m_dwWidthInPixels=1024;
				pcurrentvideomodedetails->m_dwHeightInLines=576;
				pcurrentvideomodedetails->m_dwMarginXInPixelsRecommended=20;
				pcurrentvideomodedetails->m_dwMarginYInLinesRecommended=20; // lines
				I2CTransmitWord(0x45, (0x60<<8)|0xc7);
				I2CTransmitWord(0x45, (0x62<<8)|0x0);
				I2CTransmitWord(0x45, (0x64<<8)|0x0);
				break;
		}
		// Use TV settings
		if(FindOverscanValues(pcurrentvideomodedetails->m_dwWidthInPixels,
			pcurrentvideomodedetails->m_dwHeightInLines,
			pcurrentvideomodedetails->hoc, pcurrentvideomodedetails->voc,
			pcurrentvideomodedetails->m_bBPP,
			videoStd, &parameter)) {
				SetTvModeParameter(&parameter, pcurrentvideomodedetails->m_pbBaseAddressVideo);
		}
	}

	NVDisablePalette (&riva, 0);
	writeCrtNv (&riva, 0, 0x44, 0x03);
	NVInitGrSeq(&riva);
	writeCrtNv (&riva, 0, 0x44, 0x00);
	NVInitAttr(&riva,0);

	IoOutputByte(0x80d8, 4);  // ACPI IO thing seen in kernel, set to 4
	IoOutputByte(0x80d6, 5);  // ACPI IO thing seen in kernel, set to 4 or 5

	NVVertIntrEnabled (&riva,0);
	NVSetFBStart (&riva, 0, pcurrentvideomodedetails->m_dwFrameBufferStart);

	// FOCUS HACK FROM HERE
	if (1)
	{
		int i;
		unlockCrtNv(&riva,0);
		avType = DetectAvType();
		
		// FOCUS + PAL + COMPOSITE
		if ( (videoStd == PALBDGHI) && (avType == AV_COMPOSITE) ) 
		{
			I2CWriteWordtoRegister(0x6a,0xA0,0xf);  	// Blank the TV output
			wait_ms(2);
			for(i = 0; i < sizeof(focus_composite_pal)/2; i++) {
				I2CWriteBytetoRegister(0x6a,((focus_composite_pal[i]&0xff00)>>8),focus_composite_pal[i]&0xff);
				wait_us(800);
			}
			
			for(i = 0; i < (sizeof(nvMainRegs) / 4); i++) {
				MMIO_H_OUT32(riva.PMC, 0, nvMainRegs[i], nvRAMDACRegValues_focus_composite_pal[i]);
			}

			for(i = 0; i < sizeof(nvCRTRegs_focus_composite_pal); i++) {
				writeCrtNv(&riva, 0, i, nvCRTRegs_focus_composite_pal[i]);
			}
			wait_ms(2);
			I2CWriteWordtoRegister(0x6a,0xA0,0x200);        // UN Blank the TV 
			
		}
		
		// FOCUS + NTSC + COMPOSITE
		if ( (videoStd == NTSC) && (avType == AV_COMPOSITE) )
		{
			I2CWriteWordtoRegister(0x6a,0xA0,0xf);  	// Blank the TV output
			wait_ms(2);
			for(i = 0; i < sizeof(focus_composite_ntsc)/2; i++) {
				I2CWriteBytetoRegister(0x6a,(focus_composite_ntsc[i]&0xff00)>>8,focus_composite_ntsc[i]&0xff);
				wait_us(800);
			}			
			
			for(i = 0; i < (sizeof(nvMainRegs) / 4); i++) {
				MMIO_H_OUT32(riva.PMC, 0, nvMainRegs[i], nvRAMDACRegValues_focus_composite_ntsc[i]);
			}

			for(i = 0; i < sizeof(nvCRTRegs_focus_composite_ntsc); i++) {
				writeCrtNv(&riva, 0, i, nvCRTRegs_focus_composite_ntsc[i]);
			}
			wait_ms(2);
			I2CWriteWordtoRegister(0x6a,0xA0,0x200);        // UN Blank the TV 
			
		}		
	}
	
	
	IoOutputByte(0x80d3, 4);  // ACPI IO video enable REQUIRED <-- particularly crucial to get composite out

	pcurrentvideomodedetails->m_bFinalConexantA8 = 0x81;
	pcurrentvideomodedetails->m_bFinalConexantAA = 0x49;
	pcurrentvideomodedetails->m_bFinalConexantAC = 0x8c;
	// We dimm the Video OFF
	I2CTransmitWord(0x45, (0xa8<<8)|0);
	I2CTransmitWord(0x45, (0xaa<<8)|0);
	I2CTransmitWord(0x45, (0xac<<8)|0);

	NVWriteSeq(&riva, 0x01, 0x01);  /* reenable display */

        // We reenable the Video
        I2CTransmitWord(0x45, 0xa800 | pcurrentvideomodedetails->m_bFinalConexantA8);
        I2CTransmitWord(0x45, 0xaa00 | pcurrentvideomodedetails->m_bFinalConexantAA);
        I2CTransmitWord(0x45, 0xac00 | pcurrentvideomodedetails->m_bFinalConexantAC);

	I2CWriteWordtoRegister(0x6a, 0xa8, 0x100);
  	I2CWriteWordtoRegister(0x6a, 0xaa, 0x100);
        I2CWriteWordtoRegister(0x6a, 0xac, 0x100);
        	

}

static void NVSetFBStart (RIVA_HW_INST *riva, int head, DWORD dwFBStart) {

	MMIO_H_OUT32 (riva->PCRTC, head, 0x8000, dwFBStart);
	MMIO_H_OUT32 (riva->PMC, head, 0x8000, dwFBStart);

}

static void NVVertIntrEnabled (RIVA_HW_INST *riva, int head)
{
	MMIO_H_OUT32 (riva->PCRTC, head, 0x140, 0x1);
	MMIO_H_OUT32 (riva->PCRTC, head, 0x100, 0x1);
	MMIO_H_OUT32 (riva->PCRTC, head, 0x140, 1);
	MMIO_H_OUT32 (riva->PMC, head, 0x140, 0x1);
	MMIO_H_OUT32 (riva->PMC, head, 0x100, 0x1);
	MMIO_H_OUT32 (riva->PMC, head, 0x140, 1);

}

static inline void unlockCrtNv (RIVA_HW_INST *riva, int head)
{
  writeCrtNv (riva, head, 0x1f, 0x57); /* unlock extended registers */
}

static inline void lockCrtNv (RIVA_HW_INST *riva, int head)
{
  writeCrtNv (riva, head, 0x1f, 0x99); /* lock extended registers */
}


static void writeCrtNv (RIVA_HW_INST *riva, int head, int reg, BYTE val)
{
	VGA_WR08(riva->PCIO, CRT_INDEX(head), reg);
	VGA_WR08(riva->PCIO, CRT_DATA(head), val);
}

static void mapNvMem (RIVA_HW_INST *riva, BYTE *IOAddress)
{
	riva->PMC     = IOAddress+0x000000;
	riva->PFB     = IOAddress+0x100000;
	riva->PEXTDEV = IOAddress+0x101000;
	riva->PCRTC   = IOAddress+0x600000;
	riva->PCIO    = riva->PCRTC + 0x1000;
	riva->PVIO    = IOAddress+0x0C0000;
	riva->PRAMDAC = IOAddress+0x680000;
	riva->PDIO    = riva->PRAMDAC + 0x1000;
	riva->PVIDEO  = IOAddress+0x008000;
	riva->PTIMER  = IOAddress+0x009000;
}

static void NVDisablePalette (RIVA_HW_INST *riva, int head)
{
    volatile CARD8 tmp;

    tmp = VGA_RD08(riva->PCIO + head * HEAD, VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET);
    VGA_WR08(riva->PCIO + head * HEAD, VGA_ATTR_INDEX, 0x20);
}

static void NVWriteSeq(RIVA_HW_INST *riva, CARD8 index, CARD8 value)
{

	VGA_WR08(riva->PVIO, VGA_SEQ_INDEX, index);
	VGA_WR08(riva->PVIO, VGA_SEQ_DATA,  value);

}

static void NVWriteGr(RIVA_HW_INST *riva, CARD8 index, CARD8 value)
{
	VGA_WR08(riva->PVIO, VGA_GRAPH_INDEX, index);
	VGA_WR08(riva->PVIO, VGA_GRAPH_DATA,  value);
}

static void NVInitGrSeq (RIVA_HW_INST *riva)
{
	NVWriteSeq(riva, 0x00, 0x03);
	NVWriteSeq(riva, 0x01, 0x21);
	NVWriteSeq(riva, 0x02, 0x0f);
	NVWriteSeq(riva, 0x03, 0x00);
	NVWriteSeq(riva, 0x04, 0x06);
	NVWriteGr(riva, 0x00, 0x00);
	NVWriteGr(riva, 0x01, 0x00);
	NVWriteGr(riva, 0x02, 0x00);
	NVWriteGr(riva, 0x03, 0x00);
	NVWriteGr(riva, 0x04, 0x00);    /* depth != 1 */
	NVWriteGr(riva, 0x05, 0x40);    /* depth != 1 && depth != 4 */
	NVWriteGr(riva, 0x06, 0x05);
	NVWriteGr(riva, 0x07, 0x0f);
	NVWriteGr(riva, 0x08, 0xff);
}

static void NVWriteAttr(RIVA_HW_INST *riva, int head, CARD8 index, CARD8 value)
{
	MMIO_H_OUT8(riva->PCIO, head, VGA_ATTR_INDEX,  index);
	MMIO_H_OUT8(riva->PCIO, head, VGA_ATTR_DATA_W, value);
}

static void NVInitAttr (RIVA_HW_INST *riva, int head)
{
	NVWriteAttr(riva,0, 0, 0x01);
	NVWriteAttr(riva,0, 1, 0x02);
	NVWriteAttr(riva,0, 2, 0x03);
	NVWriteAttr(riva,0, 3, 0x04);
	NVWriteAttr(riva,0, 4, 0x05);
	NVWriteAttr(riva,0, 5, 0x06);
	NVWriteAttr(riva,0, 6, 0x07);
	NVWriteAttr(riva,0, 7, 0x08);
	NVWriteAttr(riva,0, 8, 0x09);
	NVWriteAttr(riva,0, 9, 0x0a);
	NVWriteAttr(riva,0, 10, 0x0b);
	NVWriteAttr(riva,0, 11, 0x0c);
	NVWriteAttr(riva,0, 12, 0x0d);
	NVWriteAttr(riva,0, 13, 0x0e);
	NVWriteAttr(riva,0, 14, 0x0f);
	NVWriteAttr(riva,0, 15, 0x01);
	NVWriteAttr(riva,0, 16, 0x4a);
	NVWriteAttr(riva,0, 17, 0x0f);
	NVWriteAttr(riva,0, 18, 0x00);
	NVWriteAttr(riva,0, 19, 0x00);
}



