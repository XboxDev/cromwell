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
	int nRetriesToLive=400;

//	IoOutputWord(I2C_IO_BASE+0, 0xffff); // clear down all preexisting errors
	if(IoInputWord(I2C_IO_BASE+0)&0x8000) { bprintf("Smb status=%x\n",IoInputWord(I2C_IO_BASE+0)); }
	while(IoInputWord(I2C_IO_BASE+0)&0x0800) ;  // Franz's spin while bus busy with any master traffic

	while(nRetriesToLive--) {

		IoOutputByte(I2C_IO_BASE+4, (bPicAddressI2cFormat<<1)|1);
		IoOutputByte(I2C_IO_BASE+8, bDataToWrite);
		IoOutputWord(I2C_IO_BASE+0, 0xffff); // clear down all preexisting errors
		IoOutputByte(I2C_IO_BASE+2, 0x0a);

		{
			BYTE b=0x0;
			while( (b&0x36)==0 ) { b=IoInputByte(I2C_IO_BASE+0); }

			if(b&0x24) {
				bprintf("I2CTransmitByteGetReturn error %x\n", b);
			}
			if(!(b&0x10)) {
				bprintf("I2CTransmitByteGetReturn no complete, retry\n");
			} else {
				return (int)IoInputByte(I2C_IO_BASE+6);
			}
		}
	}

	return ERR_I2C_ERROR_BUS;
}



// transmit a word, no returned data from I2C device

int I2CTransmitWord(BYTE bPicAddressI2cFormat, WORD wDataToWrite)
{
	int nRetriesToLive=400;

//	IoOutputWord(I2C_IO_BASE+0, 0xffff); // clear down all preexisting errors
	if(IoInputWord(I2C_IO_BASE+0)&0x8000) { bprintf("Smb status=%x\n",IoInputWord(I2C_IO_BASE+0)); }
	while(IoInputWord(I2C_IO_BASE+0)&0x0800) ;  // Franz's spin while bus busy with any master traffic

	while(nRetriesToLive--) {

		IoOutputByte(I2C_IO_BASE+4, (bPicAddressI2cFormat<<1)|0);

		IoOutputByte(I2C_IO_BASE+8, (BYTE)(wDataToWrite>>8));
		IoOutputByte(I2C_IO_BASE+6, (BYTE)wDataToWrite);
		IoOutputWord(I2C_IO_BASE+0, 0xffff);  // clear down all preexisting errors
		IoOutputByte(I2C_IO_BASE+2, 0x1a);

		{
			BYTE b=0x0;
			while( (b&0x36)==0 ) { b=IoInputByte(I2C_IO_BASE+0); }

			if(b&0x24) {
				bprintf("I2CTransmitWord error %x\n", b);
			}
			if(!(b&0x10)) {
				bprintf("I2CTransmitWord no complete, retry\n");
			} else {
				return ERR_SUCCESS;
			}
		}
	}

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

//		I2CTransmitWord( 0x10, 0x0100 );
//		I2CTransmitWord( 0x10, 0x130f );
//		I2CTransmitWord( 0x10, 0x12f0 );

//	if(BootPicManipulation(0x35, 0x62, 0xcd, 0x4a) != 0x9e71) {
//		return ERR_BOOT_PIC_ALG_BROKEN;
//	}

	n=I2CTransmitByteGetReturn( 0x10, 0x1c );
	if(n<0) return n;
	bC=n;
	n=I2CTransmitByteGetReturn( 0x10, 0x1d );
	if(n<0) return n;
	bD=n;
	n=I2CTransmitByteGetReturn( 0x10, 0x1e );
	if(n<0) return n;
	bE=n;
	n=I2CTransmitByteGetReturn( 0x10, 0x1f );
	if(n<0) return n;
	bF=n;

	{
		WORD w=BootPicManipulation(bC, bD, bE, bF);

		I2CTransmitWord( 0x10, 0x2000 | (w&0xff));
		I2CTransmitWord( 0x10, 0x2100 | (w>>8) );
	}

	n=I2CTransmitWord( 0x10, 0x0100 );

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
		" movl	$0x4000000, %eax \n"
		" outl	%eax, %dx "
		);
	return ERR_SUCCESS;
}

extern int I2cSetFrontpanelLed(BYTE b)
{
	DWORD dw;
	BootPciInterruptGlobalStackStateAndDisable(&dw);
	I2CTransmitWord( 0x10, 0x800 | b);  // sequencing thanks to Jarin the Penguin!
	I2CTransmitWord( 0x10, 0x701);
	BootPciInterruptGlobalPopState(dw);

	return ERR_SUCCESS;
}

bool I2CGetTemperature(int * pnLocalTemp, int * pExternalTemp)
{
	DWORD dw;
	BootPciInterruptGlobalStackStateAndDisable(&dw);
	*pnLocalTemp=I2CTransmitByteGetReturn(0x4c, 0x01);
	*pExternalTemp=I2CTransmitByteGetReturn(0x4c, 0x00);
	BootPciInterruptGlobalPopState(dw);
	return true;
}


