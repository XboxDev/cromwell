#include "boot.h"
#include "xbox.h"


/* parameters to be passed to the kernel */
struct kernel_setup_t {
	unsigned char  orig_x;                  /* 0x00 */
	unsigned char  orig_y;                  /* 0x01 */
	unsigned short ext_mem_k;   /*   2 */
	unsigned short orig_video_page;         /* 0x04 */
	unsigned char  orig_video_mode;         /* 0x06 */
	unsigned char  orig_video_cols;         /* 0x07 */
	unsigned short unused2;                 /* 0x08 */
	unsigned short orig_video_ega_bx;       /* 0x0a */
	unsigned short unused3;                 /* 0x0c */
	unsigned char  orig_video_lines;        /* 0x0e */
	unsigned char  orig_video_isVGA;        /* 0x0f */
	unsigned short orig_video_points;       /* 0x10 */

	/* VESA graphic mode -- linear frame buffer */
	unsigned short lfb_width;               /* 0x12 */
	unsigned short lfb_height;              /* 0x14 */
	unsigned short lfb_depth;               /* 0x16 */
	unsigned long  lfb_base;                /* 0x18 */
	unsigned long  lfb_size;                /* 0x1c */
	unsigned short cmd_magic;	/*  32: Command line magic 0xA33F */
	unsigned short cmd_offset;	/*  34: Command line offset from 0x90000 */
	unsigned short lfb_linelength;          /* 0x24 */
	unsigned char  red_size;                /* 0x26 */
	unsigned char  red_pos;                 /* 0x27 */
	unsigned char  green_size;              /* 0x28 */
	unsigned char  green_pos;               /* 0x29 */
	unsigned char  blue_size;               /* 0x2a */
	unsigned char  blue_pos;                /* 0x2b */
	unsigned char  rsvd_size;               /* 0x2c */
	unsigned char  rsvd_pos;                /* 0x2d */
	unsigned short vesapm_seg;              /* 0x2e */
	unsigned short vesapm_off;              /* 0x30 */
	unsigned short pages;                   /* 0x32 */
	char __pad2[445];
	unsigned char  setup_sects; /* 497: 0x1f1  setup size in sectors (512) */
	unsigned short root_flags;	/* 498: 0x1f2  1 = ro ; 0 = rw */
	unsigned short kernel_para;	/* 500: 0x1f4 kernel size in paragraphs (16) syssize bootsect.S */
	unsigned short swap_dev;	/* 502: 0x1f6  OBSOLETED */
	unsigned short ram_size;	/* 504: 0x1f8 bootsect.S*/
	unsigned short vid_mode;	/* 506: 0x1fa */
	unsigned short root_dev;	/* 508: 0x1fc 0x301 */
	unsigned short boot_flag;	/* 510: 0x1fe signature 0xaa55 */
	unsigned short jump;        /* 512: 0x200 jump to startup code */
	char signature[4];          /* 514: 0x202 "HdrS" */
	unsigned short version;     /* 518: 0x206 header version 0x203*/
	unsigned long pHookRealmodeSwitch; /* 520 0x208 hook */
	unsigned short start_sys;           /* 524: 0x20c start_sys */
	unsigned short ver_offset;  /* 526: 0x20e kernel version string */
	unsigned char loader;       /* 528: 0x210 loader type */
	unsigned char flags;        /* 529: 0x211 loader flags */
	unsigned short a;           /* 530: 0x212 setup move size more LOADLIN hacks */
	unsigned long start;        /* 532: 0x214 kernel start, filled in by loader */
	unsigned long ramdisk;      /* 536: 0x218 RAM disk start address */
	unsigned long ramdisk_size; /* 540: 0x21c RAM disk size */
	unsigned short b,c;         /* 544: 0x220 bzImage hacks */
	unsigned short heap_end_ptr;/* 548: 0x224 end of free area after setup code */
	unsigned char __pad3[2];  // was wrongfully [4]
	unsigned int cmd_line_ptr;  /* 552: pointer to command line */
	unsigned int initrd_addr_max;/*556: highest address that can be used by initrd */
};

extern void* framebuffer;

