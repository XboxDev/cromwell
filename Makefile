#
# $Id$
#
# Shamelessly lifted and hacked from the
# free bios project.
#
# $Log$
# Revision 1.6  2002/12/24 03:53:33  huceke
# Added loading the cromwell as an xbe
#
# Revision 1.5  2002/12/18 10:38:25  warmcat
# ISO9660 support added allowing CD boot; linuxboot.cfg support; some extra compiletime options and CD tray management stuff
#
# Revision 1.4  2002/12/15 21:40:52  warmcat
#
# First major release.  Working video; Native Reiserfs boot into Mdk9; boot instability?
#
# Revision 1.3  2002/09/19 10:39:19  warmcat
# Merged Michael's Bochs stuff with current cromwell
# note requires the cpp0-2.95 preprocessor
#
#
# 2002-09-14 andy@warmcat.com  changed to -Wall and -Werror, superclean
# 2002-09-11 andy@warmcat.com  branched for cromwell, removed all the linux boot stuff
#
# Revision 1.4  2002/08/19 13:53:33  meriac
# the need of nasm during linking removed
#
# Revision 1.2  2002/08/15 20:01:38  mist
# kernel and initrd needn't be patched into image.bin manually any more,
# due to Milosch
#
#

### compilers and options
CC	= gcc
#CFLAGS	= -g -O2 -Wall -Werror
CFLAGS	= -g -Wall -Werror -DFSYS_REISERFS -Igrub -DSTAGE1_5 -DNO_DECOMPRESSION -Ijpeg-6b -DNO_GETENV
LD	= ld
LDFLAGS	= -s -S -T ldscript.ld
LDFLAGS-XBE	= -s -S -T ldscript-xbe.ld
LDFLAGS-BOOT	= -s -S -T xbeboot.ld
OBJCOPY	= objcopy
# The BIOS make process requires the gcc 2.95 preprocessor, not 2.96 and ot 3.x
# if you have gcc 2.95, you can use the following line:
# GCC295 = ${GCC}
# if you don't, install at least the file cpp0 taken from a gcc 2.95 setup,
# and use a line like this one:
GCC295 = cpp0-2.95
# note that SuSE 8 cannot compile the BIOS, although the GCC version would
# be correct.
BCC = /usr/lib/bcc/bcc-cc1

### objects
OBJECTS-XBE = xbeboot.o
OBJECTS	= BootStartup.o BootResetAction.o BootPerformPicChallengeResponseAction.o  \
BootPciPeripheralInitialization.o BootVgaInitialization.o BootIde.o \
BootHddKey.o rc4.o sha1.o BootVideoHelpers.o vsprintf.o filtror.o BootStartBios.o setup.o BootFilesystemIso9660.o \
grub/fsys_reiserfs.o grub/char_io.o grub/disk_io.o \
jpeg-6b/jdapimin.o jpeg-6b/jdapistd.o jpeg-6b/jdtrans.o jpeg-6b/jdatasrc.o jpeg-6b/jdmaster.o \
jpeg-6b/jdinput.o jpeg-6b/jdmarker.o jpeg-6b/jdhuff.o jpeg-6b/jdphuff.o jpeg-6b/jdmainct.o jpeg-6b/jdcoefct.o \
jpeg-6b/jdpostct.o jpeg-6b/jddctmgr.o jpeg-6b/jidctfst.o jpeg-6b/jidctflt.o jpeg-6b/jidctint.o jpeg-6b/jidctred.o \
jpeg-6b/jdsample.o jpeg-6b/jdcolor.o jpeg-6b/jquant1.o jpeg-6b/jquant2.o jpeg-6b/jdmerge.o jpeg-6b/jmemnobs.o \
jpeg-6b/jmemmgr.o jpeg-6b/jcomapi.o jpeg-6b/jutils.o jpeg-6b/jerror.o

#RESOURCES = rombios.elf amended2bl.elf xcodes11.elf
#RESOURCES = rombios.elf xcodes11.elf
#RESOURCES = rombios.elf
RESOURCES = xcodes11.elf backdrop.elf

# target:
all	: image.bin image-xbe.bin default.xbe

default.elf : ${OBJECTS-XBE}
	${LD} -o $@ ${OBJECTS-XBE} ${LDFLAGS-BOOT}

default.xbe : default.elf
	${OBJCOPY} --output-target=binary --strip-all $< $@
	cat image-xbe.bin image-xbe.bin image-xbe.bin image-xbe.bin>> default.xbe
	dd if=/dev/zero bs=1024 count=200 >> default.xbe
	@ls -l $@

clean	:
	rm -rf *.o *~ core *.core ${OBJECTS} ${RESOURCES} image.elf image.bin
	rm -f  *.a _rombios_.c _rombios_.s rombios.s rombios.bin rombios.txt
	rm -f  backdrop.elf default.xbe default.elf image-xbe.bin image-xbe.elf
	rm -f default.xbe default.elf

image.elf : ${OBJECTS} ${RESOURCES}
	${LD} -o $@ ${OBJECTS} ${RESOURCES} ${LDFLAGS}

image-xbe.elf : ${OBJECTS} ${RESOURCES}
	${LD} -o $@ ${OBJECTS} ${RESOURCES} ${LDFLAGS-XBE}

rombios.elf : rombios.bin
	${LD} -r --oformat elf32-i386 -o $@ -T rombios.ld -b binary rombios.bin

amended2bl.elf : amended2bl.bin
	${LD} -r --oformat elf32-i386 -o $@ -T amended2bl.ld -b binary amended2bl.bin

xcodes11.elf : xcodes11.bin
	${LD} -r --oformat elf32-i386 -o $@ -T xcodes11.ld -b binary xcodes11.bin

backdrop.elf : backdrop.jpg
	${LD} -r --oformat elf32-i386 -o $@ -T backdrop.ld -b binary backdrop.jpg


### rules:

%.o	: %.c boot.h consts.h BootFilesystemIso9660.h
	${CC} ${CFLAGS} -o $@ -c $<

%.o	: %.S consts.h
	${CC} -DASSEMBLER ${CFLAGS} -o $@ -c $<

image.bin : image.elf
	${OBJCOPY} --output-target=binary --strip-all $< $@
	@ls -l $@

image-xbe.bin : image-xbe.elf
	${OBJCOPY} --output-target=binary --strip-all $< $@
	@ls -l $@

#     the following send the patched result to a Filtror and starts up the
#     Xbox with the new code
#     you can get lmilk from http://warmcat.com/milksop/milk.html
#	@lmilk -f -p image.bin -q
#    this one is the same but additionally runs terminal emulator
#	@lmilk -f -p image.bin -q -t

install:
	lmilk -f -p image.bin
	lmilk -f -a c0000 -p image.bin -q


bios: rombios.bin


rombios.bin: rombios.c biosconfig.h
	${GCC295} -E $< > _rombios_.c
	${BCC} -o rombios.s -c -D__i86__ -0 _rombios_.c
	sed -e 's/^\.text//' -e 's/^\.data//' rombios.s > _rombios_.s
	as86 _rombios_.s -b rombios.bin -u- -w- -g -0 -j -O -l rombios.txt
	ls -l rombios.bin

