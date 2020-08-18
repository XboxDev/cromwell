#include "boot.h"
#include "VideoInitialization.h"
#include "BootEEPROM.h"

void BootEepromReadEntireEEPROM() {
	int i;
	u8 *pb=(u8 *)&eeprom;
	for(i = 0; i < 256; i++) {
		*pb++ = I2CTransmitByteGetReturn(0x54, i);
	}
}

void BootEepromPrintInfo() {
	VIDEO_ATTR=0xffc8c8c8;
	printk("MAC : ");
	VIDEO_ATTR=0xffc8c800;
	printk("%02X%02X%02X%02X%02X%02X  ",
		eeprom.MACAddress[0], eeprom.MACAddress[1], eeprom.MACAddress[2],
		eeprom.MACAddress[3], eeprom.MACAddress[4], eeprom.MACAddress[5]
	);

	VIDEO_ATTR=0xffc8c8c8;
	printk("Vid: ");
	VIDEO_ATTR=0xffc8c800;

	switch(*((VIDEO_STANDARD *)&eeprom.VideoStandard)) {
		case VID_INVALID:
			printk("0  ");
			break;
		case NTSC_M:
			printk("NTSC-M  ");
			break;
		case NTSC_J:
			printk("NTSC-J  ");
			break;
		case PAL_I:
			printk("PAL-I  ");
			break;
		default:
			printk("%X  ", (int)*((VIDEO_STANDARD *)&eeprom.VideoStandard));
			break;
	}

	VIDEO_ATTR=0xffc8c8c8;
	printk("Serial: ");
	VIDEO_ATTR=0xffc8c800;
	
	{
		char sz[13];
		memcpy(sz, &eeprom.SerialNumber[0], 12);
		sz[12]='\0';
		printk(" %s", sz);
	}

	printk("\n");
	VIDEO_ATTR=0xffc8c8c8;
}

/*
Copyright 2020 Mike Davis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
void EepromCRC(unsigned char *crc, unsigned char *data, long dataLen) {
	uint32_t high = 0, low = 0;

	for (uint32_t i = 0; i < dataLen / sizeof(uint32_t); i++) {
		uint32_t val = ((uint32_t*)data)[i];
		uint64_t sum = ((uint64_t)high << 32) | low;

		high = (uint32_t)((sum + val) >> 32);
		low += val;
	}

	*(uint32_t*)crc = ~(high + low);
}

void EepromSetWidescreen(int enable) {
	//Changing this setting requires that Checksum3 
	//be recalculated.
	
	unsigned char sum[4];
	if (enable) {
		//Enable WS
		WriteToSMBus(0x54, 0x96, 0, 1);
		eeprom.VideoFlags[2] = 0x01;
	} 
	else {
		//Disable WSS
		WriteToSMBus(0x54, 0x96, 0, 0);
		eeprom.VideoFlags[2] = 0x00;
	}

	EepromCRC(sum,eeprom.TimeZoneBias,0x5b);
	WriteToSMBus(0x54, 0x60, 1, sum[0]);
	WriteToSMBus(0x54, 0x61, 1, sum[1]);
	WriteToSMBus(0x54, 0x62, 1, sum[2]);
	WriteToSMBus(0x54, 0x63, 1, sum[3]);
}

void EepromSetVideoStandard(VIDEO_STANDARD standard) {
	//Changing this setting requires that Checksum2
	//be recalculated.
	unsigned char sum[4]; 
	unsigned int i;

	//Write the four bytes to the EEPROM
	for (i=0; i<4; ++i) {
		WriteToSMBus(0x54,0x58+i, 1, (u8)(standard>>(8*i))&0xff);
	}

	memcpy(eeprom.VideoStandard, &standard, 0x04);
	
	EepromCRC(sum,eeprom.SerialNumber,0x28);
	WriteToSMBus(0x54, 0x30, 0, sum[0]);
	WriteToSMBus(0x54, 0x31, 0, sum[1]);
	WriteToSMBus(0x54, 0x32, 0, sum[2]);
	WriteToSMBus(0x54, 0x33, 0, sum[3]);
}
