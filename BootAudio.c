#include "boot.h"

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

// Andy@warmcat.com 2003-03-10:
//
// Xbox PC audio is an AC97 comptible audio controller in the MCPX chip
// http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/24467.pdf
// unlike standard AC97 all the regs appear as MMIO from 0xfec00000
// +0 - +7f = Mixer/Codec regs  <-- Wolfson Micro chip
// +100 - +17f = Busmaster regs
// I was unable to get anything other than 0x00 out of the Mixer/Codec region, except
// for the mandatory Manufacturer and Device ID words.
// this is disappointing as the Codec regs are probably needed to get S/PDIF working

// these are the audio buffers, I reuse the 4 x 2KDWORD buffers
// over the 32 descriptors specified in AC97

DWORD dwaaAudioBuffer[4][2048] __attribute__ ((aligned (8)));

// single quadrant of sine wave, from 0 to 0x7fff
// extra sample appended for interpolation function

const WORD waSineQuadrant256[] = {
	0x0, 0xc9, 0x192, 0x25b, 0x324, 0x3ed, 0x4b6, 0x57e,
	0x647, 0x710, 0x7d9, 0x8a1, 0x96a, 0xa32, 0xafb, 0xbc3,
	0xc8b, 0xd53, 0xe1b, 0xee3, 0xfab, 0x1072, 0x1139, 0x1200,
	0x12c7, 0x138e, 0x1455, 0x151b, 0x15e1, 0x16a7, 0x176d, 0x1833,
	0x18f8, 0x19bd, 0x1a82, 0x1b46, 0x1c0b, 0x1ccf, 0x1d93, 0x1e56,
	0x1f19, 0x1fdc, 0x209f, 0x2161, 0x2223, 0x22e4, 0x23a6, 0x2467,
	0x2527, 0x25e7, 0x26a7, 0x2767, 0x2826, 0x28e5, 0x29a3, 0x2a61,
	0x2b1e, 0x2bdb, 0x2c98, 0x2d54, 0x2e10, 0x2ecc, 0x2f86, 0x3041,
	0x30fb, 0x31b4, 0x326d, 0x3326, 0x33de, 0x3496, 0x354d, 0x3603,
	0x36b9, 0x376f, 0x3824, 0x38d8, 0x398c, 0x3a3f, 0x3af2, 0x3ba4,
	0x3c56, 0x3d07, 0x3db7, 0x3e67, 0x3f16, 0x3fc5, 0x4073, 0x4120,
	0x41cd, 0x4279, 0x4325, 0x43d0, 0x447a, 0x4523, 0x45cc, 0x4674,
	0x471c, 0x47c3, 0x4869, 0x490e, 0x49b3, 0x4a57, 0x4afa, 0x4b9d,
	0x4c3f, 0x4ce0, 0x4d80, 0x4e20, 0x4ebf, 0x4f5d, 0x4ffa, 0x5097,
	0x5133, 0x51ce, 0x5268, 0x5301, 0x539a, 0x5432, 0x54c9, 0x555f,
	0x55f4, 0x5689, 0x571d, 0x57b0, 0x5842, 0x58d3, 0x5963, 0x59f3,
	0x5a81, 0x5b0f, 0x5b9c, 0x5c28, 0x5cb3, 0x5d3d, 0x5dc6, 0x5e4f,
	0x5ed6, 0x5f5d, 0x5fe2, 0x6067, 0x60eb, 0x616e, 0x61f0, 0x6271,
	0x62f1, 0x6370, 0x63ee, 0x646b, 0x64e7, 0x6562, 0x65dd, 0x6656,
	0x66ce, 0x6745, 0x67bc, 0x6831, 0x68a5, 0x6919, 0x698b, 0x69fc,
	0x6a6c, 0x6adb, 0x6b4a, 0x6bb7, 0x6c23, 0x6c8e, 0x6cf8, 0x6d61,
	0x6dc9, 0x6e30, 0x6e95, 0x6efa, 0x6f5e, 0x6fc0, 0x7022, 0x7082,
	0x70e1, 0x7140, 0x719d, 0x71f9, 0x7254, 0x72ae, 0x7306, 0x735e,
	0x73b5, 0x740a, 0x745e, 0x74b1, 0x7503, 0x7554, 0x75a4, 0x75f3,
	0x7640, 0x768d, 0x76d8, 0x7722, 0x776b, 0x77b3, 0x77f9, 0x783f,
	0x7883, 0x78c6, 0x7908, 0x7949, 0x7989, 0x79c7, 0x7a04, 0x7a41,
	0x7a7c, 0x7ab5, 0x7aee, 0x7b25, 0x7b5c, 0x7b91, 0x7bc4, 0x7bf7,
	0x7c29, 0x7c59, 0x7c88, 0x7cb6, 0x7ce2, 0x7d0e, 0x7d38, 0x7d61,
	0x7d89, 0x7db0, 0x7dd5, 0x7df9, 0x7e1c, 0x7e3e, 0x7e5e, 0x7e7e,
	0x7e9c, 0x7eb9, 0x7ed4, 0x7eef, 0x7f08, 0x7f20, 0x7f37, 0x7f4c,
	0x7f61, 0x7f74, 0x7f86, 0x7f96, 0x7fa6, 0x7fb4, 0x7fc1, 0x7fcd,
	0x7fd7, 0x7fe0, 0x7fe8, 0x7fef, 0x7ff5, 0x7ff9, 0x7ffc, 0x7ffe,
	0x7fff // 257th
};

