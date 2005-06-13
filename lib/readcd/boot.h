#ifndef BOOT_H
#define BOOT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define printk printf

int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes) {

	lseek(nDriveIndex, block * 2048, SEEK_SET);
	read(nDriveIndex, pbBuffer, 2048);
	return 0;

};

#endif
