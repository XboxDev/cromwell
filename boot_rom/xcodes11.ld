/**************************************************************************
 *
 *  "BIOS image as binary resource" ld script -
 *      (C) Milosch Meriac <xboxlinux@meriac.de> 2002
 *
 *  $log:$
 *
 **************************************************************************/

SECTIONS {
    .xcodes11 (0xfffc0080) : AT (0x80) {

	       xcodes11 = .;
        *(.data);
    }
}
