#include "etherboot.h"

/*
 * This is a helper function; it tries to execute an ELF image using
 * the Etherboot code. This is because *BSD kernels are raw ELF files,
 * which are unsupported when booting from CD-ROM or disk.
 */
void
try_elf_boot (char* image, int len)
{
	os_download_t os_download;

	/* do nothing if the ELF magic mismatches */
	if (*(int*)image != 0x464c457f)
		return;

	os_download = probe_image (image, len);
	if (os_download == 0)
		return;

	os_download (image, len, 1);
}
