/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Copyright (C) 2007 Ge van Geldorp                                     *
 *   Copyright (C) 2019 Stanislav Motylkov                                 *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "BootFATX.h"
#include "memory_layout.h"
#include <shared.h>

extern grub_error_t errnum;

/* Length of area to be searched for multiboot header. */
#define MULTIBOOT_SEARCHAREA_LEN        8192

/* Magic numbers for the Multiboot 1 specification. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

#define MB_INFO_FLAG_MEM_SIZE           0x00000001
#define MB_INFO_FLAG_BOOT_DEVICE        0x00000002
#define MB_INFO_FLAG_COMMAND_LINE       0x00000004
#define MB_INFO_FLAG_MODULES            0x00000008
#define MB_INFO_FLAG_AOUT_SYMS          0x00000010
#define MB_INFO_FLAG_ELF_SYMS           0x00000020
#define MB_INFO_FLAG_MEMORY_MAP         0x00000040
#define MB_INFO_FLAG_DRIVES             0x00000080
#define MB_INFO_FLAG_CONFIG_TABLE       0x00000100
#define MB_INFO_FLAG_BOOT_LOADER_NAME   0x00000200
#define MB_INFO_FLAG_APM_TABLE          0x00000400
#define MB_INFO_FLAG_GRAPHICS_TABLE     0x00000800

/* The Multiboot header. */
typedef struct tagMULTIBOOTHEADER {
	u32 magic;
	u32 flags;
	u32 checksum;
	u32 header_addr;
	u32 load_addr;
	u32 load_end_addr;
	u32 bss_end_addr;
	u32 entry_addr;
} MULTIBOOTHEADER, *PMULTIBOOTHEADER;

/* The symbol table for a.out. */
typedef struct tagAOUTSYMBOLTABLE {
	u32 tabsize;
	u32 strsize;
	u32 addr;
	u32 reserved;
} AOUTSYMBOLTABLE, *PAOUTSYMBOLTABLE;

/* The section header table for ELF. */
typedef struct tagELFSECTIONHEADERTABLE {
	u32 num;
	u32 size;
	u32 addr;
	u32 shndx;
} ELFSECTIONHEADERTABLE, *PELFSECTIONHEADERTABLE;

/* The Multiboot information. */
typedef struct tagMULTIBOOTINFO {
	u32 flags;
	u32 mem_lower;
	u32 mem_upper;
	u32 boot_device;
	u32 cmdline;
	u32 mods_count;
	u32 mods_addr;
	union {
		AOUTSYMBOLTABLE aout_sym;
		ELFSECTIONHEADERTABLE elf_sec;
	} u;
	u32 mmap_length;
	u32 mmap_addr;
	u32 drives_length;
	u32 drives_addr;
	u32 config_table;
	u32 boot_loader_name;
	u32 apm_table;
	u32 vbe_control_info;
	u32 vbe_mode_info;
	u32 vbe_mode;
	u32 vbe_interface_seg;
	u32 vbe_interface_off;
	u32 vbe_interface_len;
} MULTIBOOTINFO, *PMULTIBOOTINFO;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct tagMEMORYMAP {
	u32 size;
	u32 base_addr_low;
	u32 base_addr_high;
	u32 length_low;
	u32 length_high;
	u32 type;
} MEMORYMAP, *PMEMORYMAP;

void startReactOS(PMULTIBOOTHEADER mbHeader, u32 loaderSize, u32 bootDevice);

u32 GetMultibootBootDevice(u8 drive, u8 partition, u8 subpartition1, u8 subpartition2) {
	return (subpartition2 | (subpartition1 << 8) | (partition << 16) | (drive << 24));
}

PMULTIBOOTHEADER CheckMultibootHeader(u8 *buffer) {
	PMULTIBOOTHEADER mbHeader;

	for (mbHeader = (PMULTIBOOTHEADER)buffer;
		(u8 *)mbHeader - buffer < MULTIBOOT_SEARCHAREA_LEN;
		mbHeader = (PMULTIBOOTHEADER)((u8 *)mbHeader + 4))
	{
		if (mbHeader->magic == MULTIBOOT_HEADER_MAGIC &&
			mbHeader->magic + mbHeader->flags + mbHeader->checksum == 0) {
			break;
		}
	}

	if ((u8 *)mbHeader - buffer >= MULTIBOOT_SEARCHAREA_LEN) {
		printk("No multiboot header found\n");
		return NULL;
	}

	return mbHeader;
}

