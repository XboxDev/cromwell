#ifndef memory_layout_h
#define memory_layout_h

/* a retail Xbox has 64 MB of RAM */
#define RAMSIZE (64 * 1024*1024)
/* parameters for the kernel have to be here */
#define KERNEL_SETUP   0x90000
/* command line must not be overwritten, place it in unused setup area */
#define CMD_LINE_LOC (KERNEL_SETUP+0x0800)
/* place GDT at 0xA0000 */
#define GDT_LOC 0xA0000
/* place IDT at 0xB0000 */
#define IDT_LOC 0xB0000
/* the protected mode part of the kernel has to reside at 1 MB in RAM */
#define PM_KERNEL_DEST 0x100000

/* let's reserve 4 MB at the top for the framebuffer */
#define RAMSIZE_USE (RAMSIZE - FRAMEBUFFER_SIZE)
/* the initrd resides at 1 MB from the top of RAM */
#define INITRD_DEST (RAMSIZE_USE - 1024*1024)

#define MEMORYMANAGERSTART 	0x01000000
#define MEMORYMANAGERSIZE 	0x1000000 // 16 MB
#define MEMORYMANAGEREND 	0x01FFFFFF

#define INITRD_POS 0x02000000

#define STACK_TOP 0x03C00000

/* the size of the framebuffer (defaults to 4 MB) */
#define FRAMEBUFFER_SIZE 0x00400000
/* the start of the framebuffer */
#define FRAMEBUFFER_START (0xf0000000 | (RAMSIZE - FRAMEBUFFER_SIZE))

//#define LPCFlashadress 0xFFF00000
#define LPCFlashadress 0xFF000000

#endif /* #ifndef memory_layout_h */
