/*
 * I2C-related code
 * AG 2002-07-27
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


// ----------------------------  I2C -----------------------------------------------------------
//
// get a value from a given device address
// errors will have b31 set, ie, will be negative, otherwise fetched byte in LSB of return

int I2CTransmitByteGetReturn(BYTE bPicAddressI2cFormat, BYTE bDataToWrite)
{
	DWORD dwRetriesToLive=4;

	__asm __volatile__ ( "pushf ; cli" );

	while(dwRetriesToLive--) {
		DWORD dwSpinsToLive=0x800000;

		IoOutputByte(I2C_IO_BASE+4, (bPicAddressI2cFormat<<1)|1);
		IoOutputByte(I2C_IO_BASE+8, bDataToWrite);
		IoOutputWord(I2C_IO_BASE+0, 0x10 /*IoInputWord(I2C_IO_BASE+0)*/);
		IoOutputByte(I2C_IO_BASE+2, 0x0a);

		{
			BYTE b=0;
			while( (b !=0x10) && ((b&0x26)==0) && (dwSpinsToLive--) ) {
				b=IoInputByte(I2C_IO_BASE+0);
//				if(dwSpinsToLive<0x7ffffd) bprintf("%02X\n", b);
			}
			if(dwSpinsToLive==0) { __asm __volatile__ ( "popf" ); return ERR_I2C_ERROR_TIMEOUT; }
			if(b&0x2) {

				continue; // retry
			}
			__asm __volatile__ ( "popf" );

			if(b&0x24) return ERR_I2C_ERROR_BUS;
			if(!(b&0x10)) return ERR_I2C_ERROR_BUS;

				// we are okay, fetch returned byte
			return (int)IoInputByte(I2C_IO_BASE+6);

		}
	}
		__asm __volatile__ ( "popf" );

	return ERR_I2C_ERROR_BUS;
}

// transmit a word, no returned data from I2C device

int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite, bool fMode)
{
	DWORD dwRetriesToLive=4;
__asm __volatile__ ( "pushf; cli" );
	while(dwRetriesToLive--) {
		DWORD dwSpinsToLive=0x8000000;

		IoOutputByte(I2C_IO_BASE+4, (bPicAddressI2cFormat<<1)|0);

		IoOutputByte(I2C_IO_BASE+8, (BYTE)(wDataToWrite>>8));
		IoOutputByte(I2C_IO_BASE+6, (BYTE)wDataToWrite);
		IoOutputWord(I2C_IO_BASE+0, 0x10 /*IoInputWord(I2C_IO_BASE+0)*/);
//		if(fMode) {
			IoOutputByte(I2C_IO_BASE+2, 0x1a);
//		} else {
//			IoOutputByte(I2C_IO_BASE+2, 0x0a);
//		}

		{
			BYTE b=0x0;
			while( (b!= 0x10) && ((b&0x26)==0) && (dwSpinsToLive--) ) { b=IoInputByte(I2C_IO_BASE+0); }

			if(dwSpinsToLive==0) { 		__asm __volatile__ ( "popf" ); return ERR_I2C_ERROR_TIMEOUT; }
			if(b&0x2) continue; // retry

					__asm __volatile__ ( "popf" );

			if(b&0x24) return ERR_I2C_ERROR_BUS;
			if(!(b&0x10)) return ERR_I2C_ERROR_BUS;

				// we are okay, return happy code
			return ERR_SUCCESS;
		}
	}
		__asm __volatile__ ( "popf" );
	return ERR_I2C_ERROR_BUS;
}

// ----------------------------  PIC challenge/response -----------------------------------------------------------
//
// given four bytes, returns a WORD
// LSB of return is the 'first' byte, MSB is the 'second' response byte

WORD BootPicManipulation(
	BYTE bC,
	BYTE  bD,
	BYTE  bE,
	BYTE  bF
) {
	int n=4;
	BYTE
		b1 = 0x33,
		b2 = 0xed,
		b3 = ((bC<<2) ^ (bD +0x39) ^ (bE >>2) ^ (bF +0x63)),
		b4 = ((bC+0x0b) ^ (bD>>2) ^ (bE +0x1b))
	;

	while(n--) {
		b1 += b2 ^ b3;
		b2 += b1 ^ b4;
	}

	return (WORD) ((((WORD)b2)<<8) | b1);
}



// actual business of getting I2C data from PIC and reissuing munged version
// returns zero if all okay, else error code

int BootPerformPicChallengeResponseAction()
{
	BYTE bC, bD, bE, bF;
	int n;

//		I2CTransmitWord( 0x10, 0x0100, false );
//		I2CTransmitWord( 0x10, 0x130f, false );
//		I2CTransmitWord( 0x10, 0x12f0, false );

//	if(BootPicManipulation(0x35, 0x62, 0xcd, 0x4a) != 0x9e71) {
//		return ERR_BOOT_PIC_ALG_BROKEN;
//	}

	n=I2CTransmitByteGetReturn( 0x10, 0x1c);
	if(n<0) return n;
	bC=n;
	n=I2CTransmitByteGetReturn( 0x10, 0x1d);
	if(n<0) return n;
	bD=n;
	n=I2CTransmitByteGetReturn( 0x10, 0x1e);
	if(n<0) return n;
	bE=n;
	n=I2CTransmitByteGetReturn( 0x10, 0x1f);
	if(n<0) return n;
	bF=n;

/*
			__asm__ __volatile__ (
		" push %edx \n"
		" push %eax \n"
		" mov $0x8000036c, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" movl	$01000000, %eax \n"
		" outl	%eax, %dx \n"
//		" hlt \n"
		" pop %eax \n"
		" pop %edx \n"
		);
*/

	{
		WORD w=BootPicManipulation(bC, bD, bE, bF);

		n=I2CTransmitWord( 0x10, 0x2000 | (w&0xff), false);
//		if(n<0) return n;

		n=I2CTransmitWord( 0x10, 0x2100 | (w>>8), false );
//		if(n<0) return n;

	}

		n=I2CTransmitWord( 0x10, 0x0100, false );
		if(n<0) return n;

/*
		// the following traffic was observed during a normal boot
		// all seemed happy without it, but it is included here for completeness

	n=I2CTransmitByteGetReturn( 0x10, 0x04);
	if(n<0) return n;
	n=I2CTransmitByteGetReturn( 0x10, 0x11);
	if(n<0) return n;
	I2CTransmitWord( 0x10, 0x0d04, true);
	n=I2CTransmitByteGetReturn( 0x10, 0x1b);
	if(n<0) return n;
	I2CTransmitWord( 0x10, 0x1b04, true);
	I2CTransmitWord( 0x10, 0x1901, true);
	I2CTransmitWord( 0x10, 0x0c00, true);
	n=I2CTransmitByteGetReturn( 0x10, 0x11);
	if(n<0) return n;
*/
		// continues as part of video setup....


		__asm__ __volatile__ (
		" mov $0x8000088c, %eax \n"
		" movw $0xcf8, %dx \n"
		" outl	%eax, %dx \n"
		" movw $0xcfc, %dx \n"
		" movl	$0x40000000, %eax \n"
		" outl	%eax, %dx \n"
		);


	return ERR_SUCCESS;
}

extern int I2cSetFrontpanelLed(BYTE b)
{
	int n;
	__asm __volatile__ ( "pushf ; cli" );
	n=I2CTransmitWord( 0x10, 0x800 | b, true);  // sequencing thanks to Jarin the Penguin!
	n=I2CTransmitWord( 0x10, 0x701, true);
	__asm __volatile__ ( "popf" );

	return ERR_SUCCESS;
}

