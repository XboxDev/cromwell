#ifndef audio_h
#define audio_h

typedef struct {
	WORD m_wTimeStartmS;
	WORD m_wTimeDurationmS;
	DWORD m_dwFrequency;
} SONG_NOTE;


typedef enum {
	AET_SINE=0,
	AET_NOISE
} AUDIO_ELEMENT_TYPES;

typedef void (*CALLBACK_AFTER_BLOCK)(void * pvoidAc97Device, void * pvoidaudioelement);


typedef struct _AUDIO_ELEMENT {  // represents a kind of generated sound, needs a derived object to be usable
	struct _AUDIO_ELEMENT * m_paudioelementNext;
	short m_sPanZeroIsAllLeft7FFFIsAllRight;
	AUDIO_ELEMENT_TYPES m_aetType;
	DWORD m_dwVolumeElementMaster7fff0000Max;
	DWORD m_dwVolumeAttackRate;
	DWORD m_dwVolumeAttackLimit;
	DWORD m_dwVolumeSustainRate;
	DWORD m_dwVolumeSustainLimit;
	DWORD m_dwVolumeDecayRate;
	BYTE m_bStageZeroIsAttack;
	void * m_pvPayload;
	DWORD m_dwPayload2;
	CALLBACK_AFTER_BLOCK m_callbackEveryBufferFill;
	DWORD m_dwCount48kHzSamplesRendered;
} AUDIO_ELEMENT;

typedef struct _AUDIO_ELEMENT_SINE {  // derived object composed from base object AUDIO_ELEMENT represents a sine wave with harmonics
	AUDIO_ELEMENT m_paudioelement;
	DWORD m_dwComputedFundamentalPhaseIncrementFor48kHzSamples;
	short m_saVolumePerHarmonicZeroIsNone7FFFIsFull[5]; // first entry is fundamental, then 2nd harmonic, etc
	DWORD m_dwPhaseAccumilator[5];
} AUDIO_ELEMENT_SINE;

typedef struct _AUDIO_ELEMENT_NOISE {  // derived object composed from base object AUDIO_ELEMENT represents a whitish noise source
	AUDIO_ELEMENT m_paudioelement;
	short m_sVolumeZeroIsNone7FFFIsFull;
	union {
		DWORD m_dwShifter;
		short m_saSamples[2];
	} shifter;
	short m_sLastSample;
} AUDIO_ELEMENT_NOISE;

typedef struct {  // descriptor from AC97 specification
	DWORD m_dwBufferStartAddress;
	WORD m_wBufferLengthInSamples;  // 0=no smaples
	WORD m_wBufferControl;  // b15=1=issue IRQ on completion, b14=1=last in stream
} AC97_DESCRIPTOR __attribute__ ((aligned (8)));


typedef struct {
	AC97_DESCRIPTOR m_aac97descriptorPcmSpdif[32];
	AC97_DESCRIPTOR m_aac97descriptorPcmOut[32];
	volatile DWORD * m_pdwMMIO;
	volatile DWORD m_dwCount48kHzSamplesRendered;
	volatile DWORD m_dwNextDescriptorMod31;
	AUDIO_ELEMENT * m_paudioelementFirst;
} AC97_DEVICE  __attribute__ ((aligned (8))) ;

void BootAudioInit(volatile AC97_DEVICE * pac97device);
void BootAudioInterrupt(volatile AC97_DEVICE * pac97device);
void BootAudioSilence(volatile AC97_DEVICE * pac97device);
void BootAudioOutBufferToDescriptor(volatile AC97_DEVICE * pac97device, DWORD * pdwBuffer, WORD wLengthInSamples, bool fFinal);
short BootAudioInterpolatedSine(DWORD dwPhase4GIs2PiRadians);
void BootAudioPlayDescriptors(volatile AC97_DEVICE * pac97device);
void BootAudioAttachAudioElement(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT * paudioelement);
void BootAudioDetachAudioElement(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT * paudioelement);
void ConstructAUDIO_ELEMENT_SINE(volatile AUDIO_ELEMENT_SINE *, int nFrequencyFundamental);
void DestructAUDIO_ELEMENT_SINE(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT_SINE * paes);
void ConstructAUDIO_ELEMENT_NOISE(volatile AUDIO_ELEMENT_NOISE *);
void DestructAUDIO_ELEMENT_NOISE(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT_NOISE * paen);
WORD BootAudioGetMixerSetting(volatile AC97_DEVICE * pac97device, int nSettingIndex);
void BootAudioSetMixerSetting(volatile AC97_DEVICE * pac97device, int nSettingIndex, WORD wValue);

#endif /* #ifndef audio_h */