// returns signed 16-bit amplitude of sine of given phase.
// phase is 32-bit, 0 is 0 radians and 0xffffffff is almost 2 pi radians

short BootAudioInterpolatedSine(DWORD dwPhase4GIs2PiRadians)
{
	switch(dwPhase4GIs2PiRadians &0xc0000000) {  // reuse the one quadrant that we have in all four parts of 1 full sine wave
		case 0x00000000:  // 0 -> 1
			return
				waSineQuadrant256[(dwPhase4GIs2PiRadians>>22)&0xff]+
				(((waSineQuadrant256[((dwPhase4GIs2PiRadians>>22)&0xff)+1] - waSineQuadrant256[(dwPhase4GIs2PiRadians>>22)&0xff] ) // max step is < 8 bits
				* (dwPhase4GIs2PiRadians&0x3fffff) )/ 0x3fffff );

		case 0x40000000: // 1 -> 0
			return
				waSineQuadrant256[0xff-((dwPhase4GIs2PiRadians>>22)&0xff)]+
				(((waSineQuadrant256[(0xff-((dwPhase4GIs2PiRadians>>22)&0xff))+1] - waSineQuadrant256[0xff-((dwPhase4GIs2PiRadians>>22)&0xff)] ) // max step is < 8 bits
				* (0x3fffff-(dwPhase4GIs2PiRadians&0x3fffff)) )/ 0x3fffff );

		case 0x80000000:  // 0 -> -1
			return 0xffff-
				waSineQuadrant256[(dwPhase4GIs2PiRadians>>22)&0xff]+
				(((waSineQuadrant256[((dwPhase4GIs2PiRadians>>22)&0xff)+1] - waSineQuadrant256[(dwPhase4GIs2PiRadians>>22)&0xff] ) // max step is < 8 bits
				* (dwPhase4GIs2PiRadians&0x3fffff) )/ 0x3fffff );

		default:  // -1 -> 0
			return 0xffff-
				waSineQuadrant256[0xff-((dwPhase4GIs2PiRadians>>22)&0xff)]+
				(((waSineQuadrant256[(0xff-((dwPhase4GIs2PiRadians>>22)&0xff))+1] - waSineQuadrant256[0xff-((dwPhase4GIs2PiRadians>>22)&0xff)] ) // max step is < 8 bits
				* (0x3fffff-(dwPhase4GIs2PiRadians&0x3fffff)) )/ 0x3fffff );
	}
}


	// this is the main routine responsible for filling up an audio buffer with good stuff
	// this gets called from the Audio interrupt.  When its called after init, it means that the
	// ac97 controller has one complete buffer to go through before it runs out
	// this routine's job is to cook one new buffer just ahead of the remaining one(s) the ac97 controller is
	// burning through

