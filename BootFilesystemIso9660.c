/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2002-12-17 andy@warmcat.com  Created
*/

#include "boot.h"
#include <ctype.h>

// ISO9660 likes fixed length, space-padded strings, we don't.

void BootIso9660DescriptorToString(const char * szcDescriptor, int nLength, char * szStringResult)
{
	int n=0, n1=0, nLastNonspace=0;

	while(n<nLength) {
		if((n1!=0) || (szcDescriptor[n]!=' ')) {
			szStringResult[n1++]=szcDescriptor[n];
			if(szcDescriptor[n]!=' ') nLastNonspace=n1;
		}
		n++;
	}
	szStringResult[nLastNonspace]='\0';
}

void BootIso9660ConvertAsciiStringToDchar8point3String(char *szIso, const char *szAscii)
{
	int n=0;
	szIso[0]='/';
	while(n<8) {
		if((szAscii[n]>='a') && (szAscii[n]<='z')) {
			szIso[n+1]=szAscii[n]-('a'-'A');
		} else {
			if(
				( (szAscii[n]>='0') && (szAscii[n]<='9') ) ||
				( (szAscii[n]>='A') && (szAscii[n]<='Z') )
			) {
				szIso[n+1]=szAscii[n];
			} else {
				szIso[n+1]='_';
			}
		}
		n++;
	}
	szIso[n+1]='.'; szIso[n+2]='\0';
}

// this is a helper function for BootIso9660GetFileDetails() below
// returns zero if error, 1 if file, 2 if directory

int BootIso9660GoDownOneLevelOrHit(DWORD dwSector, DWORD dwBytesToScan, const char * szcName, ISO_SYSTEM_DIRECTORY_RECORD * pisdrForFile)
{
	BYTE ba[2048];
	ISO_SYSTEM_DIRECTORY_RECORD * pisdr=(ISO_SYSTEM_DIRECTORY_RECORD *)&ba[0];

//	printk("BootIso9660GoDownOneLevelOrHit(0x%x, 0x%x, %s ...)\n", dwSector, dwBytesToScan, szcName);

	while((dwBytesToScan>=0x800) || (dwSector==ROOT_SECTOR)) {

		if(dwBytesToScan>=0x800) dwBytesToScan-=0x800; else dwBytesToScan=0;

		if(BootIdeReadSector(1, &ba[0], dwSector, 0, 2048)) return 0;

			// root sector has a single struct pointing to root dir

		if(dwSector==ROOT_SECTOR) {
			pisdr=(ISO_SYSTEM_DIRECTORY_RECORD *)&ba[ROOT_DIR_STRUCT_OFFSET];
//			VideoDumpAddressAndData(0, &ba[0], 0x200);
//			printk("Root dir length=0x%x, 0x%lx, 0x%lx\n",
//				pisdr->m_bLength,
//				pisdr->m_dwrExtentLocation.m_dwLittleEndian,
//				pisdr->m_dwrDataLength.m_dwLittleEndian
//			);
			*pisdrForFile = *pisdr;
			return 2;
		}

		dwSector++;

			// otherwise we are looking at a number of back-to-back structs
			// each talking about a file

		while(pisdr->m_bLength) {

			char *szc = (char *)(&pisdr->m_cFirstFileIdPlaceholder);
			 /* {
				char sz[64];
				memcpy(sz, szc, 64);
				sz[pisdr->m_bFileIdentifierLength]='\0';
				printk("'%s'(%d)\n", sz, pisdr->m_bFileIdentifierLength);
			} */
			int n=0, n1=0;
			bool fGood=true;
			while(n<pisdr->m_bFileIdentifierLength) {
				if(szc[n]==';') { n=pisdr->m_bFileIdentifierLength; continue; }
				if(szc[n++]!=szcName[n1++]) { fGood=false; n=pisdr->m_bFileIdentifierLength; }
			}
			if(fGood) if(szcName[n1]!='\0') fGood=false;

			if(fGood) {
				*pisdrForFile = *pisdr; // copy over results
				if(pisdr->m_bFileFlags & 1) {
//					printk("returning dir hit\n");
					return 2; // its a hit: its a directory
				}
//				printk("returning file hit\n");
				return 1; // its a hit: its a file
			}

			pisdr=(ISO_SYSTEM_DIRECTORY_RECORD *)(((BYTE *)pisdr)+pisdr->m_bLength);
		}

//		printk("*");

	}

//	printk("returning fail\n");

	return 0;
}

