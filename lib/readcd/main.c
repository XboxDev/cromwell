#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boot.h>
#include "../../fs/cdrom/iso_fs.h"

int main(int argc, const char * argv[]) {
	char *buffer;
	long max_length = 1024 * 1024 *4;
	long bytes_read;
	int file;
	
	if(argc < 2) {
		printf("usage: readcd isofile\n");
		return 0;
	}

	file = open(argv[1], O_RDONLY);
	if( file == -1) {
		printf("error open file\n");
		return 0;
	}
	buffer = (char *)malloc(max_length);
	
	printf("hansi\n");

	memset(buffer, 0x0, max_length);
	

	bytes_read = BootIso9660GetFile(file, "/", "linuxboo.cfg", max_length);
	if(bytes_read != -1) {
		printf("Bytes read %d\n", bytes_read);
	}
	
	close(file);
	free(buffer);
	return 0;
}
