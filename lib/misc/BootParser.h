#ifndef _BootParser_H_
#define _BootParesr_H_

#include "BootEEPROM.h"

#define MAX_LINE 1024

typedef struct _CONFIGENTRY {
        int  nValid;
	char szPath[MAX_LINE];
        char szKernel[MAX_LINE];
        char szInitrd[MAX_LINE];
        char szAppend[MAX_LINE];
        int nXboxFB;
        int nRivaFB;
	int nVesaFB;
} CONFIGENTRY, *LPCONFIGENTRY;

int ParseConfig(char *szBuffer, CONFIGENTRY *entry, EEPROMDATA *eeprom);

#endif // _BootParser_H_
