/* you can change this */
#undef DEBUG

/* but you better shouldn't change this */
#define CONFIG_FILE "linuxboot.cfg"

#define BUFFERSIZE 256 /* we have little stack */
#define CONFIG_BUFFERSIZE (BUFFERSIZE*16)

#ifdef DEBUG
#define dprintf printf
#else
#define dprintf
#endif

#ifdef DEBUG
#define splash
#define splash_init()
#else
#define splash show_splash
#define splash_init do_splash_init
#endif

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT_576 576
#define SCREEN_HEIGHT_480 480

/* memory layout */

/* a retail Xbox has 64 MB of RAM */
#define RAMSIZE (64 * 1024*1024)

#define SCREEN_SIZE_480 (SCREEN_HEIGHT_480 * SCREEN_WIDTH * 4)
#define SCREEN_SIZE_576 (SCREEN_HEIGHT_576 * SCREEN_WIDTH * 4)
#define FRAMEBUFFER_SIZE_480 ((SCREEN_SIZE_480+0xFFFF)& 0xFFFF0000)
#define FRAMEBUFFER_SIZE_576 ((SCREEN_SIZE_576+0xFFFF)& 0xFFFF0000)
#define NEW_FRAMEBUFFER_480 (RAMSIZE - FRAMEBUFFER_SIZE_480)
#define NEW_FRAMEBUFFER_576 (RAMSIZE - FRAMEBUFFER_SIZE_576)
#define RAMSIZE_USE_480 (NEW_FRAMEBUFFER_480)
#define RAMSIZE_USE_576 (NEW_FRAMEBUFFER_576)

#define MAX_KERNEL_SIZE (2*1024*1024)
#define MAX_INITRD_SIZE (6*1024*1024)

/* position of kernel setup data */
#define SETUP 0x90000
/* the GDT must not be overwritten, so we place it at the ISA VGA memory location */
#define GDT 0xA0000
/* position of protected mode kernel */
#define PM_KERNEL_DEST 0x100000

/* Lowest allowable address of the kernel (at or above 1 meg) */
#define MIN_KERNEL PM_KERNEL_DEST
/* Highest allowable address */
#define MAX_KERNEL_480 (RAMSIZE_USE_480-1)
#define MAX_KERNEL_576 (RAMSIZE_USE_576-1)

/* i386 constants */

/* CR0 bit to enable paging */
#define CR0_ENABLE_PAGING		0x80000000
/* Size of a page on x86 */
#define PAGE_SIZE			4096

/* Size of the read chunks to use when reading the kernel; bigger = a lot faster */
#define READ_CHUNK_SIZE 128*1024

