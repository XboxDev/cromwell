/*
 *  This code was originally from:
 *   UBL, The Universal Talkware Boot Loader
 *    Copyright (C) 2000 Universal Talkware Inc.
 *
 *  However after severe edits, even its own Mum wouldn't recognize it now
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Original from http://www.talkware.net/GPL/UBL
 *
 * 	 2002-09-19 andy@warmcat.com  Added code to manage standard CMOS settings for Bochs
 *   2002-09-18 andy@warmcat.com  fixed problem detecting slave not present
 *   2002-09-08 andy@warmcat.com  changed to standardized symbol format; ATAPI packet code
 *   2002-08-26 andy@warmcat.com  more edits to call speedbump's code to unlock HDD
 *   2002-08-25 andy@warmcat.com  threshed around to work with xbox
 */

#include  "boot.h"
//#include "string.h"
#include "grub/shared.h"
#include "BootEEPROM.h"

////////////////////////////////////
// IDE types and constants


#define IDE_SECTOR_SIZE 0x200
#define IDE_BASE1             (0x1F0u) /* primary controller */

#define IDE_REG_EXTENDED_OFFSET   (0x200u)

#define IDE_REG_DATA(base)          ((base) + 0u) /* word register */
#define IDE_REG_ERROR(base)         ((base) + 1u)
#define IDE_REG_FEATURE(base)         ((base) + 1u)
#define IDE_REG_SECTOR_COUNT(base)  ((base) + 2u)
#define IDE_REG_SECTOR_NUMBER(base) ((base) + 3u)
#define IDE_REG_CYLINDER_LSB(base)  ((base) + 4u)
#define IDE_REG_CYLINDER_MSB(base)  ((base) + 5u)
#define IDE_REG_DRIVEHEAD(base)     ((base) + 6u)
#define IDE_REG_STATUS(base)        ((base) + 7u)
#define IDE_REG_COMMAND(base)       ((base) + 7u)
#define IDE_REG_ALTSTATUS(base)     ((base) + IDE_REG_EXTENDED_OFFSET + 6u)
#define IDE_REG_CONTROL(base)       ((base) + IDE_REG_EXTENDED_OFFSET + 6u)
#define IDE_REG_ADDRESS(base)       ((base) + IDE_REG_EXTENDED_OFFSET + 7u)

typedef struct {
    unsigned char m_bPrecomp;
    unsigned char m_bCountSector;
    unsigned char m_bSector;
    unsigned short m_wCylinder;
    unsigned char m_bDrivehead;
#       define IDE_DH_DEFAULT (0xA0)
#       define IDE_DH_HEAD(x) ((x) & 0x0F)
#       define IDE_DH_MASTER  (0x00)
#       define IDE_DH_SLAVE   (0x10)
#       define IDE_DH_DRIVE(x) ((((x) & 1) != 0)?IDE_DH_SLAVE:IDE_DH_MASTER)
#       define IDE_DH_LBA     (0x40)
#       define IDE_DH_CHS     (0x00)

} tsIdeCommandParams;

#define IDE_DEFAULT_COMMAND { 0xFFu, 0x01, 0x00, 0x0000, IDE_DH_DEFAULT | IDE_DH_SLAVE }

typedef enum {
    IDE_CMD_NOOP = 0,
    IDE_CMD_RECALIBRATE = 0x10,
    IDE_CMD_READ_MULTI_RETRY = 0x20,
    IDE_CMD_READ_MULTI = IDE_CMD_READ_MULTI_RETRY,
    IDE_CMD_READ_MULTI_NORETRY = 0x21,

    IDE_CMD_DRIVE_DIAG = 0x90,
    IDE_CMD_SET_PARAMS = 0x91,
    IDE_CMD_STANDBY_IMMEDIATE = 0x94, /* 2 byte command- also send
                                         IDE_CMD_STANDBY_IMMEDIATE2 */
    IDE_CMD_SET_MULTIMODE = 0xC6,
    IDE_CMD_STANDBY_IMMEDIATE2 = 0xE0,
    IDE_CMD_GET_INFO = 0xEC,

		IDE_CMD_SET_FEATURES = 0xef,

		IDE_CMD_ATAPI_PACKET = 0xa0,

		IDE_CMD_SECURITY_UNLOCK = 0xf2
} ide_command_t;

#define printk_debug bprintf


//////////////////////////////////
//  Statics

tsHarddiskInfo tsaHarddiskInfo[2];  // static struct stores data about attached drives


///////////////////////////////////////////////////////////////////////////////////////////////////
//  Helper routines
//


int Delay() { int i=0, u=0; while(u<1000) { i+=u++; } return i; }