void BootAudioFillNextBuffer(volatile AC97_DEVICE * pac97device)
{
	int n=0;
	DWORD * pdw=&dwaaAudioBuffer[(pac97device->m_dwNextDescriptorMod31)&3][0];
	DWORD dwMaximumAggregatedVolume=0;

	for(n=0;n< sizeof(dwaaAudioBuffer[0])/sizeof(DWORD);n++) {  // one buffer of 48kHz samples

		AUDIO_ELEMENT * pae=pac97device->m_paudioelementFirst;
		int nSummedSample[2]={0, 0};

		while(pae!=NULL) {  // go through all the audio element objects on the linked list

			switch(pae->m_aetType) {
				case AET_SINE:
					{
						AUDIO_ELEMENT_SINE * paes=(AUDIO_ELEMENT_SINE *)pae;
						int nHarmonic=0;
						for(nHarmonic=0;nHarmonic<sizeof(paes->m_saVolumePerHarmonicZeroIsNone7FFFIsFull)/sizeof(WORD);nHarmonic++) {  // for each harmonic we deal with

								// first track worst-case volume level from this harmonic for final scaling

							if(n==0) dwMaximumAggregatedVolume+=paes->m_saVolumePerHarmonicZeroIsNone7FFFIsFull[nHarmonic];

									// now add the sound for this harmonic if any

							if(paes->m_saVolumePerHarmonicZeroIsNone7FFFIsFull[nHarmonic]) {
								short s=(BootAudioInterpolatedSine(paes->m_dwPhaseAccumilator[nHarmonic])*paes->m_saVolumePerHarmonicZeroIsNone7FFFIsFull[nHarmonic])/0x7fff;
								s=(s * (paes->m_paudioelement.m_dwVolumeElementMaster7fff0000Max>>16))/0x7fff;  // apply element envelope
								nSummedSample[0]+=(s*(0x7fff-paes->m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight))/0x7fff;
								nSummedSample[1]+=(s*paes->m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight)/0x7fff;
								paes->m_dwPhaseAccumilator[nHarmonic]+=paes->m_dwComputedFundamentalPhaseIncrementFor48kHzSamples*(nHarmonic+1);
							}
						} // for each harmonic

					}
					break;
				case AET_NOISE:
					{
						AUDIO_ELEMENT_NOISE * paen=(AUDIO_ELEMENT_NOISE *)pae;
						short s=(short)(((paen->m_dwShifter&0xffff)*paen->m_sVolumeZeroIsNone7FFFIsFull)/0x7fff);
						s=(s * (paen->m_paudioelement.m_dwVolumeElementMaster7fff0000Max>>16))/0x7fff;  // apply element envelope
						nSummedSample[0]+=(s*(0x7fff-paen->m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight))/0x7fff;
						nSummedSample[1]+=(s*paen->m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight)/0x7fff;
						paen->m_dwShifter=(paen->m_dwShifter>>7)^((paen->m_dwShifter&0x310009020)>>2)^(paen->m_dwShifter<<19);
					}
					break;
			} // switch type

			switch(pae->m_bStageZeroIsAttack) {
				case 0: // attack
					pae->m_dwVolumeElementMaster7fff0000Max+=pae->m_dwVolumeAttackRate;
					if(pae->m_dwVolumeElementMaster7fff0000Max>pae->m_dwVolumeAttackLimit) pae->m_bStageZeroIsAttack++;
					break;
				case 1: // sustain
					if(pae->m_dwVolumeElementMaster7fff0000Max<pae->m_dwVolumeSustainRate) {
						pae->m_bStageZeroIsAttack++;
					} else {
						pae->m_dwVolumeElementMaster7fff0000Max-=pae->m_dwVolumeSustainRate;
					}
					break;
				case 2: // decay
					if(pae->m_dwVolumeElementMaster7fff0000Max<pae->m_dwVolumeDecayRate) {
						pae->m_bStageZeroIsAttack++;
					} else {
						pae->m_dwVolumeElementMaster7fff0000Max-=pae->m_dwVolumeDecayRate;
					}
					break;
				default:
					break;
			}


			pae=(AUDIO_ELEMENT *)pae->m_paudioelementNext;

		} // while there are more sound sources


			// now scale the sample for this place in the buffer according to our dynamic computed worst-case aggregate volume

		if(dwMaximumAggregatedVolume>0x7fff) {

			nSummedSample[0]=(nSummedSample[0] * 0x80) / (dwMaximumAggregatedVolume>>8);
			nSummedSample[1]=(nSummedSample[1] * 0x80) / (dwMaximumAggregatedVolume>>8);

			*pdw++ = ((nSummedSample[1]&0xffff)<<16)|(nSummedSample[0]&0xffff);
		} else {
			*pdw++ = ((nSummedSample[1]&0xffff)<<16)|(nSummedSample[0]&0xffff);

		}

	} // for each sample in buffer we are filling

	pac97device->m_dwCount48kHzSamplesRendered+=sizeof(dwaaAudioBuffer[0])/sizeof(DWORD);

	BootAudioOutBufferToDescriptor(
		pac97device,
		&dwaaAudioBuffer[pac97device->m_dwNextDescriptorMod31&3][0],
		sizeof(dwaaAudioBuffer[0])/sizeof(DWORD)*2,
		false
	);
}

