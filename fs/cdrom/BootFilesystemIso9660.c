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
#include <string.h>

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


// this is a helper function for BootIso9660GetFileDetails() below
// returns zero if error, 1 if file, 2 if directory

int BootIso9660GoDownOneLevelOrHit(DWORD dwSector, DWORD dwBytesToScan, const char * szcName, ISO_SYSTEM_DIRECTORY_RECORD * pisdrForFile)
{
	BYTE *ba=malloc(dwBytesToScan);  // enough for 50 sectors
	ISO_SYSTEM_DIRECTORY_RECORD * pisdr=(ISO_SYSTEM_DIRECTORY_RECORD *)&ba[0];
	int nPos=0;
	DWORD dwTotalLength;
//	printk("BootIso9660GoDownOneLevelOrHit(0x%x, 0x%x, %s ...)\n", dwSector, dwBytesToScan, szcName);

	dwTotalLength=dwBytesToScan;

		// pull in the whole dir struct at once, to avoid probs at sector boundaries

	while(dwBytesToScan || (dwSector==ROOT_SECTOR)) {

		if(dwBytesToScan>=0x800) dwBytesToScan-=0x800; else dwBytesToScan=0;

		if(BootIdeReadSector(1, &ba[nPos], dwSector, 0, 2048)) return 0;
		nPos+=2048;
		dwSector++;
	}

			// root sector has a single struct pointing to root dir

	if((dwSector-1)==ROOT_SECTOR) {
		pisdr=(ISO_SYSTEM_DIRECTORY_RECORD *)&ba[ROOT_DIR_STRUCT_OFFSET];
		*pisdrForFile = *pisdr;
		free(ba);
		return 2;
	}

			// otherwise we are looking at a number of back-to-back structs
			// each talking about a file

	nPos=0;

	while((((BYTE *)pisdr)-&ba[0])<dwTotalLength) {  // while we haven't gone off the end the dir structs

		char *szc = (char *)(&pisdr->m_cFirstFileIdPlaceholder);
			// compute what's left after total struct size and 8.3 name
		int nCharsLeft=pisdr->m_bLength - ((szc-((char *)pisdr))  );

			// check mangled 8.3 version

//		{ int n=0; for(n=0;n<pisdr->m_bFileIdentifierLength;n++) { printk("%c", szc[n]); } printk("\n"); }

		 {

			int n=0;
			while((n<strlen(szcName)) && (szc[n]==szcName[n]) && (szc[n]!=';')) n++;

			if((n==strlen(szcName)) && (szc[n]==';')) {

				*pisdrForFile = *pisdr; // copy over results
				if(pisdr->m_bFileFlags & 1) {
//						printk("returning dir hit\n");
					free(ba);
					return 2; // its a hit: its a directory
				}
//					printk("returning file hit\n");
				free(ba);
				return 1; // its a hit: its a file
			}
		}

		nCharsLeft-=pisdr->m_bFileIdentifierLength;
		szc+=pisdr->m_bFileIdentifierLength; // skip over the horrible mangled 8.3 version

//		 VideoDumpAddressAndData((DWORD)pisdr, (BYTE *)szc-0x10, 0x20);

//		printk("pisdr->m_bFileIdentifierLength=%d\n", pisdr->m_bFileIdentifierLength);
		if(pisdr->m_bFileIdentifierLength==0) { // uh oh
//				printk("returning, zero length identifier seen\n");
				free(ba);
				return 0;
		}
		if(*szc=='\0') szc++;  // 00 separator on some, not on others.  Can never be 00 by accident

			// structure of extended names seems to be two char extension descriptor, eg, RR
			// followed by byte length count of extension including descriptor chars
			// then for all descriptors an LH format version word
			// for the name that we want the extension descriptor is NM (name?)

			// first we iterate through the descriptors looking for NM

		{
			bool fSeen=false;
			int nHitLength=0;

			while((nCharsLeft>1) && (!fSeen)) {

				if((*szc=='N') && (szc[1]=='M')) {
					fSeen=true;
					nHitLength=((BYTE)szc[2])-5;
					szc+=5;
					continue;
				}
				nCharsLeft-=szc[2];
				szc+=(int)(BYTE)szc[2];
			}

			if(fSeen) { // szc is aligned to start of candidate extended name, nHitLength is length of that name
				int n=0, n1=0;
				bool fGood=true;

				while(n<nHitLength) {
					if(szcName[n1]=='\0') { fGood=false; n=nHitLength; }
					if(szc[n++]!=szcName[n1++]) { fGood=false; n=nHitLength; }
				}

				if(fGood) if(szcName[n1]!='\0') fGood=false;

				if(fGood) {
					*pisdrForFile = *pisdr; // copy over results
					if(pisdr->m_bFileFlags & 1) {
//						printk("returning dir hit\n");
						free(ba);
						return 2; // its a hit: its a directory
					}
//					printk("returning file hit\n");
					free(ba);
					return 1; // its a hit: its a file
				} // if fGood
			} // if fSeen
		}

		pisdr=(ISO_SYSTEM_DIRECTORY_RECORD *)(((BYTE *)pisdr)+pisdr->m_bLength);
	} // while there are more dir structs

//	printk("returning fail\n");
	free(ba);
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
		szPathElement[n++]=*szc++;

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
