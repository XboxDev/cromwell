OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);

MEMORY
{
	RAM (RW) : ORIGIN = 0, LENGTH = 64M
	ROM (RX) : ORIGIN = 0xfffc0000, LENGTH = 256K /* this should be changed depending on the actual size of the ROM */
}

RAM_CODE = 0x00100000;

ROM_BASE = ORIGIN(ROM);
ROM_SIZE = LENGTH(ROM);
ROM_END  = ROM_BASE + ROM_SIZE;

RESET_VECTOR_BLOCK_SIZE = 4K;
RESET_VECTOR_BLOCK_BASE = ROM_END - RESET_VECTOR_BLOCK_SIZE;

SECTIONS
{
	/* ROM allocations */

	.low_rom ROM_BASE : AT ( 0 ) {
		*(.low_rom);
		_end_rom = . ;
	}

	.reset_vector (RESET_VECTOR_BLOCK_BASE) : AT (RESET_VECTOR_BLOCK_BASE - ROM_BASE) {
		_reset_vector_base = .;
		*(.reset_vector_base)
		*(.reset_vector.*)
		. = ALIGN(RESET_VECTOR_BLOCK_SIZE) - 16;
		*(.reset_vector)
	} >ROM = 0x00

	/* RAM allocations */

	.text (RAM_CODE) : AT( SIZEOF(.low_rom) ) {
		_ram_location = .;
		_start_ramcopy = _end_rom;
		*(.text);
		*(.text.*)
		*(.eh_frame)
		_start_checksum = _start_ramcopy - ROM_BASE;
	}

	.data (RAM_CODE + SIZEOF(.text)) : AT( SIZEOF(.low_rom) + SIZEOF(.text)) {
		*(.data);
		*(.data.*);
		*(.sdata);
		*(.sdata2);
		*(.got);
		*(.got.*);
	}

	.rodata ( RAM_CODE + SIZEOF(.text) + SIZEOF(.data) ) : AT ( SIZEOF(.low_rom) + SIZEOF(.text) + SIZEOF(.data)) {
		*(.rodata);
		*(.rodata.str1.1);
		*(.rodata.str1.4);
		*(.rodata.str1.32);
		*(.rodata.cst4);
		*(.rodata.cst8);
	}

	.bss ( RAM_CODE + SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata) ) : AT ( SIZEOF(.low_rom) + SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata)) {
		BSS_BASE = .;
		_bss = .;
		*(.bss)
		*(.sbss)
		*(COMMON)
		_ebss = . ;
		_heap = . ;
		BSS_END = .;
	}

	/* We need to copy the .data section to upper memory */

	_size_ramcopy = SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata) + SIZEOF(.bss);
	_size_sha1hash = SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.rodata);

	/DISCARD/ : {
		*(.note.gnu.property)
		*(.comment)
		*(.note.GNU-stack)
		*(.eh_frame)
		*(.rela.eh_frame)
		*(.shstrtab)
		*(.symtab)
		*(.strtab)
		*(.got.plt)
	}
}

BSS_SIZE = BSS_END - BSS_BASE;
BSS_SIZE_L = BSS_SIZE / 4;

