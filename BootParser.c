#include "boot.h"
#include "BootParser.h"

int ParseConfig(char *szBuffer, CONFIGENTRY *entry) {
	char szLine[MAX_LINE];
	char szTmp[MAX_LINE];
	char *ptr;
	
	memset(entry,0,sizeof(CONFIGENTRY));
	ptr = szBuffer;
	ptr = HelpGetToken(szBuffer,10);
	entry->nValid = 1;
	while(1) {
		memcpy(szLine,ptr,HelpStrlen(ptr));
		if(HelpStrlen(ptr) < MAX_LINE) {
			if(_strncmp(ptr,"kernel",HelpStrlen("kernel")) == 0)  {
				HelpGetParm(szTmp, ptr);
				if(szTmp[0] != '/')
					HelpCopyUntil(entry->szKernel,"/",MAX_LINE);
				HelpCopyUntil(HelpScan0(entry->szKernel),szTmp,MAX_LINE);
			}
			if(_strncmp(ptr,"initrd",HelpStrlen("initrd")) == 0) {
				HelpGetParm(szTmp, ptr);
				if(szTmp[0] != '/')
					HelpCopyUntil(entry->szInitrd,"/",MAX_LINE);
				HelpCopyUntil(HelpScan0(entry->szInitrd),szTmp,MAX_LINE);
			}
			if(_strncmp(ptr,"append",HelpStrlen("append")) == 0)
				HelpGetParm(entry->szAppend, ptr);
		} else {
			entry->nValid = 0;
		}
		ptr = HelpGetToken(0,10);
		if(*ptr == 0) break;
	}
	return entry->nValid;
}

