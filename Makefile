#
# $Id$
#
# Shamelessly lifted and hacked from the
# free bios project.
#
# $Log$
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
CFLAGS	= -g -O2 -Wall -Werror
LD	= ld
LDFLAGS	= -s -S -T ldscript.ld
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
OBJECTS	= BootStartup.o BootResetAction.o filtror.o BootPerformPicChallengeResponseAction.o vsprintf.o \
BootPciPeripheralInitialization.o BootPerformXCodeActions.o BootVgaInitialization.o BootIde.o \
BootHddKey.o rc4.o sha1.o BootVideoHelpers.o BootStartBios.o
RESOURCES = rombios.elf

# target:
all	: image.bin

clean	:
	rm -rf *.o *~ core *.core ${OBJECTS} ${RESOURCES} image.elf image.bin
	rm -f  *.a _rombios_.c _rombios_.s rombios.s rombios.bin rombios.txt

image.elf : ${OBJECTS} ${RESOURCES}
	${LD} -o $@ ${OBJECTS} ${RESOURCES} ${LDFLAGS}

rombios.elf : rombios.bin
	${LD} -r --oformat elf32-i386 -o $@ -T rombios.ld -b binary rombios.bin


### rules:
%.o	: %.c boot.h consts.h
	${CC} ${CFLAGS} -o $@ -c $<

%.o	: %.S consts.h
	${CC} -DASSEMBLER ${CFLAGS} -o $@ -c $<

%.bin : %.elf
	${OBJCOPY} --output-target=binary --strip-all $< $@
	@ls -l $@

#     the following send the patched result to a Filtror and starts up the 
#     Xbox with the new code
#     you can get lmilk from http://warmcat.com/milksop/milk.html
#	@lmilk -f -p image.bin -q
#    this one is the same but additionally runs terminal emulator
#	@lmilk -f -p image.bin -q -t

install:
	lmilk -f -p image.bin -q -t


bios: rombios.bin


rombios.bin: rombios.c biosconfig.h
	${GCC295} -E $< > _rombios_.c
	${BCC} -o rombios.s -c -D__i86__ -0 _rombios_.c
	sed -e 's/^\.text//' -e 's/^\.data//' rombios.s > _rombios_.s
	as86 _rombios_.s -b rombios.bin -u- -w- -g -0 -j -O -l rombios.txt
	ls -l rombios.bin