int BootIdeWaitNotBusy(unsigned uIoBase)
{
	BYTE b = 0x80;
//	int n=0;

//	I2cSetFrontpanelLed(0x66);
	while (b & 0x80) {
		Delay();
		b=IoInputByte(IDE_REG_ALTSTATUS(uIoBase));
//		printk("%02x %d\n", b, n++);
//		WATCHDOG;
	}
//	printk("Done %d\n", n++);
//		I2cSetFrontpanelLed(0x14);

	return b&1;
}

int BootIdeWaitDataReady(unsigned uIoBase)
{
	WORD i = 0x8000;
	Delay();
	do {
		if ( ((IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x88) == 0x08)	)	{
	    if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
			return 0;
		}
//		WATCHDOG();
		i--;
	} while (i != 0);

	if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	return 1;
}

int BootIdeIssueAtaCommand(
	unsigned uIoBase,
	ide_command_t command,
	tsIdeCommandParams * params)
{
	IoOutputByte(IDE_REG_SECTOR_COUNT(uIoBase), params->m_bCountSector);
	IoOutputByte(IDE_REG_SECTOR_NUMBER(uIoBase), params->m_bSector);
	IoOutputByte(IDE_REG_CYLINDER_LSB(uIoBase), params->m_wCylinder & 0xFF);
	IoOutputByte(IDE_REG_CYLINDER_MSB(uIoBase), (params->m_wCylinder >> 8) /* & 0x03 */);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), params->m_bDrivehead);

	IoOutputByte(IDE_REG_COMMAND(uIoBase), command);
	Delay();

	if (BootIdeWaitNotBusy(uIoBase))	{

		printk_debug("error on BootIdeIssueAtaCommand wait 3: error %02X\n", IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}

	return 0;
}

int BootIdeWriteAtapiData(unsigned uIoBase, void * buf, size_t size)
{
	WORD * ptr = (unsigned short *) buf;
	WORD w;


	BootIdeWaitDataReady(uIoBase);
	Delay();

	w=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
	w|=(IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase)))<<8;

//	bprintf("(bytes count =%04X)\n", w);

	while (size > 1) {

		IoOutputWord(IDE_REG_DATA(uIoBase), *ptr);
		size -= 2;
		ptr++;
	}
		IoInputByte(IDE_REG_STATUS(uIoBase));
	Delay();
	BootIdeWaitNotBusy(uIoBase);
	Delay();

   if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	return 0;
}

// issues a block of data ATA-style

int BootIdeWriteData(unsigned uIoBase, void * buf, size_t size)
{
	register unsigned short * ptr = (unsigned short *) buf;

	BootIdeWaitDataReady(uIoBase);
	Delay();

	while (size > 1) {

		IoOutputWord(IDE_REG_DATA(uIoBase), *ptr);
		size -= 2;
		ptr++;
	}
	Delay();
	BootIdeWaitNotBusy(uIoBase);
	Delay();

   if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	return 0;
}

// returns 16 == tray out, 8=tray in transit, 1 = tray in

BYTE BootIdeGetTrayState()
{
	BYTE b;
	DWORD dw;
	BootPciInterruptGlobalStackStateAndDisable(&dw);
	b=I2CTransmitByteGetReturn(0x10, 0x03); // this is the tray state
	BootPciInterruptGlobalPopState(dw);
	return b;
}

