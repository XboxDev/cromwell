#ifndef _BootVideo_H_
#define _BootVideo_H_

/* Standard VGA registers */
#define VGA_ATTR_INDEX          0x3C0
#define VGA_ATTR_DATA_W         0x3C0
#define VGA_ATTR_DATA_R         0x3C1
#define VGA_IN_STAT_0           0x3C2           /* read */
#define VGA_MISC_OUT_W          0x3C2           /* write */
#define VGA_ENABLE              0x3C3
#define VGA_SEQ_INDEX           0x3C4
#define VGA_SEQ_DATA            0x3C5
#define VGA_DAC_MASK            0x3C6
#define VGA_DAC_READ_ADDR       0x3C7
#define VGA_DAC_WRITE_ADDR      0x3C8
#define VGA_DAC_DATA            0x3C9
#define VGA_FEATURE_R           0x3CA           /* read */
#define VGA_MISC_OUT_R          0x3CC           /* read */
#define VGA_GRAPH_INDEX         0x3CE
#define VGA_GRAPH_DATA          0x3CF

#define VGA_IOBASE_MONO         0x3B0
#define VGA_IOBASE_COLOR        0x3D0

#define VGA_CRTC_INDEX_OFFSET   0x04
#define VGA_CRTC_DATA_OFFSET    0x05
#define VGA_IN_STAT_1_OFFSET    0x0A            /* read */
#define VGA_FEATURE_W_OFFSET    0x0A            /* write */

/* Little macro to construct bitmask for contiguous ranges of bits */

#define BITMASK(t,b) (((unsigned)(1U << (((t)-(b)+1)))-1)  << (b))
#define MASKEXPAND(mask) BITMASK(1?mask,0?mask)

/* Macro to set specific bitfields (mask has to be a macro x:y) ! */

#define SetBF(mask,value) ((value) << (0?mask))
#define GetBF(var,mask) (((unsigned)((var) & MASKEXPAND(mask))) >> (0?mask) )

#define MaskAndSetBF(var,mask,value) (var)=(((var)&(~MASKEXPAND(mask)) \
                                             | SetBF(mask,value)))

/* SetBitField: Move bit-range in 'from' to bit-range in 'to' */

#define SetBitField(value,from,to) SetBF(to, GetBF(value,from))
#define SetBitFlag(value,mask,to) ((value & mask) ? (1 << to) : 0)
#define SetBit(n) (1<<(n))
#define GetBit(value,n) ((value)&(1<<(n)))
#define GetBitFlag(value,from,mask) (GetBit(value,from) ? mask : 0)
#define Set8Bits(value) ((value)&0xff)

#define HEAD 0x2000

#define CARD8 BYTE
#define CARD32 DWORD

#define MMIO_IN8(base, offset) \
        *(volatile CARD8 *)(((CARD8*)(base)) + (offset))
#define MMIO_IN16(base, offset) \
        *(volatile CARD16 *)(void *)(((CARD8*)(base)) + (offset))
#define MMIO_IN32(base, offset) \
        *(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset))
#define MMIO_OUT8(base, offset, val) \
        *(volatile CARD8 *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_OUT16(base, offset, val) \
        *(volatile CARD16 *)(void *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_OUT32(base, offset, val) \
        *(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_ONB8(base, offset, val) MMIO_OUT8(base, offset, val)
#define MMIO_ONB16(base, offset, val) MMIO_OUT16(base, offset, val)
#define MMIO_ONB32(base, offset, val) MMIO_OUT32(base, offset, val)

#define MMIO_H_IN8(base,h,offset) MMIO_IN8(base,(offset)+(h)*HEAD)
#define MMIO_H_IN32(base,h,offset) MMIO_IN32(base,(offset)+(h)*HEAD)
#define MMIO_H_OUT8(base,h,offset,val) MMIO_OUT8(base,(offset)+(h)*HEAD,val)
#define MMIO_H_OUT32(base,h,offset,val) MMIO_OUT32(base,(offset)+(h)*HEAD,val)

#define MMIO_H_AND32(base,h,offset,val) MMIO_AND32(base,(offset)+(h)*HEAD,val)
#define MMIO_H_OR32(base,h,offset,val) MMIO_OR32(base,(offset)+(h)*HEAD,val)

#define MMIO_AND32(base, offset, val) \
        *(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset)) &= (val)
#define MMIO_OR32(base, offset, val) \
        *(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset)) |= (val)


/* these assume memory-mapped I/O, and not normal I/O space */
#define NV_WR08(p,i,d)  MMIO_OUT8((volatile void *)(p), (i), (d))
#define NV_RD08(p,i)    MMIO_IN8((volatile void *)(p), (i))
#define NV_WR16(p,i,d)  MMIO_OUT16((volatile void *)(p), (i), (d))
#define NV_RD16(p,i)    MMIO_IN16((volatile void *)(p), (i))
#define NV_WR32(p,i,d)  MMIO_OUT32((volatile void *)(p), (i), (d))
#define NV_RD32(p,i)    MMIO_IN32((volatile void *)(p), (i))

#define VGA_WR08(p,i,d) NV_WR08(p,i,d)
#define VGA_RD08(p,i)   NV_RD08(p,i)

#define CRT_INDEX(h) (0x3d4 + (h) * HEAD)
#define CRT_DATA(h)  (0x3d5 + (h) * HEAD)

#define NV_FLAG_DOUBLE_PIX    (1 << 1)
#define NV_FLAG_DOUBLE_SCAN   (1 << 0)

#define DEV_TELEVISION        (1 << 1)
#define DEV_FLATPANEL         (1 << 2)

typedef struct _riva_hw_inst
{
	/*
	* Non-FIFO registers.
	*/
	volatile BYTE *PCRTC;
	volatile BYTE *PRAMDAC;
	volatile BYTE *PFB;
	volatile BYTE *PFIFO;
	volatile BYTE *PGRAPH;
	volatile BYTE *PEXTDEV;
	volatile BYTE *PTIMER;
	volatile BYTE *PMC;
	volatile BYTE *PRAMIN;
	volatile BYTE *FIFO;
	volatile BYTE *CURSOR;
	volatile BYTE *CURSORPOS;
	volatile BYTE *VBLANKENABLE;
	volatile BYTE *VBLANK;

	volatile BYTE *PCIO;
	volatile BYTE *PVIO;
	volatile BYTE *PDIO;
	volatile BYTE *PVIDEO;
} RIVA_HW_INST;
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
	AV_HDTV,
	AV_COMPOSITE
} EAVTYPE;

