#include "boot.h"
#include "BootEEPROM.h"

void ReadEEPROM() {
	int i;
	unsigned char ret[256];

	for(i = 0; i < 256; i++) {
		ret[i] = I2CTransmitByteGetReturn(0x54, i);
	}
	memcpy(&eeprom,ret,sizeof(ret));
}
void PrintInfo() {

	VIDEO_STANDARD vdo = (VIDEO_STANDARD)eeprom.VideoStandard;

	VIDEO_ATTR=0xffc8c800;
	printk("XBOX EEPROM Info\n");

	printk("HW MAC : %02X %02X %02X %02X %02X %02X\n",eeprom.MACAddress[0],eeprom.MACAddress[1],eeprom.MACAddress[2],eeprom.MACAddress[3],eeprom.MACAddress[4],eeprom.MACAddress[5]);
	if(vdo == NTSC_M) {
		printk("Videostandard : NTSC\n");
	} 
	if(vdo == VID_INVALID) {
		printk("Videostandard : Invalid\n");
	} 
	if(vdo == PAL_I) {
		printk("Videostandard : PAL\n");
	}
	printk("\n");
	VIDEO_ATTR=0xffc8c8c8;
}
