#ifndef _BootParser_H_
#define _BootParesr_H_

#define MAX_LINE 1024

typedef struct _CONFIGENTRY {
        int  nValid;
	char szPath[MAX_LINE];
        char szKernel[MAX_LINE];
        char szInitrd[MAX_LINE];
        char szAppend[MAX_LINE];
} CONFIGENTRY, *LPCONFIGENTRY;

int ParseConfig(char *szBuffer, CONFIGENTRY *entry);

#endif // _BootParser_H_
