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
 ***************************************************************************

	2002-12-18 andy@warmcat.com  Added ISO9660 boot if no HDD MBR
	2002-11-25 andy@warmcat.com  Added memory size code in CMOS, LILO won't boot without it
	                              tidied
	2002-09-19 andy@warmcat.com  Added code to manage standard CMOS settings for Bochs
	                              Defeated call to VGA init for now
 */

#include "boot.h"
#include "tuxicon44.c"

//////////////////////// compile-time options ////////////////////////////////

// selects between the supported video modes, both PAL at the moment
#define VIDEO_PREFERRED_LINES 576
//#define VIDEO_PREFERRED_LINES 480

// uncomment to force CD boot mode even if MBR present
// default is to boot from HDD if MBR present, else CD
//#define FORCE_CD_BOOT

// display a line like Composite 480 detected if uncommented
//#define REPORT_VIDEO_MODE

// show the MBR table if the MBR is valid
#define DISPLAY_MBR_INFO


#ifdef memcpy
#undef memcpy
#endif
#ifdef memset
#undef memset
#endif

BYTE baBackdrop[60*72*4];


/*
Hints:

1) We are in a funny environment, executing out of ROM.  That means it is not possible to
 define filescope non-const variables with an initializer.  If you do so, the linker will oblige and you will end up with
 a 4MByte image instead of a 1MByte one, the linker has added the RAM init at 400000.
*/


// access to RTC CMOS memory

void BiosCmosWrite(BYTE bAds, BYTE bData) {
		IoOutputByte(0x70, bAds);
		IoOutputByte(0x71, bData);
}

BYTE BiosCmosRead(BYTE bAds)
{
		IoOutputByte(0x70, bAds);
		return IoInputByte(0x71);
}


//
// ----------------------------  ACTUAL RESET CODE -----------------------------------------------------------
//  Gains control after minimal setup from X-Codes and x86
//

void intel_interrupts_on()
{
  unsigned long low, high;
  rdmsr(0x1b, low, high);
  low &= ~0x800;
  wrmsr(0x1b, low, high);

}



void InterruptUnhandledSpin(void)
{
  __asm __volatile__ (	" pusha ; cli "	);
	bprintf("Unhandled Interrupt\n");
  __asm __volatile__ (	" popa; iret "	);
}


// this guy is getting called at 18.2Hz

void IntHandlerCTimer0(void)
{
	(*PDW_BIOS_TICK_PTR)++;

//	if(((*PDW_BIOS_TICK_PTR) & 0x7)==7) {

	if((*PDW_BIOS_TICK_PTR) >8) {

		if(VIDEO_LUMASCALING<0x8c) {
			VIDEO_LUMASCALING+=5;
			I2CTransmitWord(0x45, 0xac00|((VIDEO_LUMASCALING)&0xff), false); // 8c
		}
		if(VIDEO_RSCALING<0x81) {
			VIDEO_RSCALING+=3;
			I2CTransmitWord(0x45, 0xa800|((VIDEO_RSCALING)&0xff), false); // 81
		}
		if(VIDEO_BSCALING<0x49) {
			VIDEO_BSCALING+=4;
			I2CTransmitWord(0x45, 0xaa00|((VIDEO_BSCALING)&0xff), false); // 49
		}
	}
//	}

	{
		char c=((*PDW_BIOS_TICK_PTR)*5)&0xff;
		DWORD dw=c;
		if(c<0) dw=((-(int)c)-1);
		dw|=128; dw<<=24;
		BootGimpVideoBlitBlend(
			(DWORD *)(FRAMEBUFFER_START+640*4*VIDEO_MARGINY+VIDEO_MARGINX*4), 640 * 4, (void *)&gimp_image,
			0x00000001 | dw, (DWORD *)&baBackdrop[0], 60*4
		);
	}

}



extern void IntHandlerTimer0(void);



