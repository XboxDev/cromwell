/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /* 2003-01-06  andy@warmcat.com  Created
 */
 
 // header for BootFlash.c

// callback events
// note that if you receive *_START, you will always receive *_END even if an error is detected
 typedef enum {
 	EE_ERASE_START=1,
	EE_ERASE_UPDATE,  // dwPos runs from 0 to dwExtent-1
	EE_ERASE_END,
	EE_ERASE_ERROR,  // dwPos indicates error offset from start of flash
	EE_PROGRAM_START,
	EE_PROGRAM_UPDATE,  // dwPos runs from 0 to dwExtent-1
	EE_PROGRAM_END,
	EE_PROGRAM_ERROR,  // dwPos indicates error offset from start of flash, b7..b0 = read data, b15..b8 = written data
	EE_VERIFY_START,
	EE_VERIFY_UPDATE,  // dwPos runs from 0 to dwExtent-1
	EE_VERIFY_END,
	EE_VERIFY_ERROR  // dwPos indicates error offset from start of flash, b7..b0 = read data, b15..b8 = written data
 } ENUM_EVENTS;

 	// callback typedef
typedef bool (*CALLBACK_FLASH)(void * pvoidObjectFlash, ENUM_EVENTS ee, u32 dwPos, u32 dwExtent);

typedef struct {
 	volatile u8 * volatile m_pbMemoryMappedStartAddress; // fill on entry
	u8 m_bManufacturerId;
	u8 m_bDeviceId;
	char m_szFlashDescription[256];
 	char m_szAdditionalErrorInfo[256];
	u32 m_dwLengthInBytes;
	u32 m_dwStartOffset;
	u32 m_dwLengthUsedArea;
	CALLBACK_FLASH m_pcallbackFlash;
	bool m_fDetectedUsing28xxxConventions;
	bool m_fIsBelievedCapableOfWriteAndErase;
} OBJECT_FLASH;


typedef struct {
	u8 m_bManufacturerId;
	u8 m_bDeviceId;
 	char m_szFlashDescription[32];
	u32 m_dwLengthInBytes;
} KNOWN_FLASH_TYPE;

// requires pof->m_pbMemoryMappedStartAddress set to start address of flash in memory on entry

int BootReflashAndReset(u8 *pbNewData, u32 dwStartOffset, u32 dwLength);
void BootReflashAndReset_RAM(u8 *pbNewData, u32 dwStartOffset, u32 dwLength);

bool BootFlashGetDescriptor( OBJECT_FLASH *pof, KNOWN_FLASH_TYPE * pkft );
bool BootFlashEraseMinimalRegion( OBJECT_FLASH *pof);
bool BootFlashProgram( OBJECT_FLASH *pof, u8 *pba );

