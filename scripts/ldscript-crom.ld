/*
 * loader script
 */

OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);
MEMORY
{
	ram (rwx) : ORIGIN = 0, LENGTH = 64M
	rom (rx)  : ORIGIN = 0x03A00000, LENGTH = 2M
}

/* this should be changed depending the actual size of the rom */
ROM_SIZE = 256K;
LOW_ROM = 0x03A00000; /* 58MB */

SECTIONS {
	/DISCARD/ : { *(.note.gnu.property) }

	/* ROM allocations */

	.text LOW_ROM : AT ( 0 ){
		_start_low_rom = . ;
		*(.text);
		*(.text.*)
		*(.eh_frame)
		_end_low_rom = . ;
	}

	.rodata (LOW_ROM + SIZEOF(.text)) : AT (SIZEOF(.text)) {
		*(.rodata);
		*(.rodata.str1.1);
		*(.rodata.str1.32);
		*(.rodata.str1.4);
		*(.rodata.cst4);
		*(.rodata.cst8);
		_start_backdrop = .;
		*(.backdrop);
		_end_backdrop = .;
		_start_pcrombios = .;
		*(.pcrombios);
		_end_pcrombios = .;
		_end_rodata = . ;
	}

	/* RAM allocations */

	.data (LOW_ROM + SIZEOF(.text) + SIZEOF(.rodata)) : AT( SIZEOF(.text) + SIZEOF(.rodata) ) {
		_start_data = .;
		*(.data);
		*(.data.*);
		*(.sdata);
		*(.sdata2);
		*(.got);
		*(.got.*);
		_end_data = .;
	}

	/* the data (initialized globals) is moved to ram by the startup code */

	.bss (LOW_ROM + SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data)) : AT( SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data)) {
		BSS_BASE = .;
		_bss = .;
		*(.bss)
		*(.sbss)
		*(COMMON)
		_ebss = . ;
		_heap = . ;
		BSS_END = .;
	}

	_end_complete_rom = SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data) + SIZEOF(.bss);
}

BSS_SIZE = BSS_END - BSS_BASE;
BSS_SIZE_L = BSS_SIZE / 4;