void BootAudioInit(volatile AC97_DEVICE * pac97device)
{
	int n;

	pac97device->m_pdwMMIO=(DWORD *)0xfec00000;

	pac97device->m_dwCount48kHzSamplesRendered=0;
	pac97device->m_dwNextDescriptorMod31=0;
	pac97device->m_paudioelementFirst=NULL; // no audio elements to start with

	memset((void *)&pac97device->m_aac97descriptorPcmIn[0], 0, sizeof(pac97device->m_aac97descriptorPcmIn));
	memset((void *)&pac97device->m_aac97descriptorPcmOut[0], 0, sizeof(pac97device->m_aac97descriptorPcmOut));
	memset((void *)&pac97device->m_aac97descriptorPcmMic[0], 0, sizeof(pac97device->m_aac97descriptorPcmMic));

	pac97device->m_pdwMMIO[0x108>>2]=0x02000000; // reset, pause
	pac97device->m_pdwMMIO[0x118>>2]=0x1e000000; // reset, pause, allow interrupts
	pac97device->m_pdwMMIO[0x128>>2]=0x02000000; // reset, pause

	pac97device->m_pdwMMIO[0x12C>>2]=0x31; // allow interrupts, reset AC97 link
	for(n=0;n<100000;n++) ;  // wait >=1mS
	pac97device->m_pdwMMIO[0x12C>>2]=0x33; // allow interrupts, no reset

	pac97device->m_pdwMMIO[0x100>>2]=(DWORD)&pac97device->m_aac97descriptorPcmIn[0];
	pac97device->m_pdwMMIO[0x110>>2]=(DWORD)&pac97device->m_aac97descriptorPcmOut[0];
	pac97device->m_pdwMMIO[0x120>>2]=(DWORD)&pac97device->m_aac97descriptorPcmMic[0];

		// paused after this

	BootAudioSilence(pac97device);

		// prepare several buffers full of audio to start
		// we will get an interrupt after the first one is empty and can prepare the fourth while the second plays

	BootAudioFillNextBuffer(pac97device);
	BootAudioFillNextBuffer(pac97device);
	BootAudioFillNextBuffer(pac97device);
}

void BootAudioAttachAudioElement(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT * paudioelement)
{
	paudioelement->m_paudioelementNext=pac97device->m_paudioelementFirst;
	pac97device->m_paudioelementFirst=paudioelement;
}

void BootAudioDetachAudioElement(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT * paudioelement)
{
	AUDIO_ELEMENT * pae=pac97device->m_paudioelementFirst;
	AUDIO_ELEMENT ** ppae=(AUDIO_ELEMENT **)&pac97device->m_paudioelementFirst;
	while(pae) {
		if(pae==paudioelement) {
			*ppae=(AUDIO_ELEMENT *)pae->m_paudioelementNext;
			return;
		}
		ppae=&pae->m_paudioelementNext;
		pae=pae->m_paudioelementNext;
	}
}

// creates a default sine element with all harmonic volumes at zero and centred pan
// everything else is set up as best as it can

