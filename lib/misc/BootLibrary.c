
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#include "boot.h"


void BootPciInterruptEnable()  {	__asm__ __volatile__  (  "sti" ); }



void * memcpy(void *dest, const void *src,  size_t size) {

#if 0
	BYTE * pb=(BYTE *)src, *pbd=(BYTE *)dest;
	while(size--) *pbd++=*pb++;

#else
		__asm__ __volatile__ (
              "    push %%esi    \n"
              "    push %%edi    \n"
              "    push %%ecx    \n"
              "    cld    \n"
              "    mov %0, %%esi \n"
              "    mov %1, %%edi \n"
              "    mov %2, %%ecx \n"
              "    push %%ecx    \n"
              "    shr $2, %%ecx \n"
              "    rep movsl     \n"
              "    pop %%ecx     \n"
              "    and $3, %%ecx \n"
              "    rep movsb     \n"
              : :"S" (src), "D" (dest), "c" (size)
		);

		__asm__ __volatile__ (
	      "    pop %ecx     \n"
              "    pop %edi     \n"
              "    pop %esi     \n"
		);
#endif
}

size_t strlen(const char *s) 
{
  	register size_t i;
  	if (!s) return 0;
  	for (i=0; *s; ++s) ++i;
  	return i;
}

int tolower(int ch) 
{
  	if ( (unsigned int)(ch - 'A') < 26u )
    		ch += 'a' - 'A';
  	return ch;
}

int isspace (int c)
{
  	if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return 1;
  	return 0;
}

void * memset(void *dest, int data,  size_t size)
{
  	char *p = dest;
	while (size -- > 0)
	{
		*p ++ = data;
	}
}
                              
int memcmp(const void *buffer1, const void *buffer2, size_t num) 
{
  	register int r;
  	register const char *d=buffer1;
  	register const char *s=buffer2;
  	while (num--) {
    		if ((r=(*d - *s))) return r;
    		++d;
    		++s;
  	}
  	return 0;
}

char * strcpy(char *sz, const char *szc)
{
	char *szStart=sz;
	while(*szc) *sz++=*szc++;
	*sz='\0';
	return szStart;
}

char * _strncpy (char * dest,const char * src, size_t n)
{
	char *szStart=dest;
	while((*src) && (n--)) *dest++=*src++;
	*dest='\0';
	return szStart;
}


int _strncmp(const char *sz1, const char *sz2, int nMax) 
{

	while((*sz1) && (*sz2) && nMax--) {
		if(*sz1 != *sz2) return (*sz1 - *sz2);
		sz1++; sz2++;
	}
	if(nMax==0) return 0;
	if((*sz1) || (*sz2)) return 0;
	return 0; // used up nMax
}

char *strrchr0(char *string, char ch) 
{
        char *ptr = string;
	while(*ptr != 0) {
		if(*ptr == ch) {
			return ptr;
		} else {
			ptr++;
		}
	}
	return NULL;
}



/* -------------------------------------------------------------------- */



unsigned int MemoryManagerStartAddress=0;

int MemoryManagerGetFree(void) {
	
	unsigned char *memsmall = (void*)MemoryManagerStartAddress;
	int freeblocks = 0;
	int counter;
	for (counter=0;counter<0x400;counter++) {
		if (memsmall[counter]==0x0) freeblocks++;	
	}
	return freeblocks;
		
}

void MemoryManagementInitialization(void * pvStartAddress, DWORD dwTotalMemoryAllocLength)
{
	unsigned char *mem = pvStartAddress;
	//if (dwTotalMemoryAllocLength!=0x1000000) return 1;

	MemoryManagerStartAddress = (unsigned int)pvStartAddress;

	// Prepare the memory cluster Table to be free
	memset(mem,0x0,0x4000);
	// Set the first cluster to be "reserved"
	mem[0] = 0xff;

	return ;
}


void * t_malloc(size_t size)
{
	unsigned char *memsmall = (void*)MemoryManagerStartAddress;
	unsigned int counter;
	unsigned int dummy = 0;
	unsigned int blockcount = 0;
	unsigned int temp;

	// this is for 16 kbyes allocations (quick & fast)
	if (size<(0x4000+1)) {
		for (counter=1;counter<0x400;counter++) {
			if (memsmall[counter]==0)
			{
				memsmall[counter] = 0xAA;
				return (void*)(MemoryManagerStartAddress+counter*0x4000);
			}
		}
		// No free Memory found
		return 0;
	}

	// this is for 64 kbyte allocations (also quick)
	if (size<(0x10000+1)) {
		for (counter=1;counter<0x400;counter++) {
			if (memcmp(&memsmall[counter],&dummy,4)==0)
			{
				dummy = 0xB8BADCFE;
				memcpy(&memsmall[counter],&dummy,4);
				return (void*)(MemoryManagerStartAddress+counter*0x4000);
			}
		}
		// No free Memory found
		return 0;
	}

	if (size<(5*1024*1024+1)) {

		for (counter=1;counter<0x400;counter++) {
			unsigned int needsectory;
			unsigned int foundstart;

			temp = (size & 0xffffc000) + 0x4000;
			needsectory = temp / 0x4000;

			//printf("Need Sectors %x\n",needsectory);

			foundstart = 1;
			for (blockcount=0;blockcount<needsectory;blockcount++) {
				if (memsmall[counter+blockcount]!=0	) {
					foundstart = 0;
					break;
				}
			}

			if (foundstart == 1)
			{
				// We found a free sector
				//printf("Found Sectors Start %x\n",counter);
				memset(&memsmall[counter],0xFF,needsectory);
				memsmall[counter] = 0xBB;
				memsmall[counter+1] = 0xCC;
				memsmall[counter+needsectory-2] = 0xCC;
				memsmall[counter+needsectory-1] = 0xBB;

				return (void*)(MemoryManagerStartAddress+counter*0x4000);
			}

		}
		return 0;
	}

	return 0;

}



