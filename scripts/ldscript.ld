/*
 * $Id$
 * loader script
 *
 * adapted from the Free Bios project
 * and work by Johan Rydberg and Daniel Kahlin.
 *
 * $Log$
 * Revision 1.2  2003/04/17 09:04:45  mist
 * Cramwell :-) The ROM version now copies all code into RAM.
 * Unfortunately booting Linux does not work any more at the moment (memory
 * allocation issue?)
 *
 * Revision 1.1.1.1  2003/04/15 10:52:16  mist
 * SF version 14 Apr 2003
 *
 * Revision 1.1  2003/03/18 18:08:48  huceke
 * *** empty log message ***
 *
 * Revision 1.6  2003/01/14 15:24:33  warmcat
 * New menu UI.  VSYNC IRQ up.  Franz's I2C check added.  Flashtype and Ed's EEPROM report added.
 *
 * Revision 1.5  2003/01/13 12:29:07  warmcat
 * Left off BootInterrupts.c from last commit :-)
 *
 * Revision 1.4  2002/12/18 10:38:25  warmcat
 * ISO9660 support added allowing CD boot; linuxboot.cfg support; some extra compiletime options and CD tray management stuff
 *
 * Revision 1.3  2002/12/15 21:40:52  warmcat
 *
 * First major release.  Working video; Native Reiserfs boot into Mdk9; boot instability?
 *
 * Revision 1.2  2002/09/19 10:39:19  warmcat
 * Merged Michael's Bochs stuff with current cromwell
 * note requires the cpp0-2.95 preprocessor
 *
 * Revision 1.5  2002/08/19 13:48:55  meriac
 * need of nasm during linkage removed
 *
 * Revision 1.3  2002/08/15 20:01:38  mist
 * kernel and initrd needn't be patched into image.bin manually any more,
 * due to Milosch
 *
 * Revision 1.2  2002/08/12 12:00:52  mist
 * compiler issues fixed by Andy
 *
 *
 *
 */

OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);
MEMORY
{
	ram (rwx) : ORIGIN = 0, LENGTH = 64M
/*	rom (rx)  : ORIGIN = 0xFF000000, LENGTH = 16M */
	rom (rx)  : ORIGIN = 0xFFFC0000, LENGTH = 256K
}

//RAM_CODE = 0x3F00000;
RAM_CODE = 0x3B00000; /* 59 MB */
RAM_BASE = 0x300000;  /* was 0x400000 */

/* this should be changed depending the actual size of the rom */
ROM_SIZE = 256K;/* 1M; */
LOW_ROM = 0xfffc0000; /* 0xfff00000; */

LOW_ROM_LOAD = 0;
TOP_ROM = ( LOW_ROM + ROM_SIZE - 512 );
TOP_ROM_LOAD = ( LOW_ROM_LOAD + ROM_SIZE - 512 );

SECTIONS {



		/* ROM allocations */

		.low_rom LOW_ROM : AT ( LOW_ROM_LOAD ){
		_start_low_rom = . ;
		*(.low_rom);
		_end_low_rom = . ;
	}

	.xcodes11 (0xfffc0080) : AT (0x80) {
		_start_xcodes11 = . ;
		*(.xcodes11);
		_end_xcodes11 = . ;
	}

	.rom ( 0xfffc0c80 ) : AT ( 0xc80 )  { /* was 0xfffc0c80  0xc80 */
		_start_rom = . ;
		*(rom);
                _end_rom = . ;
	}
/*
	.amended2bl (0xffffd400) : AT (ROM_SIZE - 0x2c00) {
		_start_amended2bl = . ;
		*(.amended2bl);
		_end_amended2bl = . ;
	}
*/

	.rodata (0xfffc0c80 + SIZEOF(.rom)) : AT ( 0xc80 + SIZEOF(.rom)) {
			*(.rodata);
			*(.rodata.str1.1);
			*(.rodata.str1.32);
			*(.rodata.cst4);
	_start_backdrop = .;
			*(.backdrop);
	_end_backdrop = .;
		_end_rodata = . ;
 }


	/* ram allocations */

	.data (RAM_BASE) : AT( 0xc80 + SIZEOF(.rom)+ SIZEOF(.rodata) ) {
			_start_data = .;
				*(.data);
		*(.sdata);
		*(.sdata2);
		*(.got);
		_end_data = .;
	}

	RamCopy (RAM_CODE) : AT( 0xc80 + SIZEOF(.rom)+ SIZEOF(.rodata) +SIZEOF(.data)) {
		_ram_location = .;
		_start_ramcopy = 0xc80 + SIZEOF(.rom)+ SIZEOF(.rodata) +SIZEOF(.data) - RAM_CODE + 0xFFFC0000;
                                *(.text);
				*(RamCopy);
		_end_ramcopy = .;
		_size_ramcopy = . - RAM_CODE;
	}


	/* the data (initialized globals) is moved to ram by the startup code */

	_start_load_data = LOADADDR(.data);
	_end_load_data = LOADADDR(.data) + SIZEOF(.data);

	.bss ( ADDR(.data) + SIZEOF(.data) ) : {
		_bss = .;
		*(.bss)
		*(.sbss)
		*(COMMON)
		_ebss = . ;
		_heap = . ;
	}



	.rodata-rombios (0xffffe000 - 0x10004) : AT ((0x3e000) - 0x10004) {
			*(.rodata-rombios);
	}

	.justbehind (0xfffffc80) : AT (ROM_SIZE - 0x380) {
		_start_justbehind = . ;
		*(.justbehind);
		_end_justbehind = . ;
	}

	.high_rom TOP_ROM : AT (TOP_ROM_LOAD) {
		_start_top_rom = . ;
		. = . + ( 512 - sizeof_top );
		*(.high_rom);
		_end_top_rom = . ;
	} = 0x90909090

}
