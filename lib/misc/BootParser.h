#ifndef _BootParser_H_
#define _BootParesr_H_

#include "BootEEPROM.h"

#define MAX_LINE 2048

struct CONFIGENTRY;

typedef struct {
        int  nValid;
	char title[15];
	char szPath[MAX_LINE];
        char szKernel[MAX_LINE];
        char szInitrd[MAX_LINE];
        char szAppend[MAX_LINE];
	struct CONFIGENTRY *previousConfigEntry;
	struct CONFIGENTRY *nextConfigEntry;
} CONFIGENTRY;

CONFIGENTRY* ParseConfig(char *szBuffer, unsigned int fileLen, char *szPath);
#endif // _BootParser_H_