void ConstructAUDIO_ELEMENT_SINE(volatile AUDIO_ELEMENT_SINE * paes, int nFrequencyFundamental)
{
	memset((void *)paes, 0, sizeof(AUDIO_ELEMENT_SINE));

	paes->m_paudioelement.m_dwVolumeElementMaster7fff0000Max=0;
	paes->m_paudioelement.m_dwVolumeAttackRate=0x100000;
	paes->m_paudioelement.m_dwVolumeAttackLimit=0x7fffffff;
	paes->m_paudioelement.m_dwVolumeDecayRate=0x10000;

	paes->m_paudioelement.m_paudioelementNext=NULL;
	paes->m_dwComputedFundamentalPhaseIncrementFor48kHzSamples=0x10000 * ((nFrequencyFundamental <<16)/48000);
	paes->m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight=0x3fff;
	paes->m_paudioelement.m_aetType=AET_SINE;

}

void DestructAUDIO_ELEMENT_SINE(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT_SINE * paes)
{
	BootAudioDetachAudioElement(pac97device, (AUDIO_ELEMENT *)paes);
}

// creates a default sine element with all harmonic volumes at zero and centred pan
// everything else is set up as best as it can

void ConstructAUDIO_ELEMENT_NOISE(volatile AUDIO_ELEMENT_NOISE * paen)
{
	memset((void *)paen, 0, sizeof(AUDIO_ELEMENT_NOISE));

	paen->m_paudioelement.m_paudioelementNext=NULL;
	paen->m_paudioelement.m_sPanZeroIsAllLeft7FFFIsAllRight=0x3fff;
	paen->m_paudioelement.m_aetType=AET_NOISE;
	paen->m_sVolumeZeroIsNone7FFFIsFull=0;

	paen->m_dwShifter=0;
}

void DestructAUDIO_ELEMENT_NOISE(volatile AC97_DEVICE * pac97device, AUDIO_ELEMENT_NOISE * paen)
{
	BootAudioDetachAudioElement(pac97device, (AUDIO_ELEMENT *)paen);
}



void BootAudioPlayDescriptors(volatile AC97_DEVICE * pac97device)
{
	pac97device->m_pdwMMIO[0x118>>2]=(DWORD)0x1d000000; // PCM out - run, allow interrupts
}


void BootAudioSilence(volatile AC97_DEVICE * pac97device)
{
	pac97device->m_pdwMMIO[0x118>>2]=(DWORD)0x1c000000; // PCM out - PAUSE, allow interrupts
}


void BootAudioOutBufferToDescriptor(volatile AC97_DEVICE * pac97device, DWORD * pdwBuffer, WORD wLengthInSamples, bool fFinal)
{
	volatile BYTE * pb=(BYTE *)pac97device->m_pdwMMIO;
	WORD w=0x8000;
//	pb[0x11b]&=~1; // disable busmaster
//	while(!(pb[0x116]&1)) ;  // wait for everything to stop

	pac97device->m_aac97descriptorPcmOut[pac97device->m_dwNextDescriptorMod31].m_dwBufferStartAddress=(DWORD)pdwBuffer;
	pac97device->m_aac97descriptorPcmOut[pac97device->m_dwNextDescriptorMod31].m_wBufferLengthInSamples=wLengthInSamples;
	if(fFinal) w|=0x4000;
	pac97device->m_aac97descriptorPcmOut[pac97device->m_dwNextDescriptorMod31].m_wBufferControl=w;
	pb[0x115]=(BYTE)pac97device->m_dwNextDescriptorMod31; // set last active descriptor

	pac97device->m_dwNextDescriptorMod31 = (pac97device->m_dwNextDescriptorMod31 +1 ) & 0x1f;
//	pb[0x11b]|=1; // enable busmaster
}


	// service audio interrupt

void BootAudioInterrupt(volatile AC97_DEVICE * pac97device)
{
	volatile BYTE * pb=(BYTE *)pac97device->m_pdwMMIO;

//	bprintf("   IRQ sees descriptor %d as current (%x)\n", pb[0x114], pac97device);

	BootPciInterruptEnable();  // reenable interrupts - will NOT reneter because we did not clear IRQ source yet
		// make rentrant because the VSYNC ISR was getting held off too long

		// get next descriptor ready
	if(pb[0x116]&8) {
		BootAudioFillNextBuffer(pac97device);
	}
	if(pb[0x116]&0x10) {
		bprintf("Fifo overrun\n");
	}

		// seen b2 set too indicating DMA finished last descriptor before we made a new one
		// sonly saw this with 8192 DWORD+ buffers, they take too long to cook.

	pb[0x116]=0xff; // clear all int sources

}
