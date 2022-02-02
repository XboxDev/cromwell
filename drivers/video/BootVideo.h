#ifndef _BootVideo_H_
#define _BootVideo_H_

enum {
	VIDEO_MODE_UNKNOWN=-1,
	VIDEO_MODE_640x480=0,
	VIDEO_MODE_640x576,
	VIDEO_MODE_720x576,
	VIDEO_MODE_800x600,
	VIDEO_MODE_1024x576,
	VIDEO_MODE_COUNT
};

typedef struct {
		// fill on entry
	int m_nVideoModeIndex; // fill on entry to BootVgaInitializationKernel(), eg, VIDEO_MODE_800x600
	u8 m_fForceEncoderLumaAndChromaToZeroInitially; // fill on entry to BootVgaInitializationKernel(), 0=mode change visible immediately, !0=initially forced to black raster
	u32 m_dwFrameBufferStart; // frame buffer start address, set to zero to use default
	u8 * volatile m_pbBaseAddressVideo; // base address of video, usually 0xfd000000
		// filled on exit
	u32 width; // everything else filled by BootVgaInitializationKernel() on return
	u32 height;
	u32 xmargin;
	u32 ymargin;
	u8 m_bAvPack;
	u32 m_dwVideoFadeupTimer;
	double hoc;
	double voc;
	u8 m_bBPP;
} CURRENT_VIDEO_MODE_DETAILS;

void BootVgaInitializationKernelNG(CURRENT_VIDEO_MODE_DETAILS * pvmode);


int VideoDumpAddressAndData(u32 dwAds, const u8 * baData, u32 dwCountBytesUsable);
char *VideoEncoderName(void);
char *AvCableName(void);

#endif // _BootVideo_H_