int BootIdeIssueAtapiPacketCommandAndPacket(int nDriveIndex, BYTE *pAtapiCommandPacket12Bytes)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned 	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nDriveIndex);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

		tsicp.m_wCylinder=2048;
		BootIdeWaitNotBusy(uIoBase);
		if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_ATAPI_PACKET, &tsicp)) {
//			printk("  Drive %d: BootIdeIssueAtapiPacketCommandAndPacket 1 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

		if(BootIdeWaitNotBusy(uIoBase)) {
//			printk("  Drive %d: BootIdeIssueAtapiPacketCommandAndPacket 2 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

//					printk("  Drive %d:   status=0x%02X, error=0x%02X\n",
//				nDriveIndex, IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), IoInputByte(IDE_REG_ERROR(uIoBase)));

		if(BootIdeWriteAtapiData(uIoBase, pAtapiCommandPacket12Bytes, 12)) {
//			printk("  Drive %d:BootIdeIssueAtapiPacketCommandAndPacket 3 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

		if(BootIdeWaitDataReady(uIoBase)) {
//			printk("  Drive %d:  BootIdeIssueAtapiPacketCommandAndPacket Atapi Wait for data ready FAILED, status=0x%02X, error=0x%02X\n",
//				nDriveIndex, IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

		return 0;
}


int BootIdeReadData(unsigned uIoBase, void * buf, size_t size)
{
	WORD * ptr = (WORD *) buf;

	if (BootIdeWaitDataReady(uIoBase)) {
//		printk_debug("data not ready...\n");
		return 1;
	}

	while (size > 1) {
		*ptr++ = IoInputWord(IDE_REG_DATA(uIoBase));
		size -= 2;
	}

	IoInputByte(IDE_REG_STATUS(uIoBase));

    if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;

	return 0;
}

// Issues a block of data ATA-style but with some small delays


/////////////////////////////////////////////////
//  BootIdeDriveInit
//
//  Called by BootIdeInit for each drive
//  detects and inits each drive, and the structs containing info about them

static int BootIdeDriveInit(unsigned uIoBase, int nIndexDrive)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned short* drive_info;
	BYTE baBuffer[512];

	tsaHarddiskInfo[nIndexDrive].m_fwPortBase = uIoBase;
	tsaHarddiskInfo[nIndexDrive].m_wCountHeads = 0u;
	tsaHarddiskInfo[nIndexDrive].m_wCountCylinders = 0u;
	tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack = 0u;
	tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal = 1ul;
	tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_CHS;
	tsaHarddiskInfo[nIndexDrive].m_fDriveExists = 0;
	tsaHarddiskInfo[nIndexDrive].m_enumDriveType=EDT_UNKNOWN;
	tsaHarddiskInfo[nIndexDrive].m_fAtapi=false;
	tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported=0;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

//	printk_debug("  Drive %d: Waiting\n", nIndexDrive);

	I2cSetFrontpanelLed(0xf1);

	if(BootIdeWaitNotBusy(uIoBase)) {
			printk_debug("  Drive %d: Not Ready\n", nIndexDrive);
			return 1;
	}


//		printk_debug("  Drive %d: happy\n", nIndexDrive);


	if(!nIndexDrive) // this should be done by ATAPI sig detection, but I couldn't get it to work
	{ // master... you have to send it IDE_CMD_GET_INFO
		int nReturn=0;
		if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_GET_INFO, &tsicp)) {
			nReturn=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
			nReturn |=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
			printk("nReturn = %x\n", nReturn);

			if(nReturn!=0xeb14) {
				printk("  Drive %d: detect FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}
		}
	} else { // slave... death if you send it IDE_CMD_GET_INFO, it needs an ATAPI request
		if(BootIdeIssueAtaCommand(uIoBase, 0xa1, &tsicp)) {
			printk("  %d: detect FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}
	}

	BootIdeWaitDataReady(uIoBase);
	if(BootIdeReadData(uIoBase, baBuffer, IDE_SECTOR_SIZE)) {
		printk("  %d: Drive not detected\n", nIndexDrive);
		return 1;
	}

	drive_info = (unsigned short*)baBuffer;
	tsaHarddiskInfo[nIndexDrive].m_wCountHeads = drive_info[3];
	tsaHarddiskInfo[nIndexDrive].m_wCountCylinders = drive_info[1];
	tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack = drive_info[6];
	tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal = *((unsigned int*)&(drive_info[60]));
	tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported = drive_info[88];

	{ int n;  // get rid of trailing spaces, add terminating '\0'
		WORD * pw=(WORD *)&(drive_info[10]);
		for(n=0; n<20;n+=2) { tsaHarddiskInfo[nIndexDrive].m_szSerial[n]=(*pw)>>8; tsaHarddiskInfo[nIndexDrive].m_szSerial[n+1]=(char)(*pw); pw++; }
		n=19; while(tsaHarddiskInfo[nIndexDrive].m_szSerial[n]==' ') n--; tsaHarddiskInfo[nIndexDrive].m_szSerial[n+1]='\0';
		pw=(WORD *)&(drive_info[27]);
		for(n=0; n<40;n+=2) { tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n]=(*pw)>>8;tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n+1]=(char)(*pw); pw++; }
		n=39; while(tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n]==' ') n--; tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n+1]='\0';
	}
	tsaHarddiskInfo[nIndexDrive].m_fDriveExists = 1;

	if(tsaHarddiskInfo[nIndexDrive].m_wCountHeads==0) { // CDROM/DVD

		tsaHarddiskInfo[nIndexDrive].m_fAtapi=true;

		VIDEO_ATTR=0xffc8c8c8;
		printk("hd%c: ", nIndexDrive+'a');
		VIDEO_ATTR=0xffc8c800;

		printk("%s %s\n",
			tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
			tsaHarddiskInfo[nIndexDrive].m_szSerial
		);


		{  // this is the only way to clear the ATAPI ''I have been reset'' error indication
			BYTE ba[128];
			int nPacketLength=BootIdeAtapiAdditionalSenseCode(nIndexDrive, &ba[0], sizeof(ba));
			if(nPacketLength<12) {
				printk("Unable to get ASC from drive\n");
				return 1;
			}
//			printk("ATAPI Drive reports ASC 0x%02X\n", ba[12]);  // normally 0x29 'reset' but clears the condition by reading
			nPacketLength=BootIdeAtapiAdditionalSenseCode(nIndexDrive, &ba[0], sizeof(ba));
			if(nPacketLength<12) {
				printk("Unable to get ASC from drive\n");
				return 1;
			}
//			printk("ATAPI Drive reports ASC 0x%02X\n", ba[12]);
			if(ba[12]==0x3a) { // no media, this is normal if there is no CD in the drive

			}
		}

	} else { // HDD
		BYTE bAdsBase=0x1b;
		unsigned long ulDriveCapacity1024=((tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal /1000)*512)/1000;
/*
		int nAta=0;
		if(tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported&2) nAta=1;
		if(tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported&4) nAta=2;
		if(tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported&8) nAta=3;
		if(tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported&16) nAta=4;
		if(tsaHarddiskInfo[nIndexDrive].m_wAtaRevisionSupported&32) nAta=5;
*/
		VIDEO_ATTR=0xffc8c8c8;
		printk("hd%c: ", nIndexDrive+'a');
		VIDEO_ATTR=0xffc8c800;
		
		printk("%s %u.%uGB ",
			tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
//			tsaHarddiskInfo[nIndexDrive].m_szSerial,
//			nAta,
// 			tsaHarddiskInfo[nIndexDrive].m_wCountHeads,
//  		tsaHarddiskInfo[nIndexDrive].m_wCountCylinders,
//   		tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack,
			ulDriveCapacity1024/1000, ulDriveCapacity1024%1000 //,
//			drive_info[128]
		);

		if(!nIndexDrive) { //HDD0
			BiosCmosWrite(0x19, 47); // drive 0 user-defined HDD type
			BiosCmosWrite(0x12, BiosCmosRead(0x12)|0xf0);
		} else { // HDD1
			BiosCmosWrite(0x12, BiosCmosRead(0x12)|0x0f);
			BiosCmosWrite(0x1a, 47); // drive 1 user-defined HDD type
			bAdsBase=0x24;
		}

		BiosCmosWrite(bAdsBase, (BYTE)(tsaHarddiskInfo[nIndexDrive].m_wCountCylinders));
		BiosCmosWrite(bAdsBase+1, (BYTE)(tsaHarddiskInfo[nIndexDrive].m_wCountCylinders>>8));
		BiosCmosWrite(bAdsBase+2, (BYTE)(tsaHarddiskInfo[nIndexDrive].m_wCountHeads));
		BiosCmosWrite(bAdsBase+3, 0);
		BiosCmosWrite(bAdsBase+4, 0);
		BiosCmosWrite(bAdsBase+5, 0);
		BiosCmosWrite(bAdsBase+6, 0);
		BiosCmosWrite(bAdsBase+7, 0);
		BiosCmosWrite(bAdsBase+0x8, (BYTE)(tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack));

	/*post_d0_type47:   <----taken from Bochs BIOS comment
  ;; CMOS  purpose                  param table offset

	:: 19
	;; 1b    cylinders low            0
  ;; 1c    cylinders high           1
  ;; 1d    heads                    2
  ;; 1e    write pre-comp low       5
  ;; 1f    write pre-comp high      6
  ;; 20    retries/bad map/heads>8  8
  ;; 21    landing zone low         C
  ;; 22    landing zone high        D
  ;; 23    sectors/track            E
	*/
	}

	if((drive_info[128]&0x0004)==0x0004) { // 'security' is in force, unlock the drive (was 0x104/0x104)
		BYTE baMagic[0x200], baKeyFromEEPROM[0x10], baEeprom[0x30];
		bool fUnlocked=false;
		int n, nVersionHashing=10;
		tsIdeCommandParams tsicp1 = IDE_DEFAULT_COMMAND;
		DWORD dwMagicFromEEPROM;
		void genHDPass(
			BYTE * beepkey,
			unsigned char *HDSerial,
			unsigned char *HDModel,
			unsigned char *HDPass
		);
		DWORD BootHddKeyGenerateEepromKeyData(
			BYTE *eeprom_data,
			BYTE *HDKey,
			int nVersion
		);
		int nVersionSuccessfulDecrypt=0;

		printk(" Lck[%x]", drive_info[128]);


		dwMagicFromEEPROM=0;

		while((nVersionHashing<=11) && (!fUnlocked)) {

			memcpy(&baEeprom[0], &eeprom, 0x30); // first 0x30 bytes from EEPROM image we picked up earlier
			dwMagicFromEEPROM = BootHddKeyGenerateEepromKeyData( &baEeprom[0], &baKeyFromEEPROM[0], nVersionHashing);

			if(!dwMagicFromEEPROM) {
				nVersionHashing++;
				continue;
			}
			nVersionSuccessfulDecrypt=nVersionHashing;

				// clear down the unlock packet, except for b8 set in first word (high security unlock)

			for(n=0;n<0x200;n++) baMagic[n]=0;
			{
				WORD * pword=(WORD *)&baMagic[0];
				*pword=0;  // try user
			}

			genHDPass( baKeyFromEEPROM, tsaHarddiskInfo[nIndexDrive].m_szSerial, tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber, &baMagic[2]);

			if(BootIdeWaitNotBusy(uIoBase)) {
					printk("  %d:  Not Ready\n", nIndexDrive);
					return 1;
			}
			tsicp1.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);

			if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_SECURITY_UNLOCK, &tsicp1)) {
				printk("  %d:  when issuing unlock command FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}
			BootIdeWaitDataReady(uIoBase);
			BootIdeWriteData(uIoBase, &baMagic[0], IDE_SECTOR_SIZE);

			if (BootIdeWaitNotBusy(uIoBase))	{
				printk("..");
				{
					WORD * pword=(WORD *)&baMagic[0];
					*pword=1;  // try master
				}

				if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_SECURITY_UNLOCK, &tsicp1)) {
					printk("  %d:  when issuing unlock command FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
					return 1;
				}
				BootIdeWaitDataReady(uIoBase);
				BootIdeWriteData(uIoBase, &baMagic[0], IDE_SECTOR_SIZE);

				if (BootIdeWaitNotBusy(uIoBase))	{
//					printk("  - Both Master and User unlock attempts failed\n");
				}
			}

				// check that we are unlocked

			tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);
			if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_GET_INFO, &tsicp)) {
				printk("  %d:  on issuing get status after unlock detect FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}
			BootIdeWaitDataReady(uIoBase);
			if(BootIdeReadData(uIoBase, baBuffer, IDE_SECTOR_SIZE)) {
				printk("  %d:  BootIdeReadData FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}

	//		printk("post-unlock sec status: 0x%x\n", drive_info[128]);
			if(drive_info[128]&0x0004) {
//				printk("  %d:  FAILED to unlock drive, security: %04x\n", nIndexDrive, drive_info[128]);
			} else {
//					printk("  %d:  Drive unlocked, new sec=%04X\n", nIndexDrive, drive_info[128]);
					fUnlocked=true;
	//				printf("    Unlocked");
			}

			nVersionHashing++;

		}

		if(!fUnlocked) {
			printk("\n");
			printk("FAILED to unlock drive, sec: %04x; Version=%d, EEPROM:\n", drive_info[128], nVersionSuccessfulDecrypt);
			VideoDumpAddressAndData(0, &baEeprom[0], 0x30);
			printk("Computed key:\n");
			VideoDumpAddressAndData(0, &baMagic[0], 0x40);
			return 1;
		}

	} else {
		if(nIndexDrive==0) printf("  Unlocked");
	}

	if (drive_info[49] & 0x200) { /* bit 9 of capability word is lba supported bit */
		tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_LBA;
	} else {
		tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_CHS;
	}

//			tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_CHS;

/*
	if(tsaHarddiskInfo[nIndexDrive].m_bLbaMode == IDE_DH_CHS) {
			printk("      CHS Mode\n");
	} else {
			printk("      LBA Mode\n");
	}
*/
	if(nIndexDrive==0 /*tsaHarddiskInfo[nIndexDrive].m_wCountHeads */) { // HDD not DVD (that shows up as 0 heads)

		unsigned char ba[512];
		int nError;

//		printk("Looking at capabilities\n");

			// report on the FATX-ness of the drive contents

		if((nError=BootIdeReadSector(nIndexDrive, &ba[0], 3, 0, 512))) {
			printk("  -  Unable to read FATX sector");
		} else {
			if( (ba[0]=='B') && (ba[1]=='R') && (ba[2]=='F') && (ba[3]=='R') ) {
				tsaHarddiskInfo[nIndexDrive].m_enumDriveType=EDT_UNKNOWN;
				printk(" - FATX", nIndexDrive);
			} else {
				printk(" - No FATX", nIndexDrive);
			}
		}

			// report on the MBR-ness of the drive contents

		if((nError=BootIdeReadSector(nIndexDrive, &ba[0], 0, 0, 512))) {
			printk("     Unable to get first sector, returned %d\n", nError);
		} else {
			if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) {
				printk(" - MBR", nIndexDrive);
				if(nIndexDrive==0) {
					BiosCmosWrite(0x3d, 2);  // boot from HDD
				}
			} else {
				printk(" - No MBR", nIndexDrive);
			}
		}

//		printk("BootIdeDriveInit() HDD completed ok\n");

		printk("\n");

	} else {  // cd/dvd
		if(BiosCmosRead(0x3d)==0) {  // no bootable HDD
			BiosCmosWrite(0x3d, 3);  // boot from CD/DVD instead then
		}

//		printk("BootIdeDriveInit() DVD completed ok\n");

	}

	return 0;
}