void t_free (void *ptr)
{
	unsigned char *memsmall = (void*)MemoryManagerStartAddress;
	unsigned int temp;
	unsigned int dummy = 0;
	unsigned int point = (unsigned int)ptr;

	// this is the offset of the Free thing
	temp = point - MemoryManagerStartAddress;

	if ((temp & 0xffffc000) == temp)
	{
		// Allignement OK
		temp = temp / 0x4000;
		//printf("Free %x\n",temp);

		if (memsmall[temp] == 0xAA)
		{
			// Found Small Block, free it
			ptr = NULL;
			memsmall[temp] = 0x0;
			return;
		}

		dummy = 0xB8BADCFE;
		if (memcmp(&memsmall[temp],&dummy,4)==0)
		{
			// Found 64 K block, free it
			ptr = NULL;
			dummy = 0;
			memset(&memsmall[temp],dummy,4);
			return;
		}


		dummy = 0xFFFFCCBB;
		if (memcmp(&memsmall[temp],&dummy,4)==0)
		{
			unsigned int counter;
			// Found 64 K block, free it
			//printf("Found Big block %x\n",temp);
			ptr = NULL;
			for (counter=temp;counter<0x400;counter++)
			{
				if ((memsmall[counter]==0xCC)&(memsmall[counter+1]==0xBB))
				{
					// End detected
					memsmall[counter]=0;
					memsmall[counter+1]=0;
					return;
				}
				memsmall[counter]=0;
			}
			return;
		}

	}

}



void * malloc(size_t size) {

	size_t temp;
	unsigned char *tempmalloc;
	unsigned int *tempmalloc1;
	unsigned int *tempmalloc2;
         __asm__ __volatile__  (  "cli" );
         
	temp = (size+0x200) & 0xffFFff00;

	tempmalloc = t_malloc(temp);
	tempmalloc2 = (unsigned int*)tempmalloc;

	tempmalloc = (unsigned char*)((unsigned int)(tempmalloc+0x100) & 0xffFFff00);
	tempmalloc1 = (unsigned int*)tempmalloc;
	tempmalloc1--;
	tempmalloc1--;
	tempmalloc1[0] = (unsigned int)tempmalloc2;
	tempmalloc1[1] = 0x1234567;
	__asm__ __volatile__  (  "sti" );
		
	return tempmalloc;
}

void free(void *ptr) {

	unsigned int *tempmalloc1;
        __asm__ __volatile__  (  "cli" );
      	
      	if (ptr == NULL) return;
      	  
	tempmalloc1 = ptr;
	tempmalloc1--;
	tempmalloc1--;
	ptr = (unsigned int*)tempmalloc1[0];
        if (tempmalloc1[1]!= 0x1234567) {
        	__asm__ __volatile__  (  "sti" );
        	return ;
	}        
	t_free(ptr);
	__asm__ __volatile__  (  "sti" );

}
 
 



void ListEntryInsertAfterCurrent(LIST_ENTRY *plistentryCurrent, LIST_ENTRY *plistentryNew)
{
	plistentryNew->m_plistentryPrevious=plistentryCurrent;
	plistentryNew->m_plistentryNext=plistentryCurrent->m_plistentryNext;
	plistentryCurrent->m_plistentryNext=plistentryNew;
	if(plistentryNew->m_plistentryNext!=NULL) {
		plistentryNew->m_plistentryNext->m_plistentryPrevious=plistentryNew;
	}
}

void ListEntryRemove(LIST_ENTRY *plistentryCurrent)
{
	if(plistentryCurrent->m_plistentryPrevious) {
		plistentryCurrent->m_plistentryPrevious->m_plistentryNext=plistentryCurrent->m_plistentryNext;
	}
	if(plistentryCurrent->m_plistentryNext) {
		plistentryCurrent->m_plistentryNext->m_plistentryPrevious=plistentryCurrent->m_plistentryPrevious;
	}
}

char *HelpGetToken(char *ptr,char token) {
	static char *old;
	char *mark;

	if(ptr != 0) old = ptr;
	mark = old;
	for(;*old != 0;old++) {
		if(*old == token) {
			*old = 0;
			old++;
			break;
		}
	}
	return mark;
}

void HelpGetParm(char *szBuffer, char *szOrig) {
	char *ptr,*copy;
	int nBeg = 0;
	int nCopy = 0;

	copy = szBuffer;
	for(ptr = szOrig;*ptr != 0;ptr++) {
		if(*ptr == ' ') nBeg = 1;
		if(*ptr != ' ' && nBeg == 1) nCopy = 1;
		if(nCopy == 1) {
			*copy = *ptr;
		 	copy++;
		}
	}
	*copy = 0;
}


