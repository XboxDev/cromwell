#include "boot.h"
#include <string.h>
#include "BootParser.h"


int ParseConfig(char *szBuffer, CONFIGENTRY *entry, EEPROMDATA *eeprom) {
	char szLine[MAX_LINE];
	char szTmp[MAX_LINE];
	char szNorm[MAX_LINE];
	char *ptr;
	
	memset(szNorm,0,MAX_LINE);
	memset(entry,0,sizeof(CONFIGENTRY));
	ptr = szBuffer;
	ptr = HelpGetToken(szBuffer,10);
	entry->nValid = 1;
	while(1) {
		memcpy(szLine,ptr,strlen(ptr));
		if(strlen(ptr) < MAX_LINE) {
			if(_strncmp(ptr,"kernel",strlen("kernel")) == 0)  {
				HelpGetParm(szTmp, ptr);
				if(szTmp[0] != '/')
					sprintf(entry->szKernel,"%s","/");
				sprintf(entry->szKernel,"%s%s",entry->szKernel,szTmp);
			}
			if(_strncmp(ptr,"initrd",strlen("initrd")) == 0) {
				HelpGetParm(szTmp, ptr);
				if(szTmp[0] != '/')
					sprintf(entry->szInitrd,"%s","/");
				sprintf(entry->szInitrd,"%s%s",entry->szInitrd,szTmp);
			}
			if(_strncmp(ptr,"rivafb",strlen("rivafb")) == 0) {
				entry->nRivaFB = 1;
			}
			if(_strncmp(ptr,"append",strlen("append")) == 0)
				sprintf(entry->szAppend,"%s",ptr);
		} else {
			entry->nValid = 0;
		}
		ptr = HelpGetToken(0,10);
		if(*ptr == 0) break;
	}
	if(entry->nRivaFB == 1) {
		switch(*((VIDEO_STANDARD *)&eeprom->VideoStandard)) {
			case NTSC_M:
				strcpy(szNorm," video=riva:640x480,nomtrr,nohwcursor,fb_mem=4M,tv=NTSC ");
				break;
			case PAL_I:
				strcpy(szNorm," video=riva:640x480,nomtrr,nohwcursor,fb_mem=4M,tv=PAL ");
				break;
			case VID_INVALID:
			default:
				printk("%X  ", (int)((VIDEO_STANDARD )eeprom->VideoStandard));
				break;
		}
	}
	if(szNorm[0] != 0) {
		sprintf(entry->szAppend,"%s%s",entry->szAppend,szNorm);
	}
	return entry->nValid;
}