void setup(void* KernelPos, void* PhysInitrdPos, void* InitrdSize, char* kernel_cmdline) {
    int cmd_line_ptr;
    struct kernel_setup_t *kernel_setup = (struct kernel_setup_t*)KernelPos;
//		int resolution=480;

    /* init kernel parameters */
    kernel_setup->loader = 0xff;		/* must be != 0 */
    kernel_setup->heap_end_ptr = 0xffff;	/* 64K heap */
    kernel_setup->flags = 0x81;			/* loaded high, heap existant */
    kernel_setup->start = PM_KERNEL_DEST;
//    if (resolution==480)
		kernel_setup->ext_mem_k = (60 * 1024)-1024; // RAMSIZE_USE_480/1024-1024; /* *extended* (minus first MB) memory in kilobytes */
//	else
//		kernel_setup->ext_mem_k = RAMSIZE_USE_576/1024-1024; /* *extended* (minus first MB) memory in kilobytes */

    /* initrd */
    /* ED : only if initrd */

    if((long)InitrdSize != 0) {
	    kernel_setup->ramdisk = (long)PhysInitrdPos;
	    kernel_setup->ramdisk_size = (long)InitrdSize;
//	    if (resolution==480)
	    	kernel_setup->initrd_addr_max =  (60 * 1024 * 1024)-1; // RAMSIZE_USE_480;
//		else
//	    	kernel_setup->initrd_addr_max = RAMSIZE_USE_576;
    }

    /* Framebuffer setup */
    kernel_setup->orig_video_isVGA = 0x23;
    kernel_setup->orig_x = 0;
    kernel_setup->orig_y = 0;
    kernel_setup->vid_mode = 0x312;		/* 640x480x16M Colors */
    kernel_setup->orig_video_mode = kernel_setup->vid_mode-0x300;
    kernel_setup->orig_video_cols = 80;
    kernel_setup->orig_video_lines = 30;
    kernel_setup->orig_video_ega_bx = 0;
    kernel_setup->orig_video_points = 16;
    kernel_setup->lfb_depth = 32;
    kernel_setup->lfb_width = SCREEN_WIDTH;
//    if (resolution==480) {
	    kernel_setup->lfb_height = VIDEO_HEIGHT; // SCREEN_HEIGHT_480;
			memcpy((void *)(60 * 1024 * 1024), (void *)(*(unsigned int*)0xFD600800), 640*480*4);
			(*(unsigned int*)0xFD600800)= (60 * 1024 * 1024);
	    kernel_setup->lfb_base = (0xf0000000+*(unsigned int*)0xFD600800); // 0xF0000000+NEW_FRAMEBUFFER_480;
//			printk("kernel_setup->lfb_base=0x%08X\n", kernel_setup->lfb_base);
			kernel_setup->lfb_size = (4 * 1024 * 1024)/0x10000; // (FRAMEBUFFER_SIZE_480+0xFFFF)/0x10000;
//	} else {
 //   	kernel_setup->lfb_height = SCREEN_HEIGHT_576;
  //  	kernel_setup->lfb_base = 0xF0000000+NEW_FRAMEBUFFER_576;
//	    kernel_setup->lfb_size = (FRAMEBUFFER_SIZE_576+0xFFFF)/0x10000;
//	}
    kernel_setup->lfb_linelength = SCREEN_WIDTH*4;
    kernel_setup->pages=1;
    kernel_setup->vesapm_seg = 0;
    kernel_setup->vesapm_off = 0;
    kernel_setup->blue_size = 8;
    kernel_setup->blue_pos = 0;
    kernel_setup->green_size = 8;
    kernel_setup->green_pos = 8;
    kernel_setup->red_size = 8;
    kernel_setup->red_pos = 16;
    kernel_setup->rsvd_size = 8;
    kernel_setup->rsvd_pos = 24;

		kernel_setup->root_dev=0x0301; // 0x0301..?? /dev/hda1 default if no comline override given
		kernel_setup->pHookRealmodeSwitch=0;
		kernel_setup->start_sys=0;
		kernel_setup->a=0;
		kernel_setup->b=0;
		kernel_setup->c=0;

		kernel_setup->root_flags=0; // allow read/write

    /* set command line */
    cmd_line_ptr = (kernel_setup->setup_sects) * 512; /* = 512 bytes from top of SETUP */
    kernel_setup->cmd_offset = (unsigned short) cmd_line_ptr;
    kernel_setup->cmd_magic = 0xA33F;
    kernel_setup->cmd_line_ptr = (DWORD)((BYTE *)kernel_setup)+cmd_line_ptr;
    memcpy((char*)(KernelPos+cmd_line_ptr), kernel_cmdline, 512);
    *(char*)(KernelPos+cmd_line_ptr+511) = 0;
}
