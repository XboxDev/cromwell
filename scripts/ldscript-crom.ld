OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);

MEMORY
{
	RAM (RW) : ORIGIN = 0, LENGTH = 64M
	ROM (RX) : ORIGIN = 0x03a00000 /* 58MiB */, LENGTH = 256K /* this should be changed depending on the actual size of the ROM */
}

ROM_BASE = ORIGIN(ROM);
ROM_SIZE = LENGTH(ROM);
ROM_END  = ROM_BASE + ROM_SIZE;

SECTIONS
{
	/* ROM allocations */

	.text ROM_BASE : AT ( 0 ) {
		*(.text);
		*(.text.*)
		*(.eh_frame)
	}

	.rodata (ROM_BASE + SIZEOF(.text)) : AT (SIZEOF(.text)) {
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

	.data (ROM_BASE + SIZEOF(.text) + SIZEOF(.rodata)) : AT( SIZEOF(.text) + SIZEOF(.rodata) ) {
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

	.bss (ROM_BASE + SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data)) : AT( SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data)) {
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

