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

#include "2bload.h"

void smbus_smc_challenge_response(void)
{
    register u8 x1c, x1d, x1e, x1f;
    register u8 b1, b2, b3, b4;
    register int i;

    smbus_read_addr(SMBUS_SMC_ADDR);
    smbus_read_start(0x1c);
    if (!smbus_cycle_completed()) return;
    x1c = smbus_read_data();
    x1d = smbus_read(0x1d);
    x1e = smbus_read(0x1e);
    x1f = smbus_read(0x1f);

    b1  = 0x33;
    b2  = 0xed;
    b3  = x1c << 2;
    b3 ^= x1d + 0x39;
    b3 ^= x1e >> 2;
    b3 ^= x1f + 0x63;
    b4  = x1c + 0x0b;
    b4 ^= x1d >> 2;
    b4 ^= x1e + 0x1b;

    for (i = 0; i < 4; b1 += b2 ^ b3, b2 += b1 ^ b4, ++i);

    smbus_write_addr(SMBUS_SMC_ADDR);
    smbus_write(0x20, b1);
    smbus_write(0x21, b2);
}

// ----------------------------  I2C -----------------------------------------------------------
//
// get a value from a given device address
// errors will have b31 set, ie, will be negative, otherwise fetched byte in LSB of return

int I2CTransmitByteGetReturn(u8 bPicAddressI2cFormat, u8 bDataToWrite)
{
	int nRetriesToLive=400;

	//if(IoInputWord(I2C_IO_BASE+0)&0x8000) {  }
	while(IoInputWord(I2C_IO_BASE+0)&0x0800) ;  // Franz's spin while bus busy with any master traffic

	while(nRetriesToLive--) {

		IoOutputByte(I2C_IO_BASE+4, (bPicAddressI2cFormat<<1)|1);
		IoOutputByte(I2C_IO_BASE+8, bDataToWrite);
		IoOutputWord(I2C_IO_BASE+0, 0xffff); // clear down all preexisting errors
		IoOutputByte(I2C_IO_BASE+2, 0x0a);

		{
			u8 b=0x0;
			while( (b&0x36)==0 ) { b=IoInputByte(I2C_IO_BASE+0); }

			if(b&0x24) {
				//bprintf("I2CTransmitByteGetReturn error %x\n", b);
			}
			if(!(b&0x10)) {
				//bprintf("I2CTransmitByteGetReturn no complete, retry\n");
			} else {
				return (int)IoInputByte(I2C_IO_BASE+6);
			}
		}
	}

	return ERR_I2C_ERROR_BUS;
}

// transmit a word, no returned data from I2C device

int I2CTransmitWord(u8 bPicAddressI2cFormat, u16 wDataToWrite)
{
	int nRetriesToLive=400;

	while(IoInputWord(I2C_IO_BASE+0)&0x0800) ;  // Franz's spin while bus busy with any master traffic

	while(nRetriesToLive--) {
		IoOutputByte(I2C_IO_BASE+4, (bPicAddressI2cFormat<<1)|0);

		IoOutputByte(I2C_IO_BASE+8, (u8)(wDataToWrite>>8));
		IoOutputByte(I2C_IO_BASE+6, (u8)wDataToWrite);
		IoOutputWord(I2C_IO_BASE+0, 0xffff);  // clear down all preexisting errors
		IoOutputByte(I2C_IO_BASE+2, 0x1a);

		{
			u8 b=0x0;
			while( (b&0x36)==0 ) { b=IoInputByte(I2C_IO_BASE+0); }

			if(b&0x24) {
				//bprintf("I2CTransmitWord error %x\n", b);
			}
			if(!(b&0x10)) {
				//bprintf("I2CTransmitWord no complete, retry\n");
			} else {
				return ERR_SUCCESS;
			}
		}
	}
	return ERR_I2C_ERROR_BUS;
}

extern int I2cSetFrontpanelLed(u8 b)
{
	I2CTransmitWord( 0x10, 0x800 | b);  // sequencing thanks to Jarin the Penguin!
	I2CTransmitWord( 0x10, 0x701);
	return ERR_SUCCESS;
}

extern int I2cResetFrontpanelLed(void)
{
	I2CTransmitWord( 0x10, 0x700);
	return ERR_SUCCESS;
}

