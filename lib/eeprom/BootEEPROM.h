/*
 * Parts are from the Team-Assembly XKUtils thx.
 *
 */

#ifndef _BootEEPROM_H_
#define _BootEEPROM_H_

#include "VideoInitialization.h"
//Defines for Data structure sizes..
#define EEPROM_SIZE		0x100
#define CONFOUNDER_SIZE		0x008
#define HDDKEY_SIZE		0x010
#define XBEREGION_SIZE		0x001
#define SERIALNUMBER_SIZE	0x00C
#define MACADDRESS_SIZE		0x006
#define ONLINEKEY_SIZE		0x010
#define DVDREGION_SIZE		0x001
#define VIDEOSTANDARD_SIZE	0x004

//EEPROM Data structe value enums
typedef enum { 
	ZONE_NONE = 0x00,
	ZONE1 = 0x01,
	ZONE2 = 0x02,
	ZONE3 = 0x03,
	ZONE4 = 0x04,
	ZONE5 = 0x05,
	ZONE6 = 0x06
} DVD_ZONE;

typedef enum {
	VID_INVALID	= 0x00000000,
	NTSC_M		= 0x00400100,
	PAL_I		= 0x00800300
} VIDEO_STANDARD;

typedef enum {
	XBE_INVALID = 0x00,
	NORTH_AMERICA = 0x01,
	JAPAN = 0x02,
	EURO_AUSTRALIA = 0x04
} XBE_REGION;


//Structure that holds contents of 256 byte EEPROM image..
typedef struct _EEPROMDATA {
   u8		HMAC_SHA1_Hash[20];			// 0x00 - 0x13 HMAC_SHA1 Hash
   u8		Confounder[8];				// 0x14 - 0x1B RC4 Encrypted Confounder ??
   u8		HDDKkey[16];				// 0x1C - 0x2B RC4 Encrypted HDD key
   u8		XBERegion[4];				// 0x2C - 0x2F RC4 Encrypted Region code (0x01 North America, 0x02 Japan, 0x04 Europe)

   u8		Checksum2[4];				// 0x30 - 0x33 Checksum of next 44 bytes
   unsigned char	SerialNumber[12];			// 0x34 - 0x3F Xbox serial number 
   u8		MACAddress[6];				// 0x40 - 0x45 Ethernet MAC address
   u8		UNKNOWN2[2];			    // 0x46 - 0x47  Unknown Padding ?

   u8		OnlineKey[16];				// 0x48 - 0x57 Online Key ?

   u8		VideoStandard[4];			// 0x58 - 0x5B  ** 0x00014000 = NTSC, 0x00038000 = PAL
   
   u8		UNKNOWN3[4];			    // 0x5C - 0x5F  Unknown Padding ?
  
   //Comes configured up to here from factory..  everything after this can be zero'd out...
   //To reset XBOX to Factory settings, Make checksum3 0xFFFFFFFF and zero all data below (0x64-0xFF)
   //Doing this will Reset XBOX and upon startup will get Language & Setup screen...
   u8		Checksum3[4];				// 0x60 - 0x63  other Checksum of next  

   u8		TimeZoneBias[4];			// 0x64 - 0x67 Zone Bias?
   unsigned char	TimeZoneStdName[4];			// 0x68 - 0x6B Standard timezone
   unsigned char	TimeZoneDltName[4];			// 0x5C - 0x6F Daylight timezone
   u8		UNKNOWN4[8];				// 0x70 - 0x77 Unknown Padding ?
   u8		TimeZoneStdDate[4];		    // 0x78 - 0x7B 10-05-00-02 (Month-Day-DayOfWeek-Hour) 
   u8		TimeZoneDltDate[4];		    // 0x7C - 0x7F 04-01-00-02 (Month-Day-DayOfWeek-Hour) 
   u8		UNKNOWN5[8];				// 0x80 - 0x87 Unknown Padding ?
   u8		TimeZoneStdBias[4];			// 0x88 - 0x8B Standard Bias?
   u8		TimeZoneDltBias[4];			// 0x8C - 0x8F Daylight Bias?

   u8		LanguageID[4];				// 0x90 - 0x93 Language ID
  
   u8		VideoFlags[4];				// 0x94 - 0x97 Video Settings - 0x96 b0==widescreen 0x96 b4 == letterbox
   u8		AudioFlags[4];				// 0x98 - 0x9B Audio Settings
  
   u8		ParentalControlGames[4];	// 0x9C - 0x9F 0=MAX rating
   u8		ParentalControlPwd[4];		// 0xA0 - 0xA3 7=X, 8=Y, B=LTrigger, C=RTrigger
   u8		ParentalControlMovies[4];   // 0xA4 - 0xA7 0=Max rating
  
   u8		XBOXLiveIPAddress[4];		// 0xA8 - 0xAB XBOX Live IP Address..
   u8		XBOXLiveDNS[4];				// 0xAC - 0xAF XBOX Live DNS Server..
   u8		XBOXLiveGateWay[4];			// 0xB0 - 0xB3 XBOX Live Gateway Address..
   u8		XBOXLiveSubNetMask[4];		// 0xB4 - 0xB7 XBOX Live Subnet Mask..
   u8		OtherSettings[4];			// 0xA8 - 0xBB Other XBLive settings ?

   u8		DVDPlaybackKitZone[4];		// 0xBC - 0xBF DVD Playback Kit Zone

   u8		UNKNOWN6[64];				// 0xC0 - 0xFF Unknown Codes / History ?
} EEPROMDATA, *LPEEPROMDATA;

EEPROMDATA eeprom;

void BootEepromReadEntireEEPROM(void);
void BootEepromPrintInfo(void);
void EepromCRC(unsigned char *crc, unsigned char *data, long dataLen);
void EepromSetVideoStandard(xbox_tv_encoding standard);
void EepromSetWidescreen(int enable);
#endif // _BootEEPROM_H_
