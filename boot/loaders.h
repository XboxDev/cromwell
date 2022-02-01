#ifndef __BOOT_LOADERS_H__
#define __BOOT_LOADERS_H__

#include "lib/misc/BootParser.h"
#include "fs/fatx/BootFATX.h"

CONFIGENTRY *DetectLinuxCD(int cdromId);
CONFIGENTRY *DetectLinuxFATX(FATXPartition *partition);
CONFIGENTRY *DetectLinuxNative(char *szGrub);
int LoadLinuxCD(CONFIGENTRY *config);
int LoadLinuxFATX(FATXPartition *partition, OPTLINUX *optLinux);
int LoadLinuxNative(char *szGrub, const OPTLINUX *optLinux);
void ExittoLinux(const OPTLINUX *optLinux);

CONFIGENTRY *DetectReactOSCD(int cdromId);
CONFIGENTRY *DetectReactOSFATX(FATXPartition *partition);
CONFIGENTRY *DetectReactOSNative(char *szGrub);
int LoadReactOSCD(CONFIGENTRY *config);
int LoadReactOSFATX(FATXPartition *partition, CONFIGENTRY *config);
int LoadReactOSNative(char *szGrub, CONFIGENTRY *config);
void ExittoReactOS(const OPTMULTIBOOT *multiboot);

int BootLoadFlashCD(int cdromId);

#endif
