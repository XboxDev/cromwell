/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

 */

#include "boot.h"
#include "config.h"
#include "cpu.h"

volatile int nCountI2cinterrupts, nCountUnusedInterrupts, nCountUnusedInterruptsPic2, nCountInterruptsSmc, nCountInterruptsIde;
volatile bool fSeenPowerdown;
volatile TRAY_STATE traystate;
unsigned int wait_ms_time;

volatile int nInteruptable = 0;

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
	u8 m_bInterruptCpu;
	u32 m_dwpVector;
} ISR_PREP;

const ISR_PREP isrprep[] = {
	{ 0x00, (u32)IntHandlerException0 },
	{ 0x01, (u32)IntHandlerException1 },
	{ 0x02, (u32)IntHandlerException2 },
	{ 0x03, (u32)IntHandlerException3 },
	{ 0x04, (u32)IntHandlerException4 },
	{ 0x05, (u32)IntHandlerException5 },
	{ 0x06, (u32)IntHandlerException6 },
	{ 0x07, (u32)IntHandlerException7 },
	{ 0x08, (u32)IntHandlerException8 },
	{ 0x09, (u32)IntHandlerException9 },
	{ 0x0a, (u32)IntHandlerExceptionA },
	{ 0x0b, (u32)IntHandlerExceptionB },
	{ 0x0c, (u32)IntHandlerExceptionC },
	{ 0x0d, (u32)IntHandlerExceptionD },
	{ 0x0e, (u32)IntHandlerExceptionE },
	{ 0x0f, (u32)IntHandlerExceptionF },
	{ 0x10, (u32)IntHandlerException10 },

			// interrupts from PIC1

	{ 0x20, (u32)IntHandlerTimer0 },
	{ 0x21, (u32)IntHandler1 },
	{ 0x22, (u32)IntHandler2 },
	{ 0x23, (u32)IntHandler3 },
	{ 0x24, (u32)IntHandler4 },
	{ 0x25, (u32)IntHandler5 },
	{ 0x26, (u32)IntHandler6 },
	{ 0x27, (u32)IntHandler7 },

			// interrupts from PIC 2

	{ 0x70, (u32)IntHandler8 },
	{ 0x71, (u32)IntHandler9 },
	{ 0x72, (u32)IntHandler10 },
	{ 0x73, (u32)IntHandlerI2C },
	{ 0x74, (u32)IntHandlerSmc },
	{ 0x75, (u32)IntHandler13 },
	{ 0x76, (u32)IntHandlerIde },
	{ 0x77, (u32)IntHandler15 },

	{ 0, 0 }
};



void wait_smalldelay(void) {
	wait_us(80);
}

void wait_us(u32 ticks) {
        
	/*
	  	32 Bit range = 1200 sec ! => 20 min
		1. sec = 0x369E99
		1 ms =  3579,545
					
	*/
	
	u32 COUNT_start;
	u32 temp;
	u32 COUNT_TO;
	u32 HH;
	
	// Maximum Input range
	if (ticks>(1200*1000)) ticks = 1200*1000;
	
	COUNT_TO = (u32) ((float)(ticks*3.579545));
	COUNT_start = IoInputDword(0x8008);	

	while(1) {

		// Reads out the System timer
		HH = IoInputDword(0x8008);		
		temp = HH-COUNT_start;
		// We reached the counter
		if (temp>COUNT_TO) break;
	
	};
	

}

void wait_ms(u32 ticks) {
        
	/*
	  	32 Bit range = 1200 sec ! => 20 min
		1. sec = 0x369E99
		1 ms =  3579,545
					
	*/
	
	u32 COUNT_start;
	u32 temp;
	u32 COUNT_TO;
	u32 HH;
	
	// Maximum Input range
	if (ticks>(1200*1000)) ticks = 1200*1000;
	
	COUNT_TO = (u32) ((float)(ticks*3579.545));
	COUNT_start = IoInputDword(0x8008);	

	while(1) {
		// Reads out the System timer
		HH = IoInputDword(0x8008);		
		temp = HH-COUNT_start;
		// We reached the counter
		if (temp>COUNT_TO) break;
	};
	

}

