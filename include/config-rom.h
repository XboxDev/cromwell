////////////////////// compile-time options ////////////////////////////////

// selects between the supported video modes, see boot.h for enum listing those available
#define VIDEO_PREFERRED_MODE VIDEO_MODE_640x480

// uncomment to force CD boot mode even if MBR present
// default is to boot from HDD if MBR present, else CD
//#define FORCE_CD_BOOT

// uncomment the following if you use cromwell
// as bootloader from CD
//#define IS_XBE_BOOTLOADER

// display a line like Composite 480 detected if uncommented
#define REPORT_VIDEO_MODE

// show the MBR table if the MBR is valid
#define DISPLAY_MBR_INFO

// show simple menu selection
#define MENU

// uncomment to do Ethernet init
//#define DO_ETHERNET 1

//#define FLASH

#undef DO_USB
//#define DO_USB
