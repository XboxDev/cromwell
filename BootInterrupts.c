/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-01-13 andy@warmcat.com  Created from overgrown stuff in BootResetAction.c
                               Added support for all ints and exceptions with
	                             individual ISRs and debug messages
 */

#include "boot.h"
#ifdef XBE
#include "config-xbe.h"
#else
#include "config-rom.h"
#endif

volatile int nCountI2cinterrupts, nCountUnusedInterrupts, nCountUnusedInterruptsPic2, nCountInterruptsSmc, nCountInterruptsIde;
volatile bool fSeenPowerdown;

	// interrupt service stubs defined in BootStartup.S

extern void IntHandlerTimer0(void);
extern void IntHandlerI2C(void);
extern void IntHandlerSmc(void);
extern void IntHandlerIde(void);
extern void IntHandlerUnused(void);
extern void IntHandlerUnusedPic2(void);

extern void IntHandler1(void);
extern void IntHandler2(void);
extern void IntHandler3(void);
extern void IntHandler4(void);
extern void IntHandler5(void);
extern void IntHandler6(void);
extern void IntHandler7(void);
extern void IntHandler8(void);
extern void IntHandler9(void);
extern void IntHandler10(void);
extern void IntHandler13(void);
extern void IntHandler15(void);

extern void IntHandlerException0(void);
extern void IntHandlerException1(void);
extern void IntHandlerException2(void);
extern void IntHandlerException3(void);
extern void IntHandlerException4(void);
extern void IntHandlerException5(void);
extern void IntHandlerException6(void);
extern void IntHandlerException7(void);
extern void IntHandlerException8(void);
extern void IntHandlerException9(void);
extern void IntHandlerExceptionA(void);
extern void IntHandlerExceptionB(void);
extern void IntHandlerExceptionC(void);
extern void IntHandlerExceptionD(void);
extern void IntHandlerExceptionE(void);
extern void IntHandlerExceptionF(void);
extern void IntHandlerException10(void);

	// structure defining our ISRs

typedef struct {
	BYTE m_bInterruptCpu;
	DWORD m_dwpVector;
} ISR_PREP;

const ISR_PREP isrprep[] = {
	{ 0x00, (DWORD)IntHandlerException0 },
	{ 0x01, (DWORD)IntHandlerException1 },
	{ 0x02, (DWORD)IntHandlerException2 },
	{ 0x03, (DWORD)IntHandlerException3 },
	{ 0x04, (DWORD)IntHandlerException4 },
	{ 0x05, (DWORD)IntHandlerException5 },
	{ 0x06, (DWORD)IntHandlerException6 },
	{ 0x07, (DWORD)IntHandlerException7 },
	{ 0x08, (DWORD)IntHandlerException8 },
	{ 0x09, (DWORD)IntHandlerException9 },
	{ 0x0a, (DWORD)IntHandlerExceptionA },
	{ 0x0b, (DWORD)IntHandlerExceptionB },
	{ 0x0c, (DWORD)IntHandlerExceptionC },
	{ 0x0d, (DWORD)IntHandlerExceptionD },
	{ 0x0e, (DWORD)IntHandlerExceptionE },
	{ 0x0f, (DWORD)IntHandlerExceptionF },
	{ 0x10, (DWORD)IntHandlerException10 },

			// interrupts from PIC1

	{ 0x20, (DWORD)IntHandlerTimer0 },
	{ 0x21, (DWORD)IntHandler1 },
	{ 0x22, (DWORD)IntHandler2 },
	{ 0x23, (DWORD)IntHandler3 },
	{ 0x24, (DWORD)IntHandler4 },
	{ 0x25, (DWORD)IntHandler5 },
	{ 0x26, (DWORD)IntHandler6 },
	{ 0x27, (DWORD)IntHandler7 },

			// interrupts from PIC 2

	{ 0x70, (DWORD)IntHandler8 },
	{ 0x71, (DWORD)IntHandler9 },
	{ 0x72, (DWORD)IntHandler10 },
	{ 0x73, (DWORD)IntHandlerI2C },
	{ 0x74, (DWORD)IntHandlerSmc },
	{ 0x75, (DWORD)IntHandler13 },
	{ 0x76, (DWORD)IntHandlerIde },
	{ 0x77, (DWORD)IntHandler15 },

	{ 0, 0 }
};


