/**************************************************************************
 *
 *  "BIOS image as binary resource" ld script -
 *      (C) Milosch Meriac <xboxlinux@meriac.de> 2002
 *
 *  $log:$
 *
 **************************************************************************/

SECTIONS {
    .rodata : {

        rombios_size = .;
        LONG( rombios_end - rombios );

        rombios = .;
        *(.data);
        rombios_end = .;

    }
}