extern void BootResetAction ( void ) {
	BYTE bAvPackType;
	__int64 i64Timestamp;
	bool fMbrPresent=false;
	int nActivePartitionIndex=0;

#if INCLUDE_FILTROR
	// clear down channel quality stats
	bfcqs.m_dwBlocksFromPc=0;
	bfcqs.m_dwCountChecksumErrorsSeenFromPc=0;
	bfcqs.m_dwBlocksToPc=0;
	bfcqs.m_dwCountTimeoutErrorsSeenToPc=0;
#endif

#if INCLUDE_FILTROR
	BootFiltrorSendArrayToPc("\nBOOT: starting BootResetAction()\n", 34);
#endif

	WATCHDOG;
	WATCHDOG;
	WATCHDOG;

	// if we don't make the PIC happy within 200mS, the damn thing will reset us
	BootPerformPicChallengeResponseAction();
	WATCHDOG;

	I2CTransmitWord( 0x10, 0x0c00, false );  // push out tray

	// bring up Video (2BL portion)
#ifdef ROM
	BootVgaInitialization();
	WATCHDOG;
#endif
	bAvPackType=BootVgaInitializationKernel(VIDEO_PREFERRED_LINES);
	WATCHDOG;
	// initialize the PCI devices
	BootPciPeripheralInitialization();
	WATCHDOG;

	{ // instability tracking, looking for duration changes
		int a2, a3;
		 __asm__ __volatile__ (" rdtsc" :"=a" (a3), "=d" (a2));
		i64Timestamp=((__int64)a3)|(((__int64)a2)<<32);
	}

	// prep our BIOS console print state

	
	VIDEO_ATTR=0xffffffff;
	VIDEO_LUMASCALING=0;
	VIDEO_RSCALING=0;
	VIDEO_BSCALING=0;

	BootVideoClearScreen();

	// get icon background into storage array
	BootVideoBlit((DWORD *)&baBackdrop[0], 60*4, (DWORD *)(FRAMEBUFFER_START+640*4*VIDEO_MARGINY+VIDEO_MARGINX*4), 640*4, 72);

	// bring up video - initially blank raster until time code brings up luma and chroma

	BootVideoEnableOutput(bAvPackType);
	
								// display solid red frontpanel LED while we start up
	I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );

	I2CTransmitWord(0x45, 0x0d04, false);  // fake int ack??
	I2CTransmitWord(0x45, 0x1b04, false);  // native BIOS to go to dashboard next time
	I2CTransmitWord(0x45, 0x1901, false); // no reset on eject
	I2CTransmitWord(0x45, 0x0c00, false); // eject DVD tray


	// then setup the interrupt table

	{  // init all of the interrupts to default to default handler (which spins complaining down Filtror)
		ts_pm_interrupt * ptspmi=(ts_pm_interrupt *)(0);  // ie, right down at start of RAM
		int n;
		for(n=0;n<32;n++) {  // have to do 256
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


	WATCHDOG;


	VIDEO_CURSOR_POSY=VIDEO_MARGINY;
	VIDEO_CURSOR_POSX=(VIDEO_MARGINX+64)*4;
	printk("\2Xbox Linux Clean BIOS  " VERSION "\2\n" );
	VIDEO_CURSOR_POSY=VIDEO_MARGINY+32;
	VIDEO_CURSOR_POSX=(VIDEO_MARGINX+64)*4;
	printk( __DATE__ " -  http://xbox-linux.sf.net\n");
	VIDEO_CURSOR_POSX=(VIDEO_MARGINX+64)*4;
	printk("(C)2002 Xbox Linux Team - Licensed under the GPL\n");

	printk("\n");

#ifdef REPORT_VIDEO_MODE

	switch(bAvPackType) {
		case 0:
			printk("Scart");
			break;
		case 1:
			printk("HDTV");
			break;
		case 2:
			printk("VGA");
			break;
		case 3:
			printk("Type 3");
			break;
		case 4:
			printk("SVideo");
			break;
		case 5:
			printk("Type 5");
			break;
		case 6:
			printk("Composite 480");
			break;
		case 7:
			printk("Composite 576");
			break;
	}

	printk(" video detected");
//	printk("  - Time=0x%08X/0x%08X", (DWORD)(i64Timestamp>>32), (DWORD)i64Timestamp);

	printk("\n");

#endif


//					__asm __volatile (	" wbinvd "	);
	WATCHDOG;
/*
	void 	BootFiltrorDebugShell();

	BootFiltrorDebugShell();

	VideoDumpAddressAndData(0xb8000, (void *)0xb8000, 0x80);
	memset((void *)0xb8000, 0x00, 256);
	{
		BYTE * pb=(BYTE *)0;
		while(pb<(BYTE *)0x00400000) {
			if((pb[0]=='m') && (pb[1]=='i') && (pb[2]=='s') && (pb[3]=='t')) {
				DumpAddressAndData((DWORD)pb, (void *)((DWORD)pb-0x20), 0x100);
			}
			pb++;
		}
	}
*/
//	VideoDumpAddressAndData(0xb8000, (void *)0xb8000, 0x80);


//	BootGimpVideoBlit( (DWORD *)(FRAMEBUFFER_START), 640 * 4, (void *)&gimp_image, 0xfffe00fe);

	WATCHDOG;

	//			I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3);

//	printk("Framebuffer Start: 0x%08lX", (DWORD)FRAMEBUFFER_START);

				I2cSetFrontpanelLed(I2C_LED_RED0 | I2C_LED_RED1 | I2C_LED_RED2 | I2C_LED_RED3 );

/*
	{ WORD *pw=(WORD *)0xffa6e;
		if((*pw==0)||(*pw>0x200)) {
			printk("\n");
		} else {
			VIDEO_ATTR=0xffe8e800;
			printk("Messages from previous boot attempt:\n");
			VIDEO_ATTR=0xffc8c800;
			((BYTE *)0xffa70)[*pw]='\0';
			BootVideoChunkedPrint((char *)&pw[1], pw[0]);
			printk("\n");
			*pw=0; __asm__ __volatile__ ("wbinvd" );
		}
	}
*/

		VIDEO_ATTR=0xffffffff;

//	VideoDumpAddressAndData(0, (BYTE *)0xffa70, 256);


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

			bprintf("Rewriting EEPROM, please wait...\n");

			while(nAds<0x100) {
				I2CTransmitWord( 0x54, (nAds<<8)|baMyEEProm[nAds], true);
				for(n=0;n<1000;n++) { IoInputByte(I2C_IO_BASE+0); }
				nAds++;
			}
	}
#endif




			// now do the main BIOS action


//			I2CTransmitWord(0x45, 0x1901, false);  // kill reset on frontpanel button

		{  			// standard BIOS settings
			int n;
			for(n=0x0e;n<0x40;n++) BiosCmosWrite(n, 0);
//			BiosCmosWrite(0x0e, 0); // say that CMOS, HDD and RTC are all valid
//			BiosCmosWrite(0x0f, 0); // say that we are coming up from a normal reset
//			BiosCmosWrite(0x10, 0); // no floppies on this thing
//			BiosCmosWrite(0x11, 0); // no standard BIOS options
//			BiosCmosWrite(0x12, 0); // no HDD yet
			BiosCmosWrite(0x14, 9); // misc equip list, VGA, display present, b0 fixed to 1
//			BiosCmosWrite(0x2d, 0); // misc BIOS settings
			BiosCmosWrite(0x30, 0x78); // 1K blocks of RAM, - first 1M
			BiosCmosWrite(0x31, 0xe6); // high part (set to 59MB)
			BiosCmosWrite(0x32, 20); // century
//			BiosCmosWrite(0x3d, 0); // boot method: default=0=do not attempt boot (set in BootIde.c)
		}


		I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2);


			// init the HDD and DVD
		VIDEO_ATTR=0xffc8c8c8;
		printk("Initializing IDE Controller\n");

		BootIdeInit();
		printk("\n");

		I2CTransmitWord(0x45, 0x0b01, false); // unknown, done immediately after reading out eeprom data

		I2CTransmitWord(0x45, 0x0b00, false); // unknown, done after video update

		I2CTransmitWord(0x45, 0x0d04, false); //fake up int ack?
		I2CTransmitWord(0x45, 0x1a01, false); // unknown, done immediately after reading out eeprom data

		{
			BYTE ba[512];
			if(BootIdeReadSector(0, &ba[0], 0, 0, 512)) {
				printk("Unable to read HDD first sector\n");
			} else {

				if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) fMbrPresent=true;

				if(fMbrPresent) {
					BYTE * pb;
					int n=0, nPos=0;
					bool fSeenActive=false;
#ifdef DISPLAY_MBR_INFO
					char sz[512];

					VIDEO_ATTR=0xffe8e8e8;
					printk("MBR Partition Table:\n");
#endif
					(BYTE *)pb=&ba[0x1be];
					n=0; nPos=0;
					while(n<4) {
#ifdef DISPLAY_MBR_INFO
						nPos=sprintf(sz, " hda%d: ", n+1);
#endif
						if(pb[0]&0x80) {
							nActivePartitionIndex=n;
							fSeenActive=true;
#ifdef DISPLAY_MBR_INFO
							nPos+=sprintf(&sz[nPos], "boot\t");
#endif
						} else {
#ifdef DISPLAY_MBR_INFO
							nPos+=sprintf(&sz[nPos], "   \t");
#endif
						}

#ifdef DISPLAY_MBR_INFO
	//					nPos+=sprintf(&sz[nPos],"type:%02X\tStart: %02X/%02X/%02X\tEnd: %02X/%02X/%02X  ", pb[4], pb[1],pb[2],pb[3],pb[5],pb[6],pb[7]);
						switch(pb[4]) {
							case 0x00:
								nPos+=sprintf(&sz[nPos],"Empty\t");
								break;
							case 0x82:
								nPos+=sprintf(&sz[nPos],"Swap\t");
								break;
							case 0x83:
								nPos+=sprintf(&sz[nPos],"Ext2 \t");
								break;
							case 0x1e:
								nPos+=sprintf(&sz[nPos],"FATX\t");
								break;
							default:
								nPos+=sprintf(&sz[nPos],"0x%02X\t", pb[4]);
								break;
						}
						if(pb[4]) {
							VIDEO_ATTR=0xffc8c8c8;
							if(pb[0]&0x80) VIDEO_ATTR=0xffd8d8d8;
							nPos+=sprintf(&sz[nPos],"Start: 0x%08x\tTotal: 0x%08x\t(%dMB)\n", *((DWORD *)&pb[8]), *((DWORD *)&pb[0xc]), (*((DWORD *)&pb[0xc]))/2048 );
						} else {
							VIDEO_ATTR=0xffa8a8a8;
							sz[nPos++]='\n'; sz[nPos]='\0';
						}
						printk(sz);
	#endif
						n++; pb+=16;
					}

					printk("\n");

					if(!fSeenActive) {
						printk("No active partition.  Cannot boot, halting\n");
						while(1) ;
					}

					VIDEO_ATTR=0xffffffff;
				} else { // no mbr signature
					;
				}

			}
		}