CONFIGENTRY *DetectReactOSNative(char *szGrub) {
	CONFIGENTRY *config;
	int nRet;

	strcpy(&szGrub[4], "/freeldr.sys");
	nRet = grub_open(szGrub);

	if (nRet != 1 || errnum) {
		// File not found
		errnum = 0;
		return NULL;
	}

	config = (CONFIGENTRY *)malloc(sizeof(CONFIGENTRY));
	memset(config, 0x00, sizeof(CONFIGENTRY));
	config->bootSystem = SYS_REACTOS;
	strcpy(config->title, "ReactOS");
	strcpy(config->szPath, "/freeldr.sys");

	grub_close();
	return config;
}

int LoadReactOSNative(char *szGrub, CONFIGENTRY *config) {
	int nRet;
	long dwSize;
	OPTMULTIBOOT *multiboot = &config->opt.Multiboot;

	VIDEO_ATTR = 0xffd8d8d8;
	printk("  Loading %s ", config->szPath);
	VIDEO_ATTR = 0xffa8a8a8;

	strncpy(&szGrub[4], config->szPath, strlen(config->szPath));

	nRet = grub_open(szGrub);

	if (nRet != 1) {
		printk("Unable to load file, Grub error %d\n", errnum);
		errnum = 0;
		wait_ms(2000);
		return false;
	}

	dwSize = grub_read(FREELDR_LOAD_AREA, filemax);
	grub_close();
	printk(" - %d bytes\n", dwSize);

	if (CheckMultibootHeader(FREELDR_LOAD_AREA) == NULL) {
		wait_ms(2000);
		return false;
	}
	multiboot->pBuffer = FREELDR_LOAD_AREA;
	multiboot->uBufferSize = dwSize;
	/* Multiboot partition indices starting at 0 */
	multiboot->uBootDevice = GetMultibootBootDevice(0x80 + config->drive, config->partition, 0xFF, 0xFF);

	return true;
}

CONFIGENTRY *DetectReactOSFATX(FATXPartition *partition) {
	FATXFILEINFO fileinfo;
	CONFIGENTRY *config = NULL;

	if (LoadFATXFile(partition, "/freeldr.sys", &fileinfo)) {
		// Root of E has a freeldr.sys in
		config = (CONFIGENTRY *)malloc(sizeof(CONFIGENTRY));
		memset(config, 0x00, sizeof(CONFIGENTRY));
		config->bootSystem = SYS_REACTOS;
		strcpy(config->title, "ReactOS");
		strcpy(config->szPath, "/freeldr.sys");

		free(fileinfo.buffer);
	}

	return config;
}

int LoadReactOSFATX(FATXPartition *partition, CONFIGENTRY *config) {
	static FATXFILEINFO fileinfo;
	OPTMULTIBOOT *multiboot = &config->opt.Multiboot;

	memset(&fileinfo, 0x00, sizeof(fileinfo));

	VIDEO_ATTR = 0xffd8d8d8;
	printk("  Loading %s from FATX", config->szPath);
	if (!LoadFATXFilefixed(partition, config->szPath, &fileinfo, FREELDR_LOAD_AREA)) {
		printk("Error loading %s\n", config->szPath);
		wait_ms(2000);
		return false;
	} else {
		printk(" - %d bytes\n", fileinfo.fileRead);

		if (CheckMultibootHeader(fileinfo.buffer) == NULL) {
			wait_ms(2000);
			return false;
		}
		multiboot->pBuffer = fileinfo.buffer;
		multiboot->uBufferSize = fileinfo.fileRead;
		/* ReactOS FreeLoader and drivers treat FATX (E:) partition as the first one,
		 * so we specify 0 here as the second argument. */
		multiboot->uBootDevice = GetMultibootBootDevice(0x80 + partition->nDriveIndex, 0, 0xFF, 0xFF);
	}

	return true;
}

CONFIGENTRY *DetectReactOSCD(int cdromId) {
	long dwSize;
	CONFIGENTRY *config;

	dwSize = BootIso9660GetFile(cdromId, "/loader/setupldr.sys", FREELDR_LOAD_AREA, FREELDR_MAX_SIZE);

	// Failed to load freeloader
	if (dwSize <= 0)
		return NULL;

	// Freeloader found
	config = (CONFIGENTRY *)malloc(sizeof(CONFIGENTRY));
	memset(config, 0x00, sizeof(CONFIGENTRY));
	config->bootSystem = SYS_REACTOS;
	strcpy(config->title, "ReactOS");
	strcpy(config->szPath, "/loader/setupldr.sys");

	return config;
}

