#ifndef VIDEOINITIALIZATION_H
#define VIDEOINITIALIZATION_H

//These two came from the kernel driver
typedef enum enumVideoStandards {
        TV_ENC_INVALID=-1,
        TV_ENC_NTSC=0,
        TV_ENC_NTSC60,
        TV_ENC_PALBDGHI,
        TV_ENC_PALN,
        TV_ENC_PALNC,
        TV_ENC_PALM,
        TV_ENC_PAL60
} xbox_tv_encoding;

typedef enum enumAvTypes {
        AV_INVALID=-1,
        AV_SCART_RGB,
        AV_SVIDEO,
        AV_VGA_SOG,
        AV_HDTV,
        AV_COMPOSITE,
        AV_VGA
} xbox_av_type;

typedef enum enumHdtvModes {
	HDTV_480p,
	HDTV_720p,
	HDTV_1080i
} xbox_hdtv_mode;

// this struct contains all required parameter to define a TV mode

typedef struct {
  long h_active;
  long v_activei;
  long v_linesi;
  long h_clki;
  long bpp;
  double clk_ratio;
  xbox_tv_encoding nVideoStd;
} TV_MODE_PARAMETER;

// this struct contains all required parameter to define a VGA mode

typedef struct {
	unsigned long xres;
	unsigned long yres;
	unsigned long hsyncstart;
	unsigned long htotal;
	unsigned long vsyncstart;
	unsigned long vtotal;
	unsigned long pixclock;
	unsigned long bpp;
} VGA_MODE_PARAMETER;

// this struct defines the final video timing
typedef struct {
	double m_dHzBurstFrequency;
	double m_dSecBurstStart;
	double m_dSecBurstEnd;
	double m_dSecHsyncWidth;
	double m_dSecHsyncPeriod;
	double m_dSecActiveBegin;
	double m_dSecImageCentre;
	double m_dSecBlankBeginToHsync;
	unsigned long m_dwALO;
	double m_TotalLinesOut;
	double m_dSecHsyncToBlankEnd;
} VID_STANDARD;

typedef struct {
	long xres;
	long crtchdispend;
	long nvhstart;
	long nvhtotal;
	long yres;
	long nvvstart;
	long crtcvstart;
	long crtcvtotal;
    	long nvvtotal;
	long pixelDepth;
} GPU_PARAMETER;

static const double dPllBaseClockFrequency = 13500000.0;
static const double dPllBasePeriod = (1.0/13500000.0);

///////// VideoInitialization.c

xbox_tv_encoding DetectVideoStd(void);
xbox_av_type DetectAvType(void);

int FindOverscanValues(
	long h_active,
	long v_activei,
	double hoc,
	double voc,
	long bpp,
	xbox_tv_encoding nVideoStd,
	TV_MODE_PARAMETER* result
);

void SetTvModeParameter(const TV_MODE_PARAMETER* mode, unsigned char *pbRegs);
void SetVgaHdtvModeParameter(const VGA_MODE_PARAMETER* mode, unsigned char *pbRegs);
void SetGPURegister(const GPU_PARAMETER* gpu, unsigned char *pbRegs);
#endif