//					VIDEO_ATTR=0xffc8c800;


//		printk("\n");
//		printk("Booting Linux\n");

			// if we made it this far, lets have a solid green LED
		I2cSetFrontpanelLed(I2C_LED_GREEN0 | I2C_LED_GREEN1 | I2C_LED_GREEN2 | I2C_LED_GREEN3);




	// Used to start Bochs; now a misnomer as it runs vmlinux
	// argument 0 for hdd and 1 for from CDROM
#ifdef FORCE_CD_BOOT
	StartBios(1, 0);
#else
		if(fMbrPresent) { // if there's an MBR, try to boot from HDD
			StartBios(0, nActivePartitionIndex);
		} else { // otherwise prompt for CDROM
			StartBios(1, 0);
		}
#endif
	}


int VideoDumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable) { // returns bytes used
	int nCountUsed=0;
	while(dwCountBytesUsable) {

		DWORD dw=(dwAds & 0xfffffff0);
		char szAscii[17];
		char sz[256];
		int n=sprintf(sz, "%08X: ", dw);
		int nBytes=0;

		szAscii[16]='\0';
		while(nBytes<16) {
			if((dw<dwAds) || (dwCountBytesUsable==0)) {
				n+=sprintf(&sz[n], "   ");
				szAscii[nBytes]=' ';
			} else {
				BYTE b=*baData++;
				n+=sprintf(&sz[n], "%02X ", b);
				if((b<32) || (b>126)) szAscii[nBytes]='.'; else szAscii[nBytes]=b;
				nCountUsed++;
				dwCountBytesUsable--;
			}
			nBytes++;
			if(nBytes==8) n+=sprintf(&sz[n], ": ");
			dw++;
		}
		n+=sprintf(&sz[n], "   ");
		n+=sprintf(&sz[n], "%s", szAscii);
//		sz[n++]='\r';
		sz[n++]='\n';
		sz[n++]='\0';

		printk(sz, n);

		dwAds=dw;
	}
	return 1;
}