void BootInterruptsWriteIdt(void) {
	volatile ts_pm_interrupt * ptspmi=(volatile ts_pm_interrupt *)(0xef000);  // ie, start of IDT area
	int n, n1=0;

			// init storage used by ISRs

	BIOS_TICK_COUNT=0;
	nCountI2cinterrupts=0;
	nCountUnusedInterrupts=0;
	nCountUnusedInterruptsPic2=0;
	nCountInterruptsSmc=0;
	nCountInterruptsIde=0;
	fSeenPowerdown=false;

		// set up default exception, interrupt vectors to dummy stubs

	for(n=0;n<0x100;n++) {  // have to do 256
		ptspmi[n].m_wSelector=0x10;
		ptspmi[n].m_wType=0x8e00;  // interrupt gate, 32-bit
		if(n==isrprep[n1].m_bInterruptCpu) {  // is it next on our prep list?  If so, stick it in
			ptspmi[n].m_wHandlerHighAddressLow16=(WORD)isrprep[n1].m_dwpVector;
			ptspmi[n].m_wHandlerLinearAddressHigh16=(WORD)(((DWORD)isrprep[n1].m_dwpVector)>>16);
			n1++;
		} else { // otherwise default handler (pretty useless, but will catch it)
			ptspmi[n].m_wHandlerHighAddressLow16=(WORD)IntHandlerUnused;
			ptspmi[n].m_wHandlerLinearAddressHigh16=(WORD)(((DWORD)IntHandlerUnused)>>16);
		}
	}

	// set up the Programmable Inetrrupt Controllers

	IoOutputByte(0x20, 0x15);  // master PIC setup
	IoOutputByte(0x21, 0x20);  // base interrupt vector address
	IoOutputByte(0x21, 0x04);  // am master, INT2 is hooked to slave
	IoOutputByte(0x21, 0x01);  // x86 mode, normal EOI
	IoOutputByte(0x21, 0x00);		// enable all ints

	IoOutputByte(0xa0, 0x15);	// slave PIC setup
	IoOutputByte(0xa1, 0x70);  // base interrupt vector address
	IoOutputByte(0xa1, 0x02);  // am slave, hooked to INT2 on master
	IoOutputByte(0xa1, 0x01);  // x86 mode normal EOI
	IoOutputByte(0xa1, 0x00);		// enable all ints

			// configure ACPI hardware to generate interrupt on PIC-chip pin6 action (via EXTSMI#)

	IoOutputByte(0x80c0, 0x08);  // from 2bl
	IoOutputByte(0x8004, IoInputByte(0x8004)|1);  // KERN: SCI enable == SCI interrupt generated
	IoOutputByte(0x8022, IoInputByte(0x8022)|2);  // KERN: Interrupt enable register, b1 RESERVED in AMD docs
	IoOutputByte(0x8002, IoInputByte(0x8002)|1);  // KERN: Enable SCI interrupt when timer status goes high
	IoOutputByte(0x8028, IoInputByte(0x8028)|1);  // KERN: setting readonly trap event???

			// enable interrupts

	 __asm __volatile (	" sti "	);  // and allow interrupt processing
   __asm__ __volatile__("push %ecx ; push %edx ; push %eax ; mov $0x1b, %cx ; rdmsr ; andl $0xfffff7ff, %eax ; wrmsr; pop %eax ; pop %edx ; pop %ecx" );

}



///////////////////////////////////////////
//
// ISRs