/////////////////////////////////////////////////
//  BootIdeInit
//
//  Called at boot-time to init and detect connected devices

int BootIdeInit(void)
{
	tsaHarddiskInfo[0].m_bCableConductors=40;

	BootIdeDriveInit(IDE_BASE1, 0);
	BootIdeDriveInit(IDE_BASE1, 1);

	if(tsaHarddiskInfo[0].m_fDriveExists) {
		unsigned int uIoBase = tsaHarddiskInfo[0].m_fwPortBase;
		tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;

		tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(0);
		IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

		if(!BootIdeIssueAtaCommand(uIoBase, IDE_CMD_GET_INFO, &tsicp)) {
			WORD waBuffer[256];
			BootIdeWaitDataReady(uIoBase);
			if(!BootIdeReadData(uIoBase, (BYTE *)&waBuffer[0], IDE_SECTOR_SIZE)) {
//				printk("%04X ", waBuffer[80]);
				if( ((waBuffer[93]&0xc000)!=0) && ((waBuffer[93]&0x8000)==0) && ((waBuffer[93]&0xe000)!=0x6000)) 	tsaHarddiskInfo[0].m_bCableConductors=80;
			} else {
				printk("Error getting final GET_INFO\n");
			}
		}
	}

	if(tsaHarddiskInfo[0].m_bCableConductors==40) {
		printk("UDMA2\n");
	} else {
		int nAta=0;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&2) nAta=1;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&4) nAta=2;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&8) nAta=3;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&16) nAta=4;
		if(tsaHarddiskInfo[0].m_wAtaRevisionSupported&32) nAta=5;
		printk("UDMA%d\n", nAta);
	}

	return 0;
}