void * memcpy(void *dest, const void *src,  size_t size) {
//    bprintf("memcpy(0x%x,0x%x,0x%x);\n",dest,src,size);
#if 0
	BYTE * pb=(BYTE *)src, *pbd=(BYTE *)dest;
	while(size--) *pbd++=*pb++;

#else
		__asm__ __volatile__ (
              "    push %%esi    \n"
              "    push %%edi    \n"
              "    push %%ecx    \n"
             "    cld    \n"
              "    mov %0, %%esi \n"
              "    mov %1, %%edi \n"
              "    mov %2, %%ecx \n"
              "    push %%ecx    \n"
              "    shr $2, %%ecx \n"
              "    rep movsl     \n"
              "    pop %%ecx     \n"
              "    and $3, %%ecx \n"
              "    rep movsb     \n"
              : :"S" (src), "D" (dest), "c" (size)
		);

		__asm__ __volatile__ (
		          "    pop %ecx     \n"
              "    pop %edi     \n"
              "    pop %esi     \n"
		);
#endif
//	I2cSetFrontpanelLed(0x0f);
//    bprintf("memcpy done\n");
//		DumpAddressAndData(0xf0000, (BYTE *)0xf0000, 0x100);

	 return dest;
}

// int strlen(const char * sz) { int n=0; while(sz[n]) n++; return n; }
size_t strlen(const char * s)
{
int d0;
register int __res;
__asm__ __volatile__(
	"repne\n\t"
	"scasb\n\t"
	"notl %0\n\t"
	"decl %0"
	:"=c" (__res), "=&D" (d0) :"1" (s),"a" (0), "0" (0xffffffff));
return __res;
}

