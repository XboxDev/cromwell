 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"

/*
	WriteToSMBus()	by Lehner Franz (franz@caos.at)
	ReadfromSMBus() by Lehner Franz (franz@caos.at)
*/

int WriteToSMBus(u8 Address,u8 bRegister,u8 Size,u32 Data_to_smbus)
{
	int nRetriesToLive=50;

	while(IoInputWord(I2C_IO_BASE+0)&0x0800) ;  // Franz's spin while bus busy with any master traffic

	while(nRetriesToLive--) {
		
		u8 b;
		unsigned int temp;
		
		IoOutputByte(I2C_IO_BASE+4, (Address<<1)|0);
		IoOutputByte(I2C_IO_BASE+8, bRegister);

		switch (Size) {
			case 4:
				IoOutputByte(I2C_IO_BASE+9, Data_to_smbus&0xff);
				IoOutputByte(I2C_IO_BASE+9, (Data_to_smbus >> 8) & 0xff );
				IoOutputByte(I2C_IO_BASE+9, (Data_to_smbus >> 16) & 0xff );
				IoOutputByte(I2C_IO_BASE+9, (Data_to_smbus >> 24) & 0xff );
				IoOutputWord(I2C_IO_BASE+6, 4);
				break;
			case 2:
				IoOutputWord(I2C_IO_BASE+6, Data_to_smbus&0xffff);
				break;
			default:	// 1
				IoOutputWord(I2C_IO_BASE+6, Data_to_smbus&0xff);
				break;
		}
	
	
		temp = IoInputWord(I2C_IO_BASE+0);
		IoOutputWord(I2C_IO_BASE+0, temp);  // clear down all preexisting errors
	
		switch (Size) {
			case 4:
				IoOutputByte(I2C_IO_BASE+2, 0x1d);	// u32 modus
				break;
			case 2:
				IoOutputByte(I2C_IO_BASE+2, 0x1b);	// u16 modus
				break;
			default:	// 1
				IoOutputByte(I2C_IO_BASE+2, 0x1a);	// u8 modus
				break;
		}

		b = 0;
		
		while( (b&0x36)==0 ) { b=IoInputByte(I2C_IO_BASE+0); }

		if ((b&0x10) != 0) {
			return ERR_SUCCESS;
		
		}
		
		wait_us(1);
	}
        
	return ERR_I2C_ERROR_BUS;

}



int ReadfromSMBus(u8 Address,u8 bRegister,u8 Size,u32 *Data_to_smbus)
{
	int nRetriesToLive=50;
	
	while(IoInputWord(I2C_IO_BASE+0)&0x0800) ;  // Franz's spin while bus busy with any master traffic

	while(nRetriesToLive--) {
		u8 b;
		int temp;
		
		IoOutputByte(I2C_IO_BASE+4, (Address<<1)|1);
		IoOutputByte(I2C_IO_BASE+8, bRegister);
		
		temp = IoInputWord(I2C_IO_BASE+0);
		IoOutputWord(I2C_IO_BASE+0, temp);  // clear down all preexisting errors
				
		switch (Size) {
			case 4:	
				IoOutputByte(I2C_IO_BASE+2, 0x0d);	// u32 modus ?
				break;
			case 2:
				IoOutputByte(I2C_IO_BASE+2, 0x0b);	// u16 modus
				break;
			default:
				IoOutputByte(I2C_IO_BASE+2, 0x0a);	// u8
				break;
		}

		b = 0;
		
			
		while( (b&0x36)==0 ) { b=IoInputByte(I2C_IO_BASE+0); }

		if(b&0x24) {
			//printf("I2CTransmitByteGetReturn error %x\n", b);
		}
		
		if(!(b&0x10)) {
			//printf("I2CTransmitByteGetReturn no complete, retry\n");
		} else {
			switch (Size) {
				case 4:
					IoInputByte(I2C_IO_BASE+6);
					IoInputByte(I2C_IO_BASE+9);
					IoInputByte(I2C_IO_BASE+9);
					IoInputByte(I2C_IO_BASE+9);
					IoInputByte(I2C_IO_BASE+9);
					break;
				case 2:
					*Data_to_smbus = IoInputWord(I2C_IO_BASE+6);
					break;
				default:
					*Data_to_smbus = IoInputByte(I2C_IO_BASE+6);
					break;
			}
			

			return ERR_SUCCESS;

		}
		
	}
	       
	return ERR_I2C_ERROR_BUS;
}

/* ************************************************************************************************************* */



int I2CWriteWordtoRegister(u8 bPicAddressI2cFormat,u8 bRegister ,u16 wDataToWrite)
{
	// int WriteToSMBus(u8 Address,u8 bRegister,u8 Size,u32 Data_to_smbus)
	return WriteToSMBus(bPicAddressI2cFormat,bRegister,2,wDataToWrite);	
}


/* --------------------- Normal 8 bit operations -------------------------- */


int I2CTransmitByteGetReturn(u8 bPicAddressI2cFormat, u8 bDataToWrite)
{
	unsigned int temp;
	if (ReadfromSMBus(bPicAddressI2cFormat,bDataToWrite,1,&temp) != ERR_SUCCESS) return ERR_I2C_ERROR_BUS;
	return temp;
}


// transmit a word, no returned data from I2C device

int I2CTransmitWord(u8 bPicAddressI2cFormat, u16 wDataToWrite)
{
	return WriteToSMBus(bPicAddressI2cFormat,(wDataToWrite>>8)&0xff,1,(wDataToWrite&0xff));
}


int I2CWriteBytetoRegister(u8 bPicAddressI2cFormat, u8 bRegister, u8 wDataToWrite)
{
	return WriteToSMBus(bPicAddressI2cFormat,bRegister,1,(wDataToWrite&0xff));
	
}


void I2CModifyBits(u8 bAds, u8 bReg, u8 bData, u8 bMask)
{
	u8 b=I2CTransmitByteGetReturn(0x45, bReg)&(~bMask);
	I2CTransmitWord(0x45, (bReg<<8)|((bData)&bMask)|b);
}

// ----------------------------  PIC challenge/response -----------------------------------------------------------

extern int I2cSetFrontpanelLed(u8 b)
{
	I2CTransmitWord( 0x10, 0x800 | b);  // sequencing thanks to Jarin the Penguin!
	I2CTransmitWord( 0x10, 0x701);


	return ERR_SUCCESS;
}

bool I2CGetTemperature(int * pnLocalTemp, int * pExternalTemp)
{
	*pnLocalTemp=I2CTransmitByteGetReturn(0x4c, 0x01);
	*pExternalTemp=I2CTransmitByteGetReturn(0x4c, 0x00);
	
	//Check for bus error - 1.6 xboxes have no readable 
	//temperature sensors.
	if (*pnLocalTemp==ERR_I2C_ERROR_BUS || 
			*pExternalTemp==ERR_I2C_ERROR_BUS)		
				return false;
	return true;
}

void I2CRebootQuick(void) {
	WriteToSMBus(0x10,0x02,1,0x01);
}


void I2CRebootSlow(void) {
	WriteToSMBus(0x10,0x02,1,0x40);
}

void I2CPowerOff(void) {
	WriteToSMBus(0x10,0x02,1,0x80);
}