/////////////////////////////////////////////////
//  BootIdeAtapiModeSense
//
//  returns the ATAPI extra error info block

int BootIdeAtapiModeSense(int nDriveIndex, BYTE bCodePage, BYTE * pba, int nLengthMaxReturn) {
	unsigned 	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

		BYTE ba[2048];
		int nReturn;

		if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

		memset(&ba[0], 0, 12);
		ba[0]=0x5a;
	 	ba[2]=bCodePage;
	 	ba[7]=(BYTE)(sizeof(ba)>>8); ba[8]=(BYTE)sizeof(ba);

		if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) {
//			BYTE bStatus=IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), bError=IoInputByte(IDE_REG_ERROR(uIoBase));
//			printk("  Drive %d: BootIdeAtapiAdditionalSenseCode 3 Atapi Wait for data ready FAILED, status=%02X, error=0x%02X, ASC unavailable\n", nDriveIndex, bStatus, bError);
			return 1;
		}

		nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
		nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
		if(nReturn>nLengthMaxReturn) nReturn=nLengthMaxReturn;
		BootIdeReadData(uIoBase, pba, nReturn);

		return nReturn;
}


/////////////////////////////////////////////////
//  BootIdeAtapiAdditionalSenseCode
//
//  returns the ATAPI extra error info block

