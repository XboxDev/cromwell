#ifndef _BootParser_H_
#define _BootParser_H_

#include "BootEEPROM.h"

#define MAX_CONFIG_FILESIZE 1024*256
#define MAX_LINE 2048

struct CONFIGENTRY;

enum BootTypes {
	BOOT_CDROM,
	BOOT_FATX,
	BOOT_NATIVE,
};

enum BootSystems {
	SYS_LINUX,
};

typedef struct OPTLINUX {
	char szKernel[MAX_LINE];
	char szInitrd[MAX_LINE];
	char szAppend[MAX_LINE];
} OPTLINUX;

typedef struct CONFIGENTRY {
	int  drive;
	int  partition;
	int  isDefault;
	enum BootTypes bootType;
	enum BootSystems bootSystem;
	char title[15];
	char szPath[MAX_LINE];
	union {
		struct OPTLINUX Linux;
	} opt;
	struct CONFIGENTRY *previousConfigEntry;
	struct CONFIGENTRY *nextConfigEntry;
	struct CONFIGENTRY *nestedConfigEntry;
} CONFIGENTRY;

CONFIGENTRY* ParseConfig(char *szBuffer, unsigned int fileLen, char *szPath);
#endif // _BootParser_H_