void BootVideoChunkedPrint(char * szBuffer, WORD wLength) {
	int n, nDone=0;

	szBuffer[wLength]='\0';

	for(n=0;n<(int)(wLength+1);n++) {
		if((szBuffer[n]=='\n') || (szBuffer[n]=='\0')) {
			bool f=(n<wLength);
			szBuffer[n]='\0';
			if(n!=nDone) {
				VIDEO_CURSOR_POSX+=BootVideoOverlayString(
					(DWORD *)((FRAMEBUFFER_START) + VIDEO_CURSOR_POSY * (640*4) + VIDEO_CURSOR_POSX),
					640*4, VIDEO_ATTR, &szBuffer[nDone]
				)<<2;
				nDone=n+1;
			} else { /* f=!nDone; */}
			if(f) { VIDEO_CURSOR_POSY+=16; VIDEO_CURSOR_POSX=VIDEO_MARGINX<<2; }
		}
	}
//	__asm__ __volatile__ ( "wbinvd" );
}

int printk(const char *szFormat, ...) {  // printk displays to video and filtror if enabled
	char szBuffer[512];
	WORD wLength=0;
	va_list argList;
	va_start(argList, szFormat);
	wLength=(WORD) vsprintf(szBuffer, szFormat, argList);
//	wLength=strlen(szFormat); // temp!
//	memcpy(szBuffer, szFormat, wLength);
	va_end(argList);

	szBuffer[wLength]='\0';

	#if INCLUDE_FILTROR
	BootFiltrorSendArrayToPcModal(&szBuffer[0], wLength);
	#endif

	BootVideoChunkedPrint(szBuffer, wLength);
	return wLength;
}




void * memset(void *dest, int data,  size_t size) {
//    bprintf("memset(0x%x,0x%x,0x%x);\n", dest, data, size);
    __asm__ __volatile__ (
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


void * malloc(size_t size) {
	void * pvoid=(void *)MALLOC_BASE;
	MALLOC_BASE+=size;
	return pvoid;
}

void free (void *ptr) {

}
