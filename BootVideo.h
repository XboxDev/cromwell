enum {
	TV_ENCODING_PAL=0,
	TV_ENCODING_NTSC,
	TV_ENCODING_COUNT
};

enum {
	VIDEO_MODE_UNKNOWN=-1,
	VIDEO_MODE_640x480=0,
	VIDEO_MODE_640x576,
	VIDEO_MODE_720x576,
	VIDEO_MODE_800x600,
	VIDEO_MODE_COUNT
};

typedef struct {
	DWORD m_dwaAddressesNv[0x3c];
	DWORD m_dwaVideoModeNv[TV_ENCODING_COUNT][VIDEO_MODE_COUNT][0x3c];
	BYTE m_baVideoModeCrtc[TV_ENCODING_COUNT][VIDEO_MODE_COUNT][0x3F];
	BYTE m_baVideoModeConexant[TV_ENCODING_COUNT][VIDEO_MODE_COUNT][0x69];
} VIDEO_MODE_TABLES;


typedef struct {
		// fill on entry
	int m_nVideoModeIndex; // fill on entry to BootVgaInitializationKernel(), eg, VIDEO_MODE_800x600
	BYTE m_fForceEncoderLumaAndChromaToZeroInitially; // fill on entry to BootVgaInitializationKernel(), 0=mode change visible immediately, !0=initially forced to black raster
	volatile BYTE * m_pbBaseAddressVideo; // base address of video, usually 0xfd000000
		// filled on exit
	DWORD m_dwWidthInPixels; // everything else filled by BootVgaInitializationKernel() on return
	DWORD m_dwHeightInLines;
	DWORD m_dwMarginXInPixelsRecommended;
	DWORD m_dwMarginYInLinesRecommended;
	BYTE m_bTvStandard;
	BYTE m_bAvPack;
	BYTE m_bFinalConexantA8;
	BYTE m_bFinalConexantAA;
	BYTE m_bFinalConexantAC;
	DWORD m_dwVideoFadeupTimer;
} CURRENT_VIDEO_MODE_DETAILS;

///////// BootVgaInitialization.c

void BootVgaInitializationKernel(CURRENT_VIDEO_MODE_DETAILS * pcurrentvideomodedetails);  // returns AV pack index, call with 480 or 576