int BootIdeAtapiAdditionalSenseCode(int nDriveIndex, BYTE * pba, int nLengthMaxReturn) {
	unsigned 	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

		BYTE ba[2048];
		int nReturn;

		if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

		memset(&ba[0], 0, 12);
		ba[0]=0x03;
	 	ba[4]=0xff;

		if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) {
//			BYTE bStatus=IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), bError=IoInputByte(IDE_REG_ERROR(uIoBase));
//			printk("  Drive %d: BootIdeAtapiAdditionalSenseCode 3 Atapi Wait for data ready FAILED, status=%02X, error=0x%02X, ASC unavailable\n", nDriveIndex, bStatus, bError);
			return 1;
		}

		nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
		nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
		if(nReturn>nLengthMaxReturn) nReturn=nLengthMaxReturn;
		BootIdeReadData(uIoBase, pba, nReturn);

		return nReturn;
}




/////////////////////////////////////////////////
//  BootIdeReadSector
//
//  Read an absolute sector from the device
//  knows if it should use ATA or ATAPI according to HDD or DVD
//  This is the main function for reading things from a drive

int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes) {
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned uIoBase;
	unsigned char baBufferSector[IDE_SECTOR_SIZE];
	unsigned int track;
	int status;

	if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nDriveIndex);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	if ((nDriveIndex < 0) || (nDriveIndex >= 2) ||
	    (tsaHarddiskInfo[nDriveIndex].m_fDriveExists == 0))
	{
		printk("unknown drive\n");
		return 1;
	}

	if(tsaHarddiskInfo[nDriveIndex].m_fAtapi) {

		BYTE ba[12];
		int nReturn;

		BootIdeWaitNotBusy(uIoBase);

		if(n_bytes<2048) {
			printk("Must have 2048 byte sector for ATAPI!!!!!\n");
			return 1;
		}

		tsicp.m_wCylinder=2048;
		memset(&ba[0], 0, 12);
		ba[0]=0x28; ba[2]=block>>24; ba[3]=block>>16; ba[4]=block>>8; ba[5]=block; ba[7]=0; ba[8]=1;

		if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) {
			printk("Unable to issue ATAPI command\n");
			return 1;
		}

		nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
		nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