void IntHandlerCSmc(void)
{
	BYTE bStatus, nBit=0, bAvPackType;
	nCountInterruptsSmc++;
	bStatus=I2CTransmitByteGetReturn(0x10, 0x11); // Query PIC for interrupt reason
	while(nBit<7) {
		if(bStatus & 1) {
			switch(nBit) {

				case 0: // POWERDOWN EVENT
					bprintf("SMC Interrupt: Powerdown\n");
					fSeenPowerdown=true;
					break;

				case 1: // CDROM TRAY IS NOW CLOSED
					bprintf("SMC Interrupt: CDROM Tray now Closed\n");
					break;

				case 2: // CDROM TRAY IS STARTING OPENING
					I2CTransmitWord(0x10, 0x0d04);
					bprintf("SMC Interrupt: CDROM starting opening\n");
					break;

				case 3: // AV CABLE HAS BEEN PLUGGED IN
					bprintf("SMC Interrupt: AV cable plugged in\n");
					bAvPackType=BootVgaInitializationKernel(VIDEO_PREFERRED_LINES);
					BootVideoEnableOutput(bAvPackType);
					break;

				case 4: // AV CABLE HAS BEEN UNPLUGGED
					bprintf("SMC Interrupt: AV cable unplugged\n");
					break;

				case 5: // BUTTON PRESSED REQUESTING TRAY OPEN
					I2CTransmitWord(0x10, 0x0d04);
					I2CTransmitWord(0x10, 0x0c00);
					bprintf("SMC Interrupt: CDROM tray opening by Button press\n");
					break;

				case 6: // CDROM TRAY IS STARTING CLOSING
					bprintf("SMC Interrupt: CDROM tray starting closing\n");
					break;

				case 7: // UNKNOWN
					bprintf("SMC Interrupt: b7 Reason code\n");
					break;
			}
		}
		nBit++;
		bStatus>>=1;
	}
}

void IntHandlerCIde(void)
{
	bprintf("IDE Interrupt\n");
	nCountInterruptsIde++;
}

void IntHandlerCI2C(void)
{
	nCountI2cinterrupts++;
}
void IntHandlerUnusedC(void)
{
	bprintf("Unhandled Interrupt\n");
	nCountUnusedInterrupts++;
	while(1) ;
}


void IntHandlerUnusedC2(void)
{
	bprintf("Unhandled Interrupt 2\n");
	nCountUnusedInterruptsPic2++;
}


// this guy is getting called at 18.2Hz

void IntHandlerCTimer0(void)
{
	BIOS_TICK_COUNT++;

	if(BIOS_TICK_COUNT >8) {

		if(fSeenPowerdown) {
			if(VIDEO_LUMASCALING) {
				if(VIDEO_LUMASCALING<8) VIDEO_LUMASCALING=0; else VIDEO_LUMASCALING-=8;
				I2CTransmitWord(0x45, 0xac00|((VIDEO_LUMASCALING)&0xff)); // 8c
			}
			if(VIDEO_RSCALING) {
				if(VIDEO_RSCALING<16) VIDEO_RSCALING=0; else VIDEO_RSCALING-=16;
				I2CTransmitWord(0x45, 0xa800|((VIDEO_RSCALING)&0xff)); // 81
			}
			if(VIDEO_BSCALING) {
				if(VIDEO_BSCALING<18) VIDEO_BSCALING=0; else VIDEO_BSCALING-=18;
				I2CTransmitWord(0x45, 0xaa00|((VIDEO_BSCALING)&0xff)); // 49
			}
		} else {

			if(VIDEO_LUMASCALING<0x8c) {
				VIDEO_LUMASCALING+=5;
				I2CTransmitWord(0x45, 0xac00|((VIDEO_LUMASCALING)&0xff)); // 8c
			}
			if(VIDEO_RSCALING<0x81) {
				VIDEO_RSCALING+=3;
				I2CTransmitWord(0x45, 0xa800|((VIDEO_RSCALING)&0xff)); // 81
			}
			if(VIDEO_BSCALING<0x49) {
				VIDEO_BSCALING+=4;
				I2CTransmitWord(0x45, 0xaa00|((VIDEO_BSCALING)&0xff)); // 49
			}
		}
	}


	{
		char c=(BIOS_TICK_COUNT*5)&0xff;
		DWORD dw=c;
		if(c<0) dw=((-(int)c)-1);
		dw|=128; dw<<=24;
/*
		BootGimpVideoBlitBlend(
			(DWORD *)(FRAMEBUFFER_START+640*4*VIDEO_MARGINY+VIDEO_MARGINX*4), 640 * 4, (void *)&gimp_image,
			0x00000001 | dw, (DWORD *)&baBackdrop[0], 60*4
		);
*/
		BootVideoJpegBlitBlend(
			(DWORD *)(FRAMEBUFFER_START+640*4*VIDEO_MARGINY+VIDEO_MARGINX*4), 640 * 4, &jpegBackdrop,
			(DWORD *)(((BYTE *)jpegBackdrop.m_pBitmapData)+(640*480*jpegBackdrop.m_nBytesPerPixel)),
			0x00ff00ff | dw, (DWORD *)&baBackdrop[0], 60*4,
			54, 64
		);
	}
}

