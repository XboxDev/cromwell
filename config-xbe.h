////////////////////// compile-time options ////////////////////////////////

// selects between the supported video modes, see boot.h for enum listing those available
#define VIDEO_PREFERRED_MODE VIDEO_MODE_800x600

//
// uncomment to force CD boot mode even if MBR present
// default is to boot from HDD if MBR present, else CD
//#define FORCE_CD_BOOT
//#define IS_XBE_CDLOADER
#define MENU
#define ENABLE_FATX

// Usefull combinations
//
// Booting from CD
// 
// #define FORCE_CD_BOOT
// #define IS_XBE_CDLOADER
// #undef MENU
// #undef ENABLE_FATX
//
// Use xromwell as a normal bootselector from
// HDD
//
// #undef FORCE_CD_BOOT
// #undef IS_XBE_CDLOADER
// #define MENU
// #define ENABLE_FATX

// display a line like Composite 480 detected if uncommented
#define REPORT_VIDEO_MODE

// show the MBR table if the MBR is valid
#define DISPLAY_MBR_INFO

// uncomment to do Ethernet init
//#define DO_ETHERNET 1