// this guy fills pisdr with details on the file location and attributes if the file can be found
// note that you must use 8.3 names, the lowest common denominator
// return 0 if found, otherwise an error index

int BootIso9660GetFileDetails(const char * szcPath, ISO_SYSTEM_DIRECTORY_RECORD * pisdr)
{
	char szPathElement[MAX_PATH_ELEMENT_SIZE];
	const char * szc=&szcPath[0];
	int n=0, nReturn;
	bool fMore=true;

	if(*szcPath!='/') return -2; // no relative paths

	while(fMore) {
		if((*szc=='/') || (*szc=='\0')) {
			if(*szc) {
				szc++; // move past it
			} else {
				fMore=false;
			}
			szPathElement[n]='\0';
			if(n==0) {
				nReturn=BootIso9660GoDownOneLevelOrHit(ROOT_SECTOR, 0, szPathElement, pisdr);
			} else {
				nReturn=BootIso9660GoDownOneLevelOrHit(
					pisdr->m_dwrExtentLocation.m_dwLittleEndian,
					pisdr->m_dwrDataLength.m_dwLittleEndian,
					szPathElement,
					pisdr
				);
			}
			if(nReturn==0) return -4; // unable to find path element
			if(nReturn==1) { // found file
				return 0;
			}
			n=0;
		}
		if((*szc>='a') && (*szc<='z')) { // convert to uppercase
			szPathElement[n++]=(*szc++)-('a'-'A');
		} else {
			szPathElement[n++]=*szc++;
		}
		if(n==sizeof(szPathElement)) return -1; // avoid buffer overflow, path element too long
	}
	return -2;
}


int BootIso9660GetFile(const char *szcPath, BYTE *pbaFile, DWORD dwFileLengthMax, DWORD dwOffset)
{
	ISO_SYSTEM_DIRECTORY_RECORD isdr;
	int nReturn=BootIso9660GetFileDetails(szcPath, &isdr);
	DWORD dwSector;
	DWORD dwFileLengthTaken;

	if(nReturn) return nReturn;

	dwSector=isdr.m_dwrExtentLocation.m_dwLittleEndian + (dwOffset / 2048);
	dwOffset=dwOffset % 2048;
	if(dwFileLengthMax > isdr.m_dwrDataLength.m_dwLittleEndian) {

		dwFileLengthMax=isdr.m_dwrDataLength.m_dwLittleEndian;
	}
	dwFileLengthTaken=dwFileLengthMax;

//	printk("File length used = 0x%x, 0x%x\n", (int)dwFileLengthMax, (int)isdr.m_dwrDataLength.m_dwLittleEndian);

	while(dwFileLengthMax) {
		DWORD dwLengthThisTime=2048;
		if(dwLengthThisTime >dwFileLengthMax) dwLengthThisTime=dwFileLengthMax;

		if(dwOffset || (dwLengthThisTime<2048)) {
			BYTE ba[2048];
			dwLengthThisTime-=dwOffset;
			if(BootIdeReadSector(1, &ba[0], dwSector, 0, 2048)) return -6;
			memcpy(pbaFile, &ba[dwOffset], dwLengthThisTime);
			dwOffset=0;
		} else {
			if(BootIdeReadSector(1, pbaFile, dwSector, 0, 2048)) return -6;
		}
		dwSector++;
		dwFileLengthMax-=dwLengthThisTime;
		pbaFile+=dwLengthThisTime;
	}
	return (int)dwFileLengthTaken;
}
