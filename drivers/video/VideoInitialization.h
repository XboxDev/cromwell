#ifndef VIDEOINITIALIZATION_H
#define VIDEOINITIALIZATION_H

// these are the various video standards supported

typedef enum enumVideoStandards {
	ILLEGAL=-1,
	NTSC=0,
	NTSC60,
	PALBDGHI,
	PALN,
	PALNC,
	PALM,
	PAL60
} EVIDEOSTD;

typedef enum enumAvTypes {
	AV_ILLEGAL=-1,
	AV_SCART_RGB,
	AV_SVIDEO,
	AV_VGA,
	AV_HDTV,
	AV_COMPOSITE
} EAVTYPE;

// this struct contains all required parameter to define a TV mode

typedef struct {
  long h_active;
  long v_activei;
  long v_linesi;
  long h_clki;
  long bpp;
  double clk_ratio;
  EVIDEOSTD nVideoStd;
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

// and here is all the video timing for every standard

static const VID_STANDARD vidstda[] = {
	{ 3579545.00, 0.0000053, 0.00000782, 0.0000047, 0.000063555, 0.0000094, 0.000035667, 0.0000015, 243, 262.5, 0.0000092 },
	{ 3579545.00, 0.0000053, 0.00000782, 0.0000047, 0.000064000, 0.0000094, 0.000035667, 0.0000015, 243, 262.5, 0.0000092 },
	{ 4433618.75, 0.0000056, 0.00000785, 0.0000047, 0.000064000, 0.0000105, 0.000036407, 0.0000015, 288, 312.5, 0.0000105 },
	{ 4433618.75, 0.0000056, 0.00000785, 0.0000047, 0.000064000, 0.0000094, 0.000035667, 0.0000015, 288, 312.5, 0.0000092 },
	{ 3582056.25, 0.0000056, 0.00000811, 0.0000047, 0.000064000, 0.0000105, 0.000036407, 0.0000015, 288, 312.5, 0.0000105 },
	{ 3575611.88, 0.0000058, 0.00000832, 0.0000047, 0.000063555, 0.0000094, 0.000035667, 0.0000015, 243, 262.5, 0.0000092 },
	{ 4433619.49, 0.0000053, 0.00000755, 0.0000047, 0.000063555, 0.0000105, 0.000036407, 0.0000015, 243, 262.5, 0.0000092 }
};

static const double dPllBaseClockFrequency = 13500000.0;
static const double dPllBasePeriod = (1.0/13500000.0);

///////// VideoInitialization.c

EVIDEOSTD DetectVideoStd(void);
EAVTYPE DetectAvType(void);

int FindOverscanValues(
	long h_active,
	long v_activei,
	double hoc,
	double voc,
	long bpp,
	EVIDEOSTD nVideoStd,
	TV_MODE_PARAMETER* result
);

void SetTvModeParameter(const TV_MODE_PARAMETER* mode, unsigned char *pbRegs);
void SetVgaModeParameter(const VGA_MODE_PARAMETER* mode, unsigned char *pbRegs);

#endif
