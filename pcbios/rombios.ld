/*
 * loader script for rombios
 * Rombios is loaded to memory address 0x000f0000, so we link it for exactly this address
 */

OUTPUT_FORMAT ("elf32-i386");
OUTPUT_ARCH(i386);
MEMORY
{
	ram (rwx) : ORIGIN = 0, LENGTH = 64M
}


LOW_ROM = 0xf0000; 

SECTIONS {

		/* ROM allocations */

	.rombios_start LOW_ROM : AT ( 0 ){
		
		*(.rombios_start);
		
	}

	.rodata (LOW_ROM + SIZEOF(.rombios_start)) : AT (SIZEOF(.rombios_start)) {
		*(.rodata);
		*(.rodata.str1.1);
		*(.rodata.str1.32);
		*(.rodata.cst4);
		*(.rodata.cst8);

	}
        
	.data (LOW_ROM + SIZEOF(.rombios_start) + SIZEOF(.rodata)) : AT( SIZEOF(.rombios_start) + SIZEOF(.rodata) ) {
		_start_data = .;
		*(.data);
		*(.sdata);
		*(.sdata2);
		*(.got);
		_end_data = .;
	}

	.bss (LOW_ROM + SIZEOF(.rombios_start) + SIZEOF(.rodata)+SIZEOF(.data)) : AT (SIZEOF(.rombios_start) + SIZEOF(.rodata)+SIZEOF(.data)){
		_bss = .;
		*(.bss)
		*(.sbss)
		*(COMMON)
		_ebss = . ;
		_heap = . ;
	}

}
