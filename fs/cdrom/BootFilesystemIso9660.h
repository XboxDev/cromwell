#ifndef _BootISO9660_H_
#define _BootISO9660_H_

typedef struct {  // ISO gives some numbers in both ways
	WORD m_wLittleEndian;
	WORD m_wBigEndian;
} __attribute__ ((packed)) WORDR;

typedef struct {  // ISO gives some numbers in both ways
	DWORD m_dwLittleEndian;
	DWORD m_dwBigEndian;
} __attribute__ ((packed)) DWORDR;

typedef struct {
	BYTE m_bYearsSince1900;
	BYTE m_bMonth1Thru12;
	BYTE m_bDayOfMonth1Thru31;
	BYTE m_bHour0Thru23;
	BYTE m_bMinute0Thru59;
	BYTE m_bSecond0Thru59;
	char m_cTimezone15minsFromGmtSigned;
} __attribute__ ((packed)) ISO_DATE_AND_TIME;

typedef struct {
	BYTE m_bLength;  // 0x22
	BYTE m_bLengthExtendedAttribute; // 00
	DWORDR m_dwrExtentLocation; // 0x0113 -- points to directory contents
	DWORDR m_dwrDataLength; // 0x800 normally - bytes to be had at m_dwrExtentLocation
	ISO_DATE_AND_TIME m_idatDateOfRecording;
	BYTE m_bFileFlags; // 02
	BYTE m_bFileUnitSize; // 00
	BYTE m_bInterleaveGapSize; // 00
	WORDR m_wrVolumeSequenceNumber; // 0001
	BYTE m_bFileIdentifierLength; // 01
	char m_cFirstFileIdPlaceholder; // offset 34 zero term string
} __attribute__ ((packed)) ISO_SYSTEM_DIRECTORY_RECORD;

typedef struct {
	BYTE m_bVolumeDescriptorType;
	char m_szStandardIdentifier[5];
	BYTE m_bVolumeDescriptorVersion;
	BYTE m_bUnused8;
	char m_szSystemIdentifier[32];  // 9-40
	char m_szVolumeIdentifier[32];	// 41-72
	BYTE m_baUnused73[8];
	DWORDR m_dwrVolumeSpaceSize; // 81-88
	BYTE m_baUnused89[32];
	WORDR m_wrVolumeSetSize;  // seen 1
	WORDR m_wrVolumeSequenceNumber; // seen 1
	WORDR m_wrLogicalBlockSize; // seen 0x800
	DWORDR m_dwrPathTableSize; // seen 0x0a
	DWORD m_dwTypeLPathTable; // seen 010b
	DWORD m_dwTypeLPathTableOptional; // 0
	DWORD m_dwTypeMPathTable; // 0d010000
	DWORD m_dwTypeMPathTableOptional; // 0
	ISO_SYSTEM_DIRECTORY_RECORD m_isdrSystemDirectoryRecord; // zero length name +157

} __attribute__ ((packed)) ISO_PRIMARY_VOLUME_DESCRIPTOR;


// observed structure on Madrake 9 install CD:

// sector 0x10:  Primary volume descriptor (0x01 type)
// sector 0x11:  Supplimentary volume descriptor (had unicode) (0x02 type)
// sector 0x12:  Volume descriptor set terminator (0xff type)
// sector 0x13:  unknown sector of identifier BEA01 version 01 (rest zeros)
// sector 0x14:  unknown sector of identifier NSR02 version 01 (rest zeros)
// sector 0x15:  unknown sector of identifier TEA01 version 01 (rest zeros)
// sector 0x16:  text iso creator data: MKI Mon Oct 7 12:35:15 2002.mkisofs 1.15a32 -J -R -udf -V Xbox Linux Mandrake 9.
// sector 0x17, 0x18  all zeros

// sector 0x113: contents of root directory, multiple ISO_SYSTEM_DIRECTORY_RECORD back to back

#define ROOT_SECTOR 0x10
#define ROOT_DIR_STRUCT_OFFSET 0x9c
#define MAX_PATH_ELEMENT_SIZE 256


void BootIso9660DescriptorToString(const char * szcDescriptor, int nLength, char * szStringResult);
int BootIso9660GetFile(const char *szcPath, BYTE *pbaFile, DWORD dwFileLengthMax, DWORD dwOffset);
int BootIso9660GetFileDetails(const char * szcPath, ISO_SYSTEM_DIRECTORY_RECORD * pisdr);

#endif //       _BootISO9660_H_