void BootInterruptsWriteIdt() {

	volatile ts_pm_interrupt * ptspmi=(volatile ts_pm_interrupt *)(0xb0000);  // ie, start of IDT area
	int n, n1=0;

	// init storage used by ISRs

	VIDEO_VSYNC_POSITION=0;
	BIOS_TICK_COUNT=0;
	VIDEO_VSYNC_DIR=0;
	nCountI2cinterrupts=0;
	nCountUnusedInterrupts=0;
	nCountUnusedInterruptsPic2=0;
	nCountInterruptsSmc=0;
	nCountInterruptsIde=0;
	fSeenPowerdown=false;
	traystate=ETS_OPEN_OR_OPENING;
	VIDEO_LUMASCALING=0;
	VIDEO_RSCALING=0;
	VIDEO_BSCALING=0;

		// set up default exception, interrupt vectors to dummy stubs

	for(n=0;n<0x100;n++) {  // have to do 256
		ptspmi[n].m_wSelector=0x10;
		ptspmi[n].m_wType=0x8e00;  // interrupt gate, 32-bit
		if(n==isrprep[n1].m_bInterruptCpu) {  // is it next on our prep list?  If so, stick it in
			ptspmi[n].m_wHandlerHighAddressLow16=(u16)isrprep[n1].m_dwpVector;
			ptspmi[n].m_wHandlerLinearAddressHigh16=(u16)(((u32)isrprep[n1].m_dwpVector)>>16);
			n1++;
		} else { // otherwise default handler (pretty useless, but will catch it)
			ptspmi[n].m_wHandlerHighAddressLow16=(u16)IntHandlerUnused;
			ptspmi[n].m_wHandlerLinearAddressHigh16=(u16)(((u32)IntHandlerUnused)>>16);
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

	if (cromwell_config==CROMWELL) 	IoOutputByte(0xa1, 0x00);	// enable int14(IDE) int12(SMI)
	else IoOutputByte(0xa1, 0xaf);

	// enable interrupts
	intel_interrupts_on();
}



///////////////////////////////////////////
//
// ISRs


void IntHandlerCSmc(void)
{
	u8 bStatus, nBit=0;
        unsigned int temp;
        u8 temp_AV_mode;
        
	nCountInterruptsSmc++;
   
        
	temp = IoInputWord(0x8000);
	if (temp!=0x0) {
		IoOutputWord(0x8000,temp);
		//printk("System Timer wants to sleep we kill him");
	//	return;
		}
        
   
	
	bStatus=I2CTransmitByteGetReturn(0x10, 0x11); // Query PIC for interrupt reason
	
	// we do nothing, if there is not Interrupt reason
	if (bStatus==0x0) return;
	
	while(nBit<7) {
		if(bStatus & 1) {
			u8 b=0x04;
			       switch(nBit) {
				case 0: // POWERDOWN EVENT
					bprintf("SMC Interrupt %d: Powerdown\n", nCountInterruptsSmc);
					I2CTransmitWord(0x10, 0x0200);
					I2CTransmitWord(0x10, 0x0100|b);
					I2CTransmitWord(0x10, 0x0500|b);
					I2CTransmitWord(0x10, 0x0600|b);
					I2CTransmitWord(0x10, 0x0900|b);
					I2CTransmitWord(0x10, 0x0a00|b);
					I2CTransmitWord(0x10, 0x0b00|b);
					I2CTransmitWord(0x10, 0x0d00|b);
					I2CTransmitWord(0x10, 0x0e00|b);
					I2CTransmitWord(0x10, 0x0f00|b);
					I2CTransmitWord(0x10, 0x1000|b);
					I2CTransmitWord(0x10, 0x1200|b);
					I2CTransmitWord(0x10, 0x1300|b);
					I2CTransmitWord(0x10, 0x1400|b);
					I2CTransmitWord(0x10, 0x1500|b);
					I2CTransmitWord(0x10, 0x1600|b);
					I2CTransmitWord(0x10, 0x1700|b);
					I2CTransmitWord(0x10, 0x1800|b);
					fSeenPowerdown=true;
					break;

				case 1: // CDROM TRAY IS NOW CLOSED
					traystate=ETS_CLOSED;
					bprintf("SMC Interrupt %d: CDROM Tray now Closed\n", nCountInterruptsSmc);
					DVD_TRAY_STATE = DVD_CLOSED;
					break;

				case 2: // CDROM TRAY IS STARTING OPENING
					traystate=ETS_OPEN_OR_OPENING;
					DVD_TRAY_STATE = DVD_OPENING;
					I2CTransmitWord(0x10, 0x0d02);
					bprintf("SMC Interrupt %d: CDROM starting opening\n", nCountInterruptsSmc);
					break;

				case 3: // AV CABLE HAS BEEN PLUGGED IN
					       
					temp_AV_mode =I2CTransmitByteGetReturn(0x10, 0x04);
					// Compare to global variable
					if (VIDEO_AV_MODE != temp_AV_mode ) {
						VIDEO_AV_MODE = 0xff;
						wait_ms(30);
						VIDEO_AV_MODE = temp_AV_mode;
						BootVgaInitializationKernelNG((CURRENT_VIDEO_MODE_DETAILS *)&vmode);
						wait_ms(200);
						BootVgaInitializationKernelNG((CURRENT_VIDEO_MODE_DETAILS *)&vmode);
						
					}
					break;

				case 4: // AV CABLE HAS BEEN UNPLUGGED
					bprintf("SMC Interrupt %d: AV cable unplugged\n", nCountInterruptsSmc);
					VIDEO_AV_MODE=0xff;
					//vmode.m_bAvPack=0xff;
					break;

				case 5: // BUTTON PRESSED REQUESTING TRAY OPEN
					traystate=ETS_OPEN_OR_OPENING;
					I2CTransmitWord(0x10, 0x0d04);
					I2CTransmitWord(0x10, 0x0c00);
					bprintf("SMC Interrupt %d: CDROM tray opening by Button press\n", nCountInterruptsSmc);
					bStatus&=~0x02; // kill possibility of conflicting closing report
					break;

				case 6: // CDROM TRAY IS STARTING CLOSING
					traystate=ETS_CLOSING;
					DVD_TRAY_STATE = DVD_CLOSING;
					bprintf("SMC Interrupt %d: CDROM tray starting closing\n", nCountInterruptsSmc);
					break;

				case 7: // UNKNOWN
					bprintf("SMC Interrupt %d: b7 Reason code\n", nCountInterruptsSmc);
					break;
			}
		}
		nBit++;
		bStatus>>=1;
	}
}

void IntHandlerCIde(void)
{
	if(!nInteruptable) return;
	IoInputByte(0x1f7);
	bprintf("IDE Interrupt\n");
	nCountInterruptsIde++;
}

void IntHandlerCI2C(void)
{
	if(!nInteruptable) return;
	nCountI2cinterrupts++;
}
void IntHandlerUnusedC(void)
{
	if(!nInteruptable) return;
	bprintf("Unhandled Interrupt\n");
	//printk("Unhandled Interrupt");
	nCountUnusedInterrupts++;
	//while(1) ;
}


void IntHandlerUnusedC2(void)
{
	if(!nInteruptable) return;

	bprintf("Unhandled Interrupt 2\n");
	nCountUnusedInterruptsPic2++;
}



// this guy is getting called at 18.2Hz

void IntHandlerCTimer0(void)
{
	BIOS_TICK_COUNT++;
}
 
 
// USB interrupt

void IntHandler1C(void)
{
	// Interrupt for OHCI controller located on 0xfed00000
	if(!nInteruptable) return;
	USBGetEvents();
}
void IntHandler9C(void)
{
      	// Interrupt for OHCI controller located on 0xfed08000
	if(!nInteruptable) return;
	USBGetEvents();
}


void IntHandler2C(void)
{
	if(!nInteruptable) return;
	bprintf("Interrupt 2\n");
}

void IntHandler3VsyncC(void)  // video VSYNC
{
	*((volatile u32 *)0xfd600100)=0x1;  // clear VSYNC int
} 


void IntHandler4C(void)
{
	if(!nInteruptable) return;
	bprintf("Interrupt 4\n");
}
void IntHandler5C(void)
{
	if(!nInteruptable) return;
	bprintf("Interrupt 5\n");
}

void IntHandler6C(void) {

	bprintf("Interrupt 6\n");
}

void IntHandler7C(void)
{
	if(!nInteruptable) return;
	bprintf("Interrupt 7\n");
}

void IntHandler8C(void)
{
	bprintf("Interrupt 8\n");
}


void IntHandler10C(void)
{
	if(!nInteruptable) return;
	bprintf("Interrupt 10\n");
}

void IntHandler13C(void)
{
	bprintf("Interrupt 13\n");
}

void IntHandler15C(void)
{
	if(!nInteruptable) return;
	bprintf("Unhandled Interrupt 15\n");
}

//void IntHandlerException0C(void) {	bprintf("CPU Exc: Divide by Zero\n");	while(1) ; }
void IntHandlerException0C(void) {	bprintf("CPU Exc: Divide by Zero\n");}
void IntHandlerException1C(void) {	bprintf("CPU Exc: Single Step\n");	while(1) ; }
void IntHandlerException2C(void) {	bprintf("CPU Exc: NMI\n");	while(1) ; }
void IntHandlerException3C(void) {	bprintf("CPU Exc: Breakpoint\n");	while(1) ; }
void IntHandlerException4C(void) {	bprintf("CPU Exc: Overflow Trap\n");	while(1) ; }
void IntHandlerException5C(void) {	bprintf("CPU Exc: BOUND exceeded\n");	while(1) ; }
void IntHandlerException6C(void) {
	u32 dwEbp=0;
	bprintf("CPU Exc: Invalid Opcode\n");
		__asm__ __volatile__ ( " mov %%esp, %%eax\n " : "=a" (dwEbp) );
	bprintf("   %08lX:%08lX\n", *((volatile u32 *)(dwEbp+0x48)), *((volatile u32 *)(dwEbp+0x44)));
	while(1) ;
}
//void IntHandlerException7C(void) {	bprintf("CPU Exc: Coprocessor Absent\n");	while(1) ; }
void IntHandlerException7C(void) {	bprintf("CPU Exc: Coprocessor Absent\n");}
void IntHandlerException8C(void) {	bprintf("CPU Exc: Double Fault\n");	while(1) ; }
//void IntHandlerException9C(void) {	bprintf("CPU Exc: Copro Seg Overrun\n");	while(1) ; }
void IntHandlerException9C(void) {	bprintf("CPU Exc: Copro Seg Overrun\n");}
void IntHandlerExceptionAC(void) {	bprintf("CPU Exc: Invalid TSS\n");	while(1) ; }
void IntHandlerExceptionBC(void) {	bprintf("CPU Exc: Segment not present\n");	while(1) ; }
void IntHandlerExceptionCC(void) {	bprintf("CPU Exc: Stack Exception\n");	while(1) ; }
void IntHandlerExceptionDC(void) {	bprintf("CPU Exc: General Protection Fault\n");	while(1) ; }
void IntHandlerExceptionEC(void) {	bprintf("CPU Exc: Page Fault\n");	while(1) ; }
void IntHandlerExceptionFC(void) {	bprintf("CPU Exc: Reserved\n");	while(1) ; }
//void IntHandlerException10C(void) {	bprintf("CPU Exc: Copro Error\n");	while(1) ; }
void IntHandlerException10C(void) {	bprintf("CPU Exc: Copro Error\n");}


