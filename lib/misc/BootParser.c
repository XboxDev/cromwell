#include "boot.h"
#include <string.h>



int ParseConfig(char *szBuffer, CONFIGENTRY *entry, EEPROMDATA *eeprom, char *szPath) {
	static char szLine[MAX_LINE];
	static char szTmp[MAX_LINE];
	static char szNorm[MAX_LINE];
	char *ptr;

	memset(szNorm,0,MAX_LINE);
	memset(entry,0,sizeof(CONFIGENTRY));
	ptr = szBuffer;
	ptr = HelpGetToken(szBuffer,10);
	entry->nValid = 1;

//	printk("Entered Parseconfig\n");
	while(1) {
                memset(szLine,0x00,MAX_LINE);
                _strncpy(szLine,ptr,MAX_LINE);
                szLine[MAX_LINE-1]=0; 	// Make sure, we are Terminated
                
		if(strlen(ptr) < MAX_LINE) {
			if(_strncmp(ptr,"kernel",strlen("kernel")) == 0)  {
				HelpGetParm(szTmp, ptr);
				if(strlen(szPath) > 0) 
					sprintf(entry->szKernel,"%s",szPath);
				if(szTmp[0] != '/')
					sprintf(entry->szKernel,"%s%s",entry->szKernel,"/");
				sprintf(entry->szKernel,"%s%s",entry->szKernel,szTmp);
			}
			if(_strncmp(ptr,"initrd",strlen("initrd")) == 0) {
				HelpGetParm(szTmp, ptr);
				if((strlen(szPath) > 0) &&
					(_strncmp(szTmp, "no", strlen("no")) != 0))
					sprintf(entry->szInitrd,"%s",szPath);
				if(szTmp[0] != '/')
					sprintf(entry->szInitrd,"%s%s",entry->szInitrd,"/");
				sprintf(entry->szInitrd,"%s%s",entry->szInitrd,szTmp);
			}
			if(_strncmp(ptr,"rivafb",strlen("rivafb")) == 0) {
				entry->nRivaFB = 1;
			}
			if(_strncmp(ptr,"xboxfb",strlen("xboxfb")) == 0) {
				entry->nXboxFB = 1;
			}
			if(_strncmp(ptr,"vesafb",strlen("vesafb")) == 0) {
				entry->nVesaFB = 1;
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
				strcpy(szNorm," video=riva:640x480,nomtrr,nohwcursor,tv=NTSC ");
				break;
			case PAL_I:
				strcpy(szNorm," video=riva:640x480,nomtrr,nohwcursor,tv=PAL ");
				break;
			case VID_INVALID:
			default:
				printk("%X  ", (int)((VIDEO_STANDARD )eeprom->VideoStandard));
				break;
		}
	}
	if(entry->nXboxFB == 1) {
		switch(*((VIDEO_STANDARD *)&eeprom->VideoStandard)) {
			case NTSC_M:
				strcpy(szNorm," video=xbox:640x480,nomtrr,nohwcursor,tv=NTSC ");
				break;
			case PAL_I:
				strcpy(szNorm," video=xbox:640x480,nomtrr,nohwcursor,tv=PAL ");
				break;
			case VID_INVALID:
			default:
				printk("%X  ", (int)((VIDEO_STANDARD )eeprom->VideoStandard));
				break;
		}
	}
	if(entry->nVesaFB == 1) {
		switch(*((VIDEO_STANDARD *)&eeprom->VideoStandard)) {
			case NTSC_M:
				strcpy(szNorm," video=vesa:640x480tv=NTSC ");
				break;
			case PAL_I:
				strcpy(szNorm," video=vesa:640x480,tv=PAL ");
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