//		printk("nReturn = %x\n", nReturn);

		if(nReturn>2048) nReturn=2048;
		BootIdeReadData(uIoBase, pbBuffer, nReturn);

		return 0;
	}

	if (tsaHarddiskInfo[nDriveIndex].m_wCountHeads > 8) {
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x0a);
	} else {
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x02);
	}

	tsicp.m_bCountSector = 1;

	if (tsaHarddiskInfo[nDriveIndex].m_bLbaMode == IDE_DH_CHS) {
		track = block / tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack;

		tsicp.m_bSector = 1+(block % tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack);
		tsicp.m_wCylinder = track / tsaHarddiskInfo[nDriveIndex].m_wCountHeads;
		tsicp.m_bDrivehead = IDE_DH_DEFAULT |
			IDE_DH_HEAD(track % tsaHarddiskInfo[nDriveIndex].m_wCountHeads) |
			IDE_DH_DRIVE(nDriveIndex) |
			IDE_DH_CHS;
	} else {

		tsicp.m_bSector = block & 0xff; /* lower byte of block (lba) */
		tsicp.m_wCylinder = (block >> 8) & 0xffff; /* middle 2 bytes of block (lba) */
		tsicp.m_bDrivehead = IDE_DH_DEFAULT | /* set bits that must be on */
			((block >> 24) & 0x0f) | /* lower nibble of byte 3 of block */
			IDE_DH_DRIVE(nDriveIndex) |
			IDE_DH_LBA;
	}

	if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_READ_MULTI_RETRY, &tsicp)) {
		printk("ide error %02X...\n", IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}
	if ((n_bytes != IDE_SECTOR_SIZE) || (byte_offset)) {
		status = BootIdeReadData(uIoBase, baBufferSector, IDE_SECTOR_SIZE);
//		if (status == 0) {
			memcpy(pbBuffer, baBufferSector+byte_offset, n_bytes);
//		}
	} else {
		status = BootIdeReadData(uIoBase, pbBuffer, IDE_SECTOR_SIZE);
	}
	return status;
}



///////////////////////////////////////////////
//      BootIdeBootSectorHddOrElTorito
//
//  Attempts to load boot code from Hdd or from CDROM/DVDROM
//   If HDD, loads MBR from Sector 0, if CDROM, uses El Torito to load default boot sector
//
// returns 0 if *pbaResult loaded with (512-byte/Hdd, 2048-byte/Cdrom) boot sector
//  otherwise nonzero return indicates error type

