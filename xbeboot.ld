/*
 * $Id$
 * loader script
 *
 * adapted from the Free Bios project
 * and work by Johan Rydberg and Daniel Kahlin.
 *
 * $Log$
 * Revision 1.3  2002/09/18 19:49:35  mist
 * gcc 3.2 (Mdk 9) fixes
 *
 * Revision 1.2  2002/08/26 22:46:33  mist
 * configuration file support, README
 *
 * Revision 1.1  2002/08/21 17:35:46  meriac
 * Michael added generic boot screen
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
	rom (rx)  : ORIGIN = 0x10000, LENGTH = 16M
}

//RAM_BASE = 0x10000; /* overwrite XBE header with variables */

HEADER = 0x10000;
HEADER_LOAD = 0;

SECTIONS {


		/* ROM allocations */
	.text ( HEADER ) : AT ( HEADER_LOAD ) {
		_start_text = . ;
		*(.text);
		*(.rodata);
		_end_text = . ;
	}
		/* ram allocations */

	.data ( HEADER + SIZEOF(.text) ) : AT ( HEADER_LOAD + SIZEOF(.text) ) {
		_start_data = .;
		*(.data);
		*(.sdata);
		*(.sdata2);
		*(.got);
		*(.*);
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
}
