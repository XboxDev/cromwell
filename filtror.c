/*
 * Filtror-specific code
 * AG 2002-07-12
 * See http://warmcat.com/milksop/filtror.html for more details
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

#if INCLUDE_FILTROR

// ----------------------------  FILTROR Debug stuff -----------------------------------------------------------
//
// Debug area occupies a 4K block near end of LPC area,  FFFFE000 - FFFFEFFF
// This area must be marked as write-through and uncached
//
// FFFFE000 - FFFFE7FB  ... X-Box writes message towards PC here
// FFFFE7FC..FFFFE7FD  ... Checksum, just the 16-bit little-endian additive checksum of the written bytes, initialized with zero
// FFFFE7FE..FFFFE7FF  ... After writing message & checksum, X-Box sets these two bytes to length, with b15 SET.  LSB in ...7FE, MSB (with b15 set) in ...7FF
// FFFFE800 - FFFFEFFD  ... PC writes message towards X-Boxhere
// FFFFEFFC..FFFFEFFD  ... Checksum, just the 16-bit little-endian additive checksum of the written bytes, initialized with zero
// FFFFEFFE..FFFFEFFF  ... After writing message and checksum, PC sets these two bytes to length, with b15 SET.  LSB in ...FFE, MSB (with b15 set) in ...FFF
//
// Messages are written from the start of the appropriate area first, them the associated checksum and length is written LSB first, then MSB with b15 SET
// After reading a message, the recipient zeros the length Word, LSB first, then MSB, indicating to the sender it has been read and another may be sent


// currently we have cache disabled all the time, no need for filtror to manipulate CPU cache dynamically
#define FiltrorCpuCache(x) {  ; }
// otherwise uncomment this
//#define FiltrorCpuCache BootCpuCache

// returns -1 if no message waiting, else message length (note: may legally be zero for null message)
// if the message is valid you may read from it at (((BYTE *)FILT_DEBUG_BASE)+0x800) without further cache disable as the caches were invalidated

int BootFiltrorGetIncomingMessageLength() {
	WORD volatile * pwIncomingLength=(WORD *)FILT_DEBUG_FROMPC_CHECKSUM;  // ie, last two bytes of block
	WORD w, wChecksum;
	FiltrorCpuCache(false);
	w=pwIncomingLength[1];
	FiltrorCpuCache(true);
	if((w&0x8000)==0) return -1; // no message waiting if b15 clear
	FiltrorCpuCache(false);
	w=pwIncomingLength[1];
	wChecksum =pwIncomingLength[0];
	FiltrorCpuCache(true);
		w&=0x7fff;

		// assess checksum

	{
		WORD wChecksumComputed=0;
		WORD wLoop=w;
		BYTE * pb = (BYTE *)FILT_DEBUG_FROMPC_START;
		while(wLoop--) { wChecksumComputed += *pb++; }

			// corrupted blocks are ignored ( '' lost in transit'' - the only safe signal on a corruptible channel) 
			// requires higher level timeout on sender

		if(wChecksum !=wChecksumComputed) {
			bfcqs.m_dwCountChecksumErrorsSeenFromPc++;  // keep note of what we are seeing
			return -1;  // claim that there is nothing to get
		}
	}

	return (int)w;
}

// call this to tell the PC that you have dealt with a message that it sent and can take the next message from it

void BootFiltrorMarkIncomingMessageAsHavingBeenRead() { 
	BYTE * pbIncomingLength=(BYTE *)FILT_DEBUG_FROMPC_LEN;  // ie, last two bytes of block
	FiltrorCpuCache(false);
	pbIncomingLength[0]=0; // zero LSB first
	pbIncomingLength[1]=0; // zero MSB last
	FiltrorCpuCache(true);
	bfcqs.m_dwBlocksFromPc++; // keep incoming stats
}

// call this to find out if PC can take a new message

bool BootFiltrorDoesPcHaveAMessageWaitingForItToRead() { 
	WORD * pbOutgoingLength=(WORD *)FILT_DEBUG_TOPC_LEN;  // ie, last two bytes of first half of block
	WORD w;
	FiltrorCpuCache(false);
	w=* pbOutgoingLength;
	FiltrorCpuCache(true);
	return (w&0x8000)!=0;
}

void BootFiltrorSetMessageLengthForPcToRead(WORD wChecksum, WORD wLength) {
	BYTE volatile * pbOutgoingLength=(BYTE *)FILT_DEBUG_TOPC_CHECKSUM;  // ie, last two bytes of first half of block
	FiltrorCpuCache(false);
	pbOutgoingLength[0]=(BYTE)wChecksum; // set LSB first
	pbOutgoingLength[1]=(BYTE)(wChecksum>>8); // set MSB last
	pbOutgoingLength[2]=(BYTE)wLength; // set LSB first
	pbOutgoingLength[3]=((BYTE)(wLength>>8))|0x80; // set MSB last, force b15 set
	FiltrorCpuCache(true);
	bfcqs.m_dwBlocksToPc++;
}

int BootFiltrorSendArrayToPc(const BYTE * pba, WORD wLength)
{
	WORD wChecksum=0;
	int n=0;
	if(wLength >= (WORD)FILT_DEBUG_MAX_DATA) return -1;
	if(BootFiltrorDoesPcHaveAMessageWaitingForItToRead()) return -2;

	while(n++<(int)wLength) {
		wChecksum+=pba[n-1];
	}

	{
		BYTE *pbDest = (BYTE *)FILT_DEBUG_TOPC_START;
		WORD w=wLength;
		while(w--) { *pbDest++ = *pba++; }
	}

	BootFiltrorSetMessageLengthForPcToRead(wChecksum, wLength);
	return 0;
}

int BootFiltrorSendArrayToPcModal(const BYTE * pba, WORD wLength)
{
	while(1) {
		int n=BootFiltrorSendArrayToPc(pba, wLength);
		if(n<0) continue;
		return n;
	}
}

int BootFiltrorSendStringToPcModal(const char *szFormat, ...) {
	char szBuffer[512];
	WORD wLength;
	va_list argList;
	va_start(argList, szFormat);
	wLength=(WORD) vsprintf(szBuffer, szFormat, argList);
	va_end(argList);
	return BootFiltrorSendArrayToPcModal(&szBuffer[0], wLength);
}



// negative retcode if nothing waiting, may return 0 for null, legal message

int BootFiltrorGetArrayFromPc( BYTE * pba, WORD wLengthMax)
{
	WORD wLength = BootFiltrorGetIncomingMessageLength();
	if(wLength & 0x8000) return (int)(short)wLength; // nothing for us, negative retcode

		// something has arrived, and checksum must have been good or we would never have heard of it

	if(wLengthMax<(wLength+1)) return -2;

	{
		BYTE *pbDest = (BYTE *)FILT_DEBUG_FROMPC_START;
		WORD w=wLength;
		while(w--) { *pba++ = *pbDest++; }
		*pba='\0';
	}

	BootFiltrorMarkIncomingMessageAsHavingBeenRead();

	return wLength;
}

// returns number of chars used
// sets *pdw to the hex version of the string
// stops at first non-hex digit

int BootAsciiHexToDword(const char *sz, DWORD * pdw)
{
	int n=0;
	*pdw=0;
	while(1) {
		if((sz[n] >='0') && (sz[n]<='9') ) {
			*pdw = ((*pdw) << 4) | (sz[n]-'0');
		} else {
			if((sz[n] >='a') && (sz[n]<='f') ) {
				*pdw = (((*pdw )<< 4) | (sz[n]-'a')) +10;
			} else {
				if((sz[n] >='A') && (sz[n]<='F') ) {
					*pdw = (((*pdw) << 4) | (sz[n]-'A'))+10;
				} else {
					return n;
				}
			}
		}
		n++;
	}
}

int DumpAddressAndData(DWORD dwAds, const BYTE * baData, DWORD dwCountBytesUsable) { // returns bytes used
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
		sz[n++]='\r';
		sz[n++]='\n';
	//	sz[n++]='\0';

		BootFiltrorSendArrayToPcModal(sz, n);

		dwAds=dw;
	}

	return nCountUsed;
}

#if INCLUDE_FILTROR
//  __attribute__ ((section (".data")))
BOOTFILTROR_CHANNEL_QUALITY_STATS bfcqs;  // information about connection quality and usage
#endif

// this is the debug shell that allows memory dumping and other interactive debugging
// using the filtror comms to lmilk in -t terminal mode.

void BootFiltrorDebugShell() {
	DWORD dwAdsContinuation=0;
			// say hello over Filtror

	bprintf("\nLinux Box BIOS debug terminal mode... h for help\n");

		// debug shell

	while(1) {
		BYTE ba[2048];

		int nLengthArrived=BootFiltrorGetArrayFromPc(&ba[0], sizeof(ba)-1);
		if(nLengthArrived>=0) {  // something has arrived

			ba[nLengthArrived]='\0';
			switch (ba[0]) {
				case 'h': // help
					bprintf("d [<hex start ads> [<hex length>]] dump memory, default is 64 bytes, no args= next 64 bytes\n");
					bprintf("e <hex start ads> <hex byte> [<hex byte>...] enter memory data\n");
					bprintf("f <hex ads> <hex length> [<2-char hex byte>]  Fill memory (default is with 0x55)\n");
					bprintf("i <hex IO ads>  perform IO port BYTE IN\n");
					bprintf("p <hex PIC ads> <hex> I2C Pic Write\n");
					bprintf("o <hex IO ads>  <2-char hex byte> perform IO port BYTE OUT\n");
					bprintf("                <4-char hex byte> perform IO port WORD OUT\n");
					bprintf("                <8-char hex byte> perform IO port DWORD OUT\n");
					bprintf("r <hex PCI Bus> <device> <function> <reg> perform PCI CFG port BYTE IN\n");
					bprintf("w <hex PCI Bus> <device> <function> <reg><2/4/8-char hex data> perform PCI CFG BYTE OUT\n");
					bprintf("q   quit debug shell and resume execution\n");
					bprintf("\n");
					break;

				case 'd': // dump <hex start ads> [<hex length>]
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwAds, dwLen=64;
						while(*szc==' ') szc++;
						if(*szc=='\n') {
							dwAds=dwAdsContinuation;
						} else {
							szc+=BootAsciiHexToDword(szc, &dwAds);
							while(*szc==' ') szc++;
							if(*szc!='\n') szc+=BootAsciiHexToDword(szc, &dwLen);
						}
						DumpAddressAndData(dwAds, (const BYTE *)dwAds, dwLen);
						dwAdsContinuation=dwAds+dwLen;
					}
					break;

					case 'f': // fill <hex start ads> <hex length> <2 hex chars>
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwAds, dwLen=64, dwData=0x55;

						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwAds);
						while(*szc==' ') szc++;
						if(*szc!='\n') szc+=BootAsciiHexToDword(szc, &dwLen);
						while(*szc==' ') szc++;
						if(*szc!='\n') szc+=BootAsciiHexToDword(szc, &dwLen);
						while(dwLen--) {
							*((BYTE *)dwAds++)=(BYTE)dwData;
						}
					}
					break;

					case 'e': // enter memory <hex start ads> <hex byte>[<hex byte>...]
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwAds, dwData, dwAdsInitial, dwLen=0;
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwAds);
						dwAdsInitial = dwAds;
						while(*szc!='\n') {
							while(*szc==' ') szc++;
							if(*szc=='\n') continue;
							szc+=BootAsciiHexToDword(szc, &dwData);
							*((BYTE *)dwAds++)=(BYTE)dwData;
							dwLen++;
							}
						DumpAddressAndData(dwAdsInitial, (const BYTE *)dwAdsInitial, dwLen);
					}
					break;

					case 'i': // IO BYTE in
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwAds;
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwAds);
						bprintf("IO %04X: %02X\n", dwAds, IoInputByte(dwAds));
					}
					break;

				case 'o': // IO BYTE or WORD or DWORD out
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwAds, dwLen;
						int nSizeArg;
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwAds);
						while(*szc==' ') szc++;
						nSizeArg=BootAsciiHexToDword(szc, &dwLen);
						switch(nSizeArg) {
							case 4: // WORD
								IoOutputWord(dwAds, (WORD)dwLen);
								bprintf("Word out %X <- %04X\n", dwAds, dwLen&0xffff);
								break;
							case 8: //DWORD
								IoOutputDword(dwAds, dwLen);
								bprintf("DWORD out %X <- %08X\n", dwAds, dwLen);
								break;
							default: // BYTE
								IoOutputByte(dwAds, (BYTE)dwLen);
								bprintf("Byte out %X <- %02X\n", dwAds, dwLen&0xff);
								break;
						}
					}
					break;

				case 'p': // PIC I2C IO
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwAds, dwLen;
						int nSizeArg;
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwAds);
						while(*szc==' ') szc++;
						nSizeArg=BootAsciiHexToDword(szc, &dwLen);

						I2CTransmitWord(0x10, (dwAds<<8)|dwLen);
						bprintf("PIC 0x%02X <- 0x%02X\n", dwAds, dwLen);
					}
					break;

				case 'r': // PCI CFG reg READ
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwBus, dwDevice, dwFunction, dwReg;
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwBus);
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwDevice);
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwFunction);
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwReg);
						IoOutputDword(0xcf8, (0x80000000 | (dwBus <<16) | (dwDevice << 11) | (dwFunction << 8) | (dwReg & 0xfc)));

						bprintf("PCI Bus 0x%x, Device 0x%x, Function: 0x%x, Register: 0x%x = %02X\n", dwBus, dwDevice, dwFunction, dwReg,  IoInputByte(0xcfc + (dwReg & 3)) );
					}
					break;

				case 'w': // PCI CFG reg WRITE
					{
						const char * szc=(const char *)&ba[1];
						DWORD dwBus, dwDevice, dwFunction, dwReg, dwSet;
						int nSizeArg;
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwBus);
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwDevice);
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwFunction);
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwReg);
						while(*szc==' ') szc++;
						nSizeArg=BootAsciiHexToDword(szc, &dwSet);
						IoOutputDword(0xcf8, (0x80000000 | (dwBus <<16) | (dwDevice << 11) | (dwFunction << 8) | (dwReg & 0xfc)) );
						if(nSizeArg<3) {
						  IoOutputByte(0xcfc + (dwReg & 3), dwSet );
						} else {
							if(nSizeArg<5) {
							  IoOutputWord(0xcfc + (dwReg & 2), dwSet );
							} else {
							  IoOutputDword(0xcfc , dwSet );
							}

						}
						bprintf("PCI Bus 0x%x, Device 0x%x, Function: 0x%x, Register: 0x%x <- %02X\n", dwBus, dwDevice, dwFunction, dwReg,  dwSet);
					}
					break;
					
				case 'z':
				{
					int n;
					for( n=0;n<64;n++) {
						IoOutputByte(0x70, n);
						bprintf("%02X ", IoInputByte(0x71));
						if((n&0x07)==0x07) bprintf("\n");
					}
				bprintf("\n");
				}
				break;

			case 't':
			{
							const char * szc=(const char *)&ba[1];
						DWORD dwBus, dwDevice, dwFunction;
						int nSizeArg;
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwBus);
						while(*szc==' ') szc++;
						szc+=BootAsciiHexToDword(szc, &dwDevice);


					for(dwFunction=0; dwFunction<256; dwFunction++) {
						for(nSizeArg=0; nSizeArg<256;nSizeArg+=4) {
							IoOutputDword(0xcf8, (0x80000000 | (dwBus <<16) | (dwDevice << 11) | (dwFunction << 8) | (nSizeArg & 0xfc)) );
							  IoOutputDword(0xcfc , 0 );
							IoOutputDword(0xcf8, (0x80000000 | (dwBus <<16) | (dwDevice << 11) | (dwFunction << 8) | (nSizeArg & 0xfc)) );
							  IoOutputDword(0xcfc , (DWORD)0xffffffff );
						}
					}
					bprintf("Done\n");
		}
			break;

				case 'q':
					return;

				default:
					break;
			}

		}

	}

}

#endif
