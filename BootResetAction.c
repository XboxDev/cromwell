/*
 * Sequences the necessary post-reset actions from as soon as we are able to run C
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
#include <stdarg.h>
#include <stdio.h>

/*
Hints:

1) We are in a funny environment, executing out of ROM.  That means it is not possible to
 define filescope non-const variables with an initializer.  If you do so, the linker will oblige and you will end up with
 a 4MByte image instead of a 1MByte one, the linker has added the RAM init at 400000.
*/

int printk(const char *szFormat, ...) {  // printk displays to video and filtror if enabled
	char szBuffer[512];
	WORD wLength;
	va_list argList;
	va_start(argList, szFormat);
	wLength=(WORD) vsprintf(szBuffer, szFormat, argList);
	va_end(argList);
	
		// Video display output here

	#ifdef INCLUDE_FILTROR
	return BootFiltrorSendArrayToPcModal(&szBuffer[0], wLength);
	#else
	return wLength;
	#endif

}



void __inline * memcpy(void *dest, const void *src,  size_t size) {
//    bprintf("memcpy(0x%x,0x%x,0x%x);\n",dest,src,size);
    __asm__  (
              "    push %%esi    \n"
              "    push %%edi    \n"
              "    push %%ecx    \n"
              "    mov %0, %%esi \n"
              "    mov %1, %%edi \n"
              "    mov %2, %%ecx \n"
              "    push %%ecx    \n"
              "    shr $2, %%ecx \n"
              "    rep movsl     \n"
              "    pop %%ecx     \n"
              "    and $3, %%ecx \n"
              "    rep movsb     \n"
              "    pop %%ecx     \n"
              "    pop %%edi     \n"
              "    pop %%esi     \n"
              : : "S" (src), "D" (dest), "c" (size)
		);
		return dest;
}
void * memset(void *dest, int data,  size_t size) {
//    bprintf("memset(0x%x,0x%x,0x%x);\n", dest, data, size);
    __asm__  (
              "    push %%eax    \n"
              "    push %%edi    \n"
              "    push %%ecx    \n"
              "    mov %0, %%edi \n"
              "    mov %1, %%eax \n"
              "    mov %2, %%ecx \n"
              "    shr $2, %%ecx \n"
              "    rep stosl     \n"
              "    pop %%ecx     \n"
              "    pop %%edi     \n"
              "    pop %%eax     \n"
              : : "S" (dest), "eax" (data), "c" (size)
		);
	return dest;
}


// CPU global cache enable/disable

void BootCpuCache(bool fEnable) {
	DWORD dwOr=0x040000000, dwAnd=0xffffffff;
	if(fEnable) { dwOr=0; dwAnd = ~0x040000000; }
	__asm__ (
        "pushf ;"
        "push %%eax ;"
        "cli    ;"
        "mov     %%cr0, %%eax ;"
        "or      %%ebx, %%eax ;"
        "and      %%ecx, %%eax ;"
        "mov     %%eax, %%cr0 ;"
				"cmpl		$0, %%ebx ;"
				"jz skip ;"
        "wbinvd ;"  // invalidating the cache gratuitously seemed to cause crashes
			"skip: pop %%eax ;"
			"popf"
		: : "ebx" (dwOr), "ecx" (dwAnd)
	);
}


//
// ----------------------------  ACTUAL RESET CODE -----------------------------------------------------------
//  Gains control after minimal setup from X-Codes and x86
//

const unsigned long long
real_mode_gdt_entries [] = {
	0x0000000000000000ULL,  /* 00h: Null descriptor */
	0x0000000000000000ULL,  /* 08h: Unused... */
	0x00cf9a000000ffffULL,	/* 10h: 32-bit 4GB code at 0x00000000 */
	0x00cf92000000ffffULL,	/* 18h: 32-bit 4GB data at 0x00000000 */
	0x00009a000000ffffULL,  /* 20h: 16-bit 64k code at 0x00000000 */
	0x000092000000ffffULL	  /* 28h: 16-bit 64k data at 0x00000000 */
};