int LoadReactOSCD(CONFIGENTRY *config) {
	long dwSize;
	OPTMULTIBOOT *multiboot = &config->opt.Multiboot;

	memset(FREELDR_LOAD_AREA, 0, FREELDR_MAX_SIZE);

	VIDEO_ATTR = 0xffd8d8d8;
	printk("  Loading %s from CD", config->szPath);
	VIDEO_ATTR = 0xffa8a8a8;
	dwSize = BootIso9660GetFile(config->drive, config->szPath, FREELDR_LOAD_AREA, FREELDR_MAX_SIZE);

	if (dwSize < 0) {
		printk("Not Found, error %d\nHalting\n", dwSize);
		wait_ms(2000);
		return false;
	} else {
		printk(" - %d bytes\n", dwSize);

		if (CheckMultibootHeader(FREELDR_LOAD_AREA) == NULL) {
			wait_ms(2000);
			return false;
		}
		multiboot->pBuffer = FREELDR_LOAD_AREA;
		multiboot->uBufferSize = dwSize;
		/* Multiboot partition indices starting at 0, FreeLoader will increment 0xFE to 0xFF
		 * and later it will interpret drive 0xE0 and partition 0xFF as CD drive. */
		multiboot->uBootDevice = GetMultibootBootDevice(0xE0, 0xFE, 0xFF, 0xFF);
	}

	return true;
}

int ExittoReactOS(const OPTMULTIBOOT *multiboot) {
	PMULTIBOOTHEADER mbHeader;

	mbHeader = CheckMultibootHeader(multiboot->pBuffer);
	if (mbHeader == NULL)
		return;

	VIDEO_ATTR = 0xff8888a8;
	printk("     Multiboot header found at 0x%X\n", mbHeader);
	printk("     Boot device is 0x%X\n", multiboot->uBootDevice);
	VIDEO_ATTR = 0xffa8a8a8;

	char *sz = "\2Starting ReactOS\2";
	VIDEO_CURSOR_POSX = ((vmode.width - BootVideoGetStringTotalWidth(sz)) / 2) * 4;
	VIDEO_CURSOR_POSY = vmode.height - 64;

	VIDEO_ATTR = 0xff9f9fbf;
	printk(sz);
	setLED("rrrr");
	startReactOS(mbHeader, multiboot->uBufferSize, multiboot->uBootDevice);
}

