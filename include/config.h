////////////////////// compile-time options ////////////////////////////////

//Cromwell version number
#define VERSION "2.33-dev"

// selects between the supported video modes, see boot.h for enum listing those available
//#define VIDEO_PREFERRED_MODE VIDEO_MODE_800x600
#define VIDEO_PREFERRED_MODE VIDEO_MODE_640x480

//Uncomment to include BIOS flashing support from CD
#define FLASH

//Uncomment to enable the 'advanced menu'
#define ADVANCED_MENU

//Time to wait in seconds before auto-selecting default item
#define BOOT_TIMEWAIT 15

//Uncomment to make connected Xpads rumble briefly at init.
#define XPAD_VIBRA_STARTUP


//Obsolete


// display a line like Composite 480 detected if uncommented
#undef REPORT_VIDEO_MODE

// show the MBR table if the MBR is valid
#undef DISPLAY_MBR_INFO

#undef DEBUG_MODE
// enable logging to serial port.  You probably don't have this.
#define INCLUDE_SERIAL 0
// enable trace message printing for debugging - with serial only
#define PRINT_TRACE 0