const ts_descriptor_pointer real_mode_gdt = {
	sizeof (real_mode_gdt_entries)-1,
	/*GDT_LOC+16 */ (DWORD)real_mode_gdt_entries
};
const ts_descriptor_pointer real_mode_idt = { 256*8-1, 0 };
//const ts_descriptor_pointer real_mode_idt = { 0, 0 };


void intel_interrupts_on()
{
  unsigned long low, high;
  rdmsr(0x1b, low, high);
  low &= ~0x800;
  wrmsr(0x1b, low, high);

}

void InterruptUnhandledSpin(void)
{
 __asm __volatile (	" sti "	);
	while(1) { bprintf("Unhandled Interrupt\n"); }
}


// this guy is getting called at 18.2Hz

void IntHandlerCTimer0(void)
{
	(*PDW_BIOS_TICK_PTR)++;
}



extern void IntHandlerTimer0(void);

extern void BootResetAction ( void ) {
//	int n;
//	int i;
	ts_descriptor_pointer * pdp= (ts_descriptor_pointer *)GDT_LOC;

//		dwGlobalTickCount=0;

		// first set up GDT and IDT in RAM where they are never paged out

			// GDT pointer first

		pdp->m_wSize = sizeof(real_mode_gdt_entries)-1;
		pdp->m_dwBase32 =((DWORD)GDT_LOC)+16;

		pdp= (ts_descriptor_pointer *) (((DWORD)GDT_LOC)+8);

			// IDT next

		pdp->m_wSize = 256 * 8-1;  // for 256 x 8 byte entries
		pdp->m_dwBase32 = 0;  // interrupt table down at 00000000

			// actual GDT

    memcpy(((BYTE *)GDT_LOC+16), real_mode_gdt_entries, sizeof(real_mode_gdt_entries));

		{
//			DWORD dwGdt=GDT_LOC, dwIdt=GDT_LOC+8;
	    __asm (
			" cli \n"
 	   "lidt  %0             \n"
 	   "lgdt  %1             \n"
 	   "mov  $0x18, %%ax     \n" /* 32-bit, 4GB data from GDT */
 	   "mov  %%ax, %%ds      \n"
 	   "mov  %%ax, %%es      \n"
		"ljmp $0x10, $self ; self: nop        \n"
 	: :  "m" (real_mode_idt), "m" (real_mode_gdt)
 	);
	}


#if INCLUDE_FILTROR
		// clear down channel quality stats
	bfcqs.m_dwBlocksFromPc=0;
	bfcqs.m_dwCountChecksumErrorsSeenFromPc=0;
	bfcqs.m_dwBlocksToPc=0;
	bfcqs.m_dwCountTimeoutErrorsSeenToPc=0;
#endif

				// then do critical setup of peripherals

	 	BootPerformXCodeActions();

			// display solid red frontpanel LED while we start up
		I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3);

#if INCLUDE_FILTROR
//		BootFiltrorSendArrayToPc("\nBOOT: starting BootResetAction()\n", 34);
#endif

			// if we don't make the PIC happy within 200mS, the damn thing will reset us
		BootPerformPicChallengeResponseAction();

//		I2CTransmitWord( 0x10, 0x1900, true);

				// restore EEPROM to my original contents
#if 0
		{
			int nAds=0, n;
			static const BYTE baMyEEProm[] = {
				0x6D, 0xAB, 0x59, 0xA2, 0xB8, 0x82, 0x09, 0xAB, 0x21, 0x84, 0xB2, 0x50, 0x8A, 0x7F, 0x4F, 0x43,
				0x54, 0x01, 0x1E, 0x52, 0xD3, 0xB6, 0x3A, 0x5C, 0x32, 0xA6, 0x11, 0x28, 0x72, 0x07, 0xAE, 0x3C,
				0x36, 0xD4, 0x83, 0xFB, 0xE0, 0x29, 0xEE, 0xA8, 0x1C, 0x9D, 0x14, 0xEF, 0x44, 0x39, 0x65, 0x37,
				0xC3, 0xA3, 0x94, 0xF3, 0x32, 0x31, 0x36, 0x34, 0x30, 0x35, 0x33, 0x32, 0x31, 0x31, 0x30, 0x33,
				0x00, 0x50, 0xF2, 0x62, 0xDD, 0x3B, 0x00, 0x00, 0x41, 0x99, 0x81, 0xB5, 0xBF, 0xCC, 0x91, 0x7C,
				0xE9, 0xE5, 0x19, 0xF5, 0xDF, 0xE9, 0x31, 0xE8, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
				0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00
			};

			while(nAds<0x100) {
				I2CTransmitWord( 0x54, (nAds<<8)|baMyEEProm[nAds], true);
				for(n=0;n<1000;n++) { IoInputByte(I2C_IO_BASE+0); }
				nAds++;
			}
	}