void startReactOS(PMULTIBOOTHEADER mbHeader, u32 loaderSize, u32 bootDevice) {
	PMULTIBOOTINFO mbInfo;
	PMEMORYMAP mmap;
	static u32 ldrSize;
	static u32 bootDev;
	u32 infoPtr;
	int nAta = 0;

	if (mbHeader == NULL)
		return;

	/* Save needed values into static variables, as the stack pointer
	 * will be modified by inline assembly below */
	ldrSize = loaderSize;
	bootDev = bootDevice;

	/* Prepare to move freeldr to its final destination. Since that might
	 * involve overwriting cromwell structures we shutdown as much as
	 * possible. After this point, we can't return anymore */
	BootStopUSB();
	if (tsaHarddiskInfo[0].m_bCableConductors == 80) {
		if (tsaHarddiskInfo[0].m_wAtaRevisionSupported & 2) nAta = 1;
		if (tsaHarddiskInfo[0].m_wAtaRevisionSupported & 4) nAta = 2;
		if (tsaHarddiskInfo[0].m_wAtaRevisionSupported & 8) nAta = 3;
		if (tsaHarddiskInfo[0].m_wAtaRevisionSupported & 16) nAta = 4;
		if (tsaHarddiskInfo[0].m_wAtaRevisionSupported & 32) nAta = 5;
	} else {
		// force the HDD into a good mode 0x40 ==UDMA | 2 == UDMA2
		nAta = 2; // best transfer mode without 80-pin cable
	}
	// nAta=1;
	BootIdeSetTransferMode(0, 0x40 | nAta);
	BootIdeSetTransferMode(1, 0x40 | nAta);

	/* Set the LED to auto-mode */
	resetLED();

	/* Do not update framebuffer address, as ReactOS video drivers reuse it as is,
	 * and do not perform video hardware initialization. See also FIXME below. */
	//(*(unsigned int *)0xFD600800) = (0xf0000000 | ((xbox_ram * 0x100000) - FB_SIZE));

	/* disable interrupts */
	asm volatile ("cli\n");

	/* clear IDT area */
	memset((void *)IDT_LOC, 0x0, 8 * 1024);

	asm volatile (
	"wbinvd\n"

	/* Flush the TLB */
	"xor %eax, %eax \n"
	"mov %eax, %cr3 \n"

	/* Load IDT table (0xB0000 = IDT_LOC) */
	"lidt 0xB0000\n"

	/* DR6/DR7: Clear the debug registers */
	"xor %eax, %eax \n"
	"mov %eax, %dr6 \n"
	"mov %eax, %dr7 \n"
	"mov %eax, %dr0 \n"
	"mov %eax, %dr1 \n"
	"mov %eax, %dr2 \n"
	"mov %eax, %dr3 \n"

	/* Kill the LDT, if any */
	"xor  %eax, %eax \n"
	"lldt %ax \n"

	/* Reload CS as 0010 from the new GDT using a far jump */
	"ljmp	$0x0010, $reload_cs_exit \n"

	".align 16  \n"
	"reload_cs_exit: \n"

	/* CS is now a valid entry in the GDT.  Set the other segment registers
	 * to valid descriptors (4Gb flat mode) as required by the multiboot
	 * spec */

	"movw $0x0018, %ax \n"
	"mov %eax, %ss \n"
	"mov %eax, %ds \n"
	"mov %eax, %es \n"
	"mov %eax, %fs \n"
	"mov %eax, %gs \n"

	/* Set the stack pointer to give us a valid stack */
	"movl $0x03BFFFFC, %esp \n"
	);

	/* Ok, preparations are complete. Now move the image.
	 * Values of load_end_addr and bss_end_addr can be equal to zero, see:
	 * https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Header-address-fields */
	memcpy((u8 *)mbHeader->load_addr,
		(u8 *)mbHeader - (mbHeader->header_addr - mbHeader->load_addr),
		(mbHeader->load_end_addr > 0 ? mbHeader->load_end_addr - mbHeader->load_addr : ldrSize));
	if (mbHeader->bss_end_addr > 0)
		memset((u8 *)mbHeader->load_end_addr, 0x00, mbHeader->bss_end_addr - mbHeader->load_end_addr);

	/* Set up the multiboot info structure */
	infoPtr = mbHeader->bss_end_addr;
	if (infoPtr == 0)
		infoPtr = mbHeader->load_addr + ldrSize;
	mbInfo = (PMULTIBOOTINFO)((infoPtr + 3) & ~0x3);
	memset(mbInfo, 0x00, sizeof(MULTIBOOTINFO));
	mbInfo->flags = MB_INFO_FLAG_MEM_SIZE | MB_INFO_FLAG_BOOT_DEVICE | MB_INFO_FLAG_MEMORY_MAP;
	/* Multiboot spec says mem_lower can't be larger than 640, which is
	 * true for a PC. Since we're bending the multiboot rules anyway
	 * let's pass the full megabyte */
	mbInfo->mem_lower = 1024;
	mbInfo->mem_upper = (xbox_ram - 1) * 1024;
	mbInfo->boot_device = bootDev;
	mbInfo->mmap_length = 2 * sizeof(MEMORYMAP);
	mmap = (PMEMORYMAP)(mbInfo + 1);
	mbInfo->mmap_addr = (u32)mmap + sizeof(u32);
	/* Normal RAM */
	mmap->size = sizeof(MEMORYMAP);
	mmap->base_addr_low = 0;
	mmap->base_addr_high = 0;
	mmap->length_low = (xbox_ram * 1024 * 1024) - FB_SIZE;
	mmap->length_high = 0;
	mmap->type = 1;
	/* Video RAM */
	mmap++;
	mmap->size = sizeof(MEMORYMAP);
	/* FIXME: Framebuffer is initialized statically with FB_START in BootVgaInitialization.c
	 * Remove this condition once the problem is fixed */
	if (xbox_ram > 64) {
		mmap->base_addr_low = (64 * 1024 * 1024) - FB_SIZE;
	} else {
		mmap->base_addr_low = (xbox_ram * 1024 * 1024) - FB_SIZE;
	}
	mmap->base_addr_high = 0;
	mmap->length_low = FB_SIZE;
	mmap->length_high = 0;
	mmap->type = 0;
	if (xbox_ram > 64) {
		/* FIXME: Remove this block once framebuffer problem is fixed */
		mmap++;
		mmap->size = sizeof(MEMORYMAP);
		mmap->base_addr_low = 64 * 1024 * 1024;
		mmap->base_addr_high = 0;
		mmap->length_low = (xbox_ram - 64) * 1024 * 1024;
		mmap->length_high = 0;
		mmap->type = 1;
	}

	/* Now setup the registers and jump to the multiboot entry point */
	asm volatile (
	"xorl %%ecx,%%ecx \n"
	"xorl %%edx,%%edx \n"
	"xorl %%esi,%%esi \n"
	"xorl %%edi,%%edi \n"
	"pushl %%eax \n"
	"movl %0,%%eax \n"
	"ret \n"
	: : "i" (MULTIBOOT_BOOTLOADER_MAGIC), "b" (mbInfo), "a" (mbHeader->entry_addr));

	// We are not longer here, we are already in freeldr, we never come back here

	// See you again in ReactOS then
	while (1);
}
