/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /* 2003-01-07  andy@warmcat.com  Created
 */

#include "boot.h"
#include "BootFlash.h"

extern KNOWN_FLASH_TYPE aknownflashtypesDefault[];

// this is a ROM-resident wrapper for the function below

 // callback to show progress


bool BootFlashUserInterface(void * pvoidObjectFlash, ENUM_EVENTS ee, DWORD dwPos, DWORD dwExtent) {
	return true;
}

	// if things go well, we won't be coming back from this
	// note this is copied to RAM, and the flash will be changed during its operation
	// therefore no library code nor interrupts can be had

int BootReflashAndReset(BYTE *pbNewData, DWORD dwStartOffset, DWORD dwLength)
{
	OBJECT_FLASH of;
	bool fMore=true;

		// prep our flash object with start address and params

	of.m_pbMemoryMappedStartAddress=(BYTE *)LPCFlashadress;
	of.m_dwStartOffset=dwStartOffset;
	of.m_dwLengthUsedArea=dwLength;
	of.m_pcallbackFlash=BootFlashUserInterface;

		// check device type and parameters are sane

	if(!BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0])) return 1; // unable to ID device - fail
	if(!of.m_fIsBelievedCapableOfWriteAndErase) return 2; // seems to be write-protected - fail
	if(of.m_dwLengthInBytes<(dwStartOffset+dwLength)) return 3; // requested layout won't fit device - sanity check fail

		// committed to reflash now

	__asm__ __volatile__ ( "cli ");  // ISRs are in flash, no interrupts possible now until reset

	while(fMore) {
		if(BootFlashEraseMinimalRegion(&of)) {
			if(BootFlashProgram(&of, pbNewData)) {
				fMore=false;  // good situation
			} else { // failed program
				;
			}
		} else { // failed erase
			;
		}
	}

		// okay, try to restart by cycling power

	__asm__ __volatile__ (
		"mov $0xc004, %dx \n"
		"mov $0x40, %al \n"
		"out %al, %dx \n"
		"mov $0xc008, %dx \n"
		"mov $0x2, %al \n"
		"out %al, %dx \n"
		"mov $0xc006, %dx \n"
		"mov $0xa6, %al \n"
		"out %al, %dx \n"
		"mov $0xc006, %dx \n"
		"in %dx,%al \n"
		"mov $0xc002, %dx \n"
		"mov $0x1a, %al \n"
		"out %al, %dx \n"
		"mov $0xc000, %dx \n"

		"ledspin: in %dx, %al ; cmp $0x10, %al ; jnz ledspin \n"
		"jmp ledspin \n"  // loop forever
	);

	return 0; // keep compiler happy
}