#endif
				// then setup the interrupt table

	{  // init all of the interrupts to default to default handler (which spins complaining down Filtror)
		ts_pm_interrupt * ptspmi=(ts_pm_interrupt *)(0);  // ie, right down at start of RAM
		int n;
		for(n=0;n<256;n++) {  // have to do 256
			ptspmi[n].m_wHandlerHighAddressLow16=(WORD)InterruptUnhandledSpin;
			ptspmi[n].m_wSelector=0x10;
			ptspmi[n].m_wType=0x8e00;  // interrupt gate, 32-bit
			ptspmi[n].m_wHandlerLinearAddressHigh16=(WORD)(((DWORD)InterruptUnhandledSpin)>>16);
		}

		ptspmi[8].m_wHandlerHighAddressLow16=(WORD)IntHandlerTimer0;
		ptspmi[8].m_wHandlerLinearAddressHigh16=(WORD)(((DWORD)IntHandlerTimer0)>>16);

	}

	intel_interrupts_on();  // let the processor respond to interrupts

	*PDW_BIOS_TICK_PTR=0;

		// set up the Programmable Inetrrupt Controllers

	IoOutputByte(0x20, 0x15);  // master PIC setup
	IoOutputByte(0x21, 0x08);
	IoOutputByte(0x21, 0x04);
	IoOutputByte(0x21, 0x01);
	IoOutputByte(0x21, 0x00);

	IoOutputByte(0xa0, 0x15);	// slave PIC setup
	IoOutputByte(0xa1, 0x70);
	IoOutputByte(0xa1, 0x02);
	IoOutputByte(0xa1, 0x01);
	IoOutputByte(0xa1, 0xff);

		// start up Timer 0 so we get the 18.2Hz tick interrupts

	IoOutputByte(0x43, 0x36);
	IoOutputByte(0x40, 0xff);
	IoOutputByte(0x40, 0xff);

 __asm __volatile (	" sti "	);  // and allow interrupt processing


			// now do the main BIOS action

	printk(
		"Xbox Linux Clean BIOS "
		VERSION 
		"    Licensed under the GPL     http://xbox-linux.sf.net\n(C)2002 The Xbox Linux Team - Hardware subsidy by Microsoft, everything else GPL\n\n"
	);

			// initialize the PCI devices
	BootPciPeripheralInitialization();

			I2CTransmitWord(0x45, 0x1901, false);  // kill reset on frontpanel button


			// try to bring up VGA - does not work yet
		BootVgaInitialization();
//		bprintf("BOOT: BootVgaInitialization done\n");

//			I2CTransmitWord( 0x10, 0x0c01, true);

	BootIdeInit();

//			bprintf("tick=%x\n", *PDW_BIOS_TICK_PTR);

/*
	bprintf("\nInsert bootable CD...\n");

	{
		BYTE ba[2048];
		ba[12]=0x3a; // no media

		while((ba[12]==0x3a)||(ba[12]==0x04)) {
			BootIdeAtapiAdditionalSenseCode(1, ba, sizeof(ba));
//		bprintf("ASC=0x%02X\n", ba[12]);
		}

		if(BootIdeBootSectorHddOrElTorito(1, &ba[0])) {
			bprintf("Failed to get CDROM boot sector\n");
		} else {
			DumpAddressAndData(0, &ba[0], 0x800);
		}
	}
*/
	I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3);

	/* run the PC-like BIOS */
	StartBios();

	}


