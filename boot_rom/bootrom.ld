/*
 *
 */

OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);
MEMORY
{
	ram (rwx) : ORIGIN = 0, LENGTH = 64M
	rom (rx)  : ORIGIN = 0xFFFC0000, LENGTH = 256K
}


RAM_CODE = 0x00100000; 
RAM_BASE = 0x01000000;  

/* this should be changed depending the actual size of the rom */
ROM_SIZE = 256K;
LOW_ROM = 0xfffc0000; 

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

	.rom ( 0xfffc1000 ) : AT ( 0x1000 )  { 
		_start_rom = . ;
		*(rom);
                _end_rom = . ;
	}


	/* ram allocations */

	.data (RAM_BASE) : AT( 0x1000 + SIZEOF(.rom) ) {
		_start_data = .;
		*(.data);
		*(.sdata);
		*(.sdata2);
		*(.got);
		_end_data = .;
	}

	.text (RAM_CODE) : AT( 0x1000 + SIZEOF(.rom)+ SIZEOF(.data)) {
		_ram_location = .;
		_start_ramcopy = 0x1000 + SIZEOF(.rom)+ SIZEOF(.data) - RAM_CODE + 0xFFFC0000;
		*(.text);
		_end_ramcopy = .;
		_size_ramcopy = . - RAM_CODE;
		_start_checksum = _start_ramcopy - LOW_ROM;
	}


	/* the data (initialized globals) is moved to ram by the startup code */

	.bss ( ADDR(.data) + SIZEOF(.data) ) : {
		_bss = .;
		*(.bss)
		*(.sbss)
		*(COMMON)
		_ebss = . ;
		_heap = . ;
	}


	.high_rom TOP_ROM : AT (TOP_ROM_LOAD) {
		_start_top_rom = . ;
		. = . + ( 512 - sizeof_top );
		*(.high_rom);
		_end_top_rom = . ;
	} = 0x90909090

}