void IntHandler1C(void)
{
	bprintf("Interrupt 1\n");
}
void IntHandler2C(void)
{
	bprintf("Interrupt 2\n");
}
void IntHandler3C(void)
{
	bprintf("Interrupt 3\n");
}
void IntHandler4C(void)
{
	bprintf("Interrupt 4\n");
}
void IntHandler5C(void)
{
	bprintf("Interrupt 5\n");
}
void IntHandler6C(void)
{
	bprintf("Interrupt 6\n");
}
void IntHandler7C(void)
{
	bprintf("Interrupt 7\n");
}

void IntHandler8C(void)
{
	bprintf("Interrupt 8\n");
}

void IntHandler9C(void)
{
	bprintf("Interrupt 9\n");
}

void IntHandler10C(void)
{
	bprintf("Interrupt 10\n");
}

void IntHandler13C(void)
{
	bprintf("Interrupt 13\n");
}

void IntHandler15C(void)
{
	bprintf("Unhandled Interrupt 15\n");
}

void IntHandlerException0C(void) {	bprintf("CPU Exc: Divide by Zero\n");	while(1) ; }
void IntHandlerException1C(void) {	bprintf("CPU Exc: Single Step\n");	while(1) ; }
void IntHandlerException2C(void) {	bprintf("CPU Exc: NMI\n");	while(1) ; }
void IntHandlerException3C(void) {	bprintf("CPU Exc: Breakpoint\n");	while(1) ; }
void IntHandlerException4C(void) {	bprintf("CPU Exc: Overflow Trap\n");	while(1) ; }
void IntHandlerException5C(void) {	bprintf("CPU Exc: BOUND exceeded\n");	while(1) ; }
void IntHandlerException6C(void) {	bprintf("CPU Exc: Invalid Opcode\n");	while(1) ; }
void IntHandlerException7C(void) {	bprintf("CPU Exc: Coprocessor Absent\n");	while(1) ; }
void IntHandlerException8C(void) {	bprintf("CPU Exc: Double Fault\n");	while(1) ; }
void IntHandlerException9C(void) {	bprintf("CPU Exc: Copro Seg Overrun\n");	while(1) ; }
void IntHandlerExceptionAC(void) {	bprintf("CPU Exc: Invalid TSS\n");	while(1) ; }
void IntHandlerExceptionBC(void) {	bprintf("CPU Exc: Segment not present\n");	while(1) ; }
void IntHandlerExceptionCC(void) {	bprintf("CPU Exc: Stack Exception\n");	while(1) ; }
void IntHandlerExceptionDC(void) {	bprintf("CPU Exc: General Protection Fault\n");	while(1) ; }
void IntHandlerExceptionEC(void) {	bprintf("CPU Exc: Page Fault\n");	while(1) ; }
void IntHandlerExceptionFC(void) {	bprintf("CPU Exc: Reserved\n");	while(1) ; }
void IntHandlerException10C(void) {	bprintf("CPU Exc: Copro Error\n");	while(1) ; }