int BootIdeBootSectorHddOrElTorito(int nDriveIndex, BYTE * pbaResult)
{
	static const BYTE baCheck11hFormat[] = {
			0x00,0x43,0x44,0x30,0x30,0x31,0x01,0x45,
			0x4C,0x20,0x54,0x4F,0x52,0x49,0x54,0x4F,
			0x20,0x53,0x50,0x45,0x43,0x49,0x46,0x49,
			0x43,0x41,0x54,0x49,0x4F,0x4E
	};
	int n;
	DWORD * pdw;

	if(!tsaHarddiskInfo[nDriveIndex].m_fDriveExists) return 4;

	if(tsaHarddiskInfo[nDriveIndex].m_fAtapi) {

/******   Numbnut's guide to El Torito CD Booting   ********

  Sector 11h of a bootable CDROM looks like this (11h is a magic number)
  The DWORD starting at +47h is the sector index of the 'boot catalog'

00000000: 00 43 44 30 30 31 01 45 : 4C 20 54 4F 52 49 54 4F    .CD001.EL TORITO
00000010: 20 53 50 45 43 49 46 49 : 43 41 54 49 4F 4E 00 00     SPECIFICATION..
00000020: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
00000040: 00 00 00 00 00 00 00 13 : 00 00 00 00 00 00 00 00    ................
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], 0x11, 0, 2048)) {
			bprintf("Unable to get first sector\n");
			return 1;
		}

		for(n=0;n<sizeof(baCheck11hFormat);n++) {
			if(pbaResult[n]!=baCheck11hFormat[n]) {
				bprintf("Sector 11h not bootable format\n");
				return 2;
			}
		}

		pdw=(DWORD *)&pbaResult[0x47];

/*
At sector 13h (in this example only), the boot catalog:

00000000: 01 00 00 00 4D 69 63 72 : 6F 73 6F 66 74 20 43 6F    ....Microsoft Co
00000010: 72 70 6F 72 61 74 69 6F : 6E 00 00 00 4C 49 55 AA    rporation...LIU.
(<--- validation entry)
00000020: 88 00 00 00 00 00 04 00 : 25 01 00 00 00 00 00 00    ........%.......
(<-- initial/default entry - 88=bootable, 04 00 = 4 x (512-byte virtual sectors),
  = 1 x 2048-byte CDROM sector in boot, 25 01 00 00 = starts at sector 0x125)
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], *pdw, 0, 2048)) {
			bprintf("Unable to get boot catalog\n");
			return 3;
		}

		if((pbaResult[0]!=1) || (pbaResult[0x1e]!=0x55) || (pbaResult[0x1f]!=0xaa)) {
			bprintf("Boot catalog header corrupt\n");
			return 4;
		}

		if(pbaResult[0x20]!=0x88) {
			bprintf("Default boot catalog entry is not bootable\n");
			return 4;
		}

		pdw=(DWORD *)&pbaResult[0x28];
/*
And so at sector 0x125 (in this example only), we finally see the boot code

00000000: FA 33 C0 8E D0 BC 00 7C : FB 8C C8 8E D8 52 E8 00    .3.....|.....R..
00000010: 00 5E 81 EE 11 00 74 12 : 81 FE 00 7C 75 75 8C C8    .^....t....|uu..
00000020: 3D 00 00 75 7F EA 37 00 : C0 07 C6 06 AE 01 33 90    =..u..7.......3.
...
000007E0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000007F0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 55 AA    ..............U.
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], *pdw, 0, 2048)) {
			bprintf("Unable to get boot catalog\n");
			return 3;
		}

		if((pbaResult[0x7fe]!=0x55) || (pbaResult[0x7ff]!=0xaa)) {
			bprintf("Boot sector does not have boot signature!\n");
			return 4;
		}

		return 0; // success

	} else { // HDD boot

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], 0, 0, 512)) {
			bprintf("Unable to get MBR\n");
			return 3;
		}

		if((pbaResult[0x1fe]!=0x55) || (pbaResult[0x1ff]!=0xaa)) {
			bprintf("Boot sector does not have boot signature!\n");
			return 4;
		}

		return 0; // succes
	}
}

	// these guys are used by grub

int get_diskinfo (int drive, struct geometry *geometry)
{
//	printk("get_diskinfo for drive %d\n", drive);
	if(drive>1) return 1; // fail
	geometry->cylinders=tsaHarddiskInfo[drive].m_wCountCylinders;
	geometry->heads=tsaHarddiskInfo[drive].m_wCountHeads;
	geometry->sectors=tsaHarddiskInfo[drive].m_wCountSectorsPerTrack;
	geometry->total_sectors=tsaHarddiskInfo[drive].m_dwCountSectorsTotal;
	geometry->flags=0;

//	printk("geometry->total_sectors=0x%X\n", geometry->total_sectors);

	return 0; // success
}

int BootIdeSetTransferMode(int nIndexDrive, int nMode)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned int uIoBase = tsaHarddiskInfo[nIndexDrive].m_fwPortBase;
	BYTE b;
	DWORD dw;

	if(tsaHarddiskInfo[nIndexDrive].m_bCableConductors==80) {
//		PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x4c, 0xffff00ff); // 2x30nS address setup on IDE
	}

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	if(BootIdeWaitNotBusy(uIoBase)) {
			printk("  Drive %d: Not Ready\n", nIndexDrive);
			return 1;
	}
	{
		int nReturn=0;

		tsicp.m_bCountSector = (BYTE)nMode;
		IoOutputByte(IDE_REG_FEATURE(uIoBase), 3); // set transfer mode subcmd

		nReturn=BootIdeIssueAtaCommand(uIoBase, IDE_CMD_SET_FEATURES, &tsicp);

//		printk("BootIdeSetTransferMode nReturn = %x/ error %02X\n", nReturn, IoInputByte(IDE_REG_ERROR(uIoBase)) );

		switch(nMode&7) {
			case 0: b=3; break;
			case 1: b=1; break;
			case 2: b=0; break;
			case 3: b=4; break;
			case 4: b=5; break;
			case 5: b=6; break;
			default: b=6; break;
		}
		b|=0xc0;
		dw=PciReadDword(BUS_0, DEV_9, FUNC_0, 0x60);
		if(nIndexDrive) { // slave
			dw&=0xff00ff00;
			dw|=(b<<16) | (b);
		} else { // primary
			dw&=0x00ff00ff;
			dw|=(b<<24) | (b<<8);
		}
//		PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x60, dw); // new

		return nReturn;
	}
	return 0;
}