// this struct contains all required parameter to define a mode

typedef struct {
  ULONG h_active;
  ULONG v_activei;
  ULONG v_linesi;
  ULONG h_clki;
  double clk_ratio;
  EVIDEOSTD nVideoStd;
} MODE_PARAMETER;

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
	DWORD m_dwALO;
	double m_TotalLinesOut;
	double m_dSecHsyncToBlankEnd;
} VID_STANDARD;

// and here is all the video timing for every standard

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
	VIDEO_MODE_1024x576,
	VIDEO_MODE_COUNT
};

typedef struct {
		// fill on entry
	int m_nVideoModeIndex; // fill on entry to BootVgaInitializationKernel(), eg, VIDEO_MODE_800x600
	BYTE m_fForceEncoderLumaAndChromaToZeroInitially; // fill on entry to BootVgaInitializationKernel(), 0=mode change visible immediately, !0=initially forced to black raster
	DWORD m_dwFrameBufferStart; // frame buffer start address, set to zero to use default
	BYTE m_fForceTvStandard; // true to use the TV standard set in m_bTvStandard, false to use EEPROM value
	BYTE * volatile m_pbBaseAddressVideo; // base address of video, usually 0xfd000000
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
	double hoc;
	double voc;
} CURRENT_VIDEO_MODE_DETAILS;

static const double dPllBaseClockFrequency = 13500000.0;
static const double dPllBasePeriod = (1.0/13500000.0);

///////// BootVgaInitialization.c

void BootVgaInitializationKernelNG(CURRENT_VIDEO_MODE_DETAILS * pcurrentvidemodedetails);

bool FindOverscanValues(
	long h_active,
	long v_activei,
	double hoc,
	double voc,
	EVIDEOSTD nVideoStd,
	MODE_PARAMETER* result
);

void CalcBlankings(const MODE_PARAMETER* m, 
	long* h_blanki, long* h_blanko, 
	long* v_blanki, 
	long* v_blanko, 
	long* vscale
);

void SetAutoParameter(const MODE_PARAMETER* mode, BYTE *pbRegs);

BYTE NvGetCrtc(BYTE * pbRegs, int nIndex);
void NvSetCrtc(BYTE * pbRegs, int nIndex, BYTE b);

EVIDEOSTD xbvDetectVideoStd(void);
void mapNvMem (RIVA_HW_INST *riva, BYTE *IOAddress);
void NVDisablePalette (RIVA_HW_INST *riva, int head);
void NVWriteSeq(RIVA_HW_INST *riva, CARD8 index, CARD8 value);
CARD8 NVReadSeq(RIVA_HW_INST *riva, CARD8 index);
void NVWriteGr(RIVA_HW_INST *riva, CARD8 index, CARD8 value);
CARD8 NVReadGr(RIVA_HW_INST *riva, CARD8 index);
void NVInitGrSeq (RIVA_HW_INST *riva);
void NVInitDac (RIVA_HW_INST *riva, int head);
void NVInitAttr (RIVA_HW_INST *riva, int head);
void andOrCrtNv (RIVA_HW_INST *riva, int head, int reg, BYTE and, BYTE or);
BYTE readCrtNv (RIVA_HW_INST *riva, int head, int reg);
inline void unlockCrtNv (RIVA_HW_INST *riva, int head);
inline void lockCrtNv (RIVA_HW_INST *riva, int head);
void andCrtNv (RIVA_HW_INST *riva, int head, int reg, BYTE val);
void orCrtNv (RIVA_HW_INST *riva, int head, int reg, BYTE val);
void writeCrtNv (RIVA_HW_INST *riva, int head, int reg, BYTE val);
void NVVertIntrEnabled (RIVA_HW_INST *riva, int head);
void NVSetFBStart (RIVA_HW_INST *riva, int head, DWORD dwFBStart);

#endif // _BootVideo_H_
