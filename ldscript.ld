/*
 * $Id$
 * loader script
 *
 * adapted from the Free Bios project
 * and work by Johan Rydberg and Daniel Kahlin.
 *
 * $Log$
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
	rom (rx)  : ORIGIN = 0xFF000000, LENGTH = 16M
}

RAM_BASE = 0x400000;

/* this should be changed depending the actual size of the rom */
ROM_SIZE = 1M;

LOW_ROM = 0xfff00000;
LOW_ROM_LOAD = 0;
TOP_ROM = ( LOW_ROM + ROM_SIZE - 512 );
TOP_ROM_LOAD = ( LOW_ROM_LOAD + ROM_SIZE - 512 );

SECTIONS {

		/* ram allocations */

	.data RAM_BASE : {
			_start_data = .;
				*(.data);
		*(.sdata);
		*(.sdata2);
		*(.got);
		_end_data = .;
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

		/* ROM allocations */

		.low_rom LOW_ROM : AT ( LOW_ROM_LOAD ){
		_start_low_rom = . ;
		*(.low_rom);
		_end_low_rom = . ;
	}

	.text ( LOW_ROM + SIZEOF(.low_rom) ) : AT ( LOW_ROM_LOAD + SIZEOF(.low_rom)  ) {
		_start_text = . ;
		*(.text);
		_end_text = . ;
	}

	.rodata (ADDR(.text) + SIZEOF(.text)) : AT ( LOW_ROM_LOAD + SIZEOF(.low_rom) + SIZEOF(.text)) {
			*(.rodata);
			*(.comment);
			*(.note);
			*(.rodata.str1.1);
			*(.rodata.str1.32);
			*(.rodata.cst4);
	}

	.high_rom TOP_ROM : AT (TOP_ROM_LOAD) {
		_start_top_rom = . ;
		. = . + ( 512 - sizeof_top );
		*(.high_rom);
		_end_top_rom = . ;
	} = 0x90909090



}
