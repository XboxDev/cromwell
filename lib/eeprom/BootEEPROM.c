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

/* The EepromCRC algorithm was obtained from the XKUtils 0.2 source released by 
 * TeamAssembly under the GNU GPL.  
 * Specifically, from XKCRC.cpp 
 *
 * Rewritten to ANSI C by David Pye (dmp@davidmpye.dyndns.org)
 *
 * Thanks! */
void EepromCRC(unsigned char *crc, unsigned char *data, long dataLen) {
	unsigned char* CRC_Data = (unsigned char *)malloc(dataLen+4);
	int pos=0;
	memset(crc,0x00,4);

	memset(CRC_Data,0x00, dataLen+4);
	//Circle shift input data one byte right
	memcpy(CRC_Data + 0x01 , data, dataLen-1);
	memcpy(CRC_Data, data + dataLen-1, 0x01);

	for (pos=0; pos<4; ++pos) {
		unsigned short CRCPosVal = 0xFFFF;
		unsigned long l;
		for (l=pos; l<dataLen; l+=4) {
			CRCPosVal -= *(unsigned short*)(&CRC_Data[l]);
		}
		CRCPosVal &= 0xFF00;
		crc[pos] = (unsigned char) (CRCPosVal >> 8);
	}
	free(CRC_Data);
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
