CC	= gcc
INCLUDE = -I$(TOPDIR)/grub -I$(TOPDIR)/include -I$(TOPDIR)/ -I./ -I$(TOPDIR)/fs/cdrom \
	-I$(TOPDIR)/fs/fatx -I$(TOPDIR)/lib/eeprom -I$(TOPDIR)/lib/crypt -I$(TOPDIR)/drivers/usb \
	-I$(TOPDIR)/drivers/video -I$(TOPDIR)/drivers/flash -I$(TOPDIR)/lib/misc \
	-I$(TOPDIR)/fs/grub -I$(TOPDIR)/lib/font -I$(TOPDIR)/lib/jpeg-6b
CFLAGS	= -g -O2 -Wall -Werror $(INCLUDE)
#CFLAGS	= -g -O0 -Wall -Werror $(INCLUDE)
LD      = ld
OBJCOPY = objcopy

TOPDIR  := $(shell /bin/pwd)
SUBDIRS	=	fs drivers lib boot boot_xbe
OBJECTS = $(TOPDIR)/obj/BootStartup.o
OBJECTS += $(TOPDIR)/obj/BootPerformPicChallengeResponseAction.o
OBJECTS += $(TOPDIR)/obj/BootPciPeripheralInitialization.o
OBJECTS += $(TOPDIR)/obj/BootVgaInitialization.o
OBJECTS += $(TOPDIR)/obj/BootResetAction.o
OBJECTS += $(TOPDIR)/obj/BootIde.o
OBJECTS += $(TOPDIR)/obj/BootHddKey.o
OBJECTS += $(TOPDIR)/obj/rc4.o
OBJECTS += $(TOPDIR)/obj/sha1.o
OBJECTS += $(TOPDIR)/obj/BootVideoHelpers.o
OBJECTS += $(TOPDIR)/obj/vsprintf.o
OBJECTS += $(TOPDIR)/obj/filtror.o
OBJECTS += $(TOPDIR)/obj/BootStartBios.o
OBJECTS += $(TOPDIR)/obj/setup.o
OBJECTS += $(TOPDIR)/obj/BootFilesystemIso9660.o
OBJECTS += $(TOPDIR)/obj/BootLibrary.o
OBJECTS += $(TOPDIR)/obj/BootInterrupts.o
OBJECTS += $(TOPDIR)/obj/fsys_reiserfs.o
OBJECTS += $(TOPDIR)/obj/char_io.o
OBJECTS += $(TOPDIR)/obj/disk_io.o
OBJECTS += $(TOPDIR)/obj/jdapimin.o
OBJECTS += $(TOPDIR)/obj/jdapistd.o
OBJECTS += $(TOPDIR)/obj/jdtrans.o
OBJECTS += $(TOPDIR)/obj/jdatasrc.o
OBJECTS += $(TOPDIR)/obj/jdmaster.o
OBJECTS += $(TOPDIR)/obj/jdinput.o
OBJECTS += $(TOPDIR)/obj/jdmarker.o
OBJECTS += $(TOPDIR)/obj/jdhuff.o
OBJECTS += $(TOPDIR)/obj/jdphuff.o
OBJECTS += $(TOPDIR)/obj/jdmainct.o
OBJECTS += $(TOPDIR)/obj/jdcoefct.o
OBJECTS += $(TOPDIR)/obj/jdpostct.o
OBJECTS += $(TOPDIR)/obj/jddctmgr.o
OBJECTS += $(TOPDIR)/obj/jidctfst.o
OBJECTS += $(TOPDIR)/obj/jidctflt.o
OBJECTS += $(TOPDIR)/obj/jidctint.o
OBJECTS += $(TOPDIR)/obj/jidctred.o
OBJECTS += $(TOPDIR)/obj/jdsample.o
OBJECTS += $(TOPDIR)/obj/jdcolor.o
OBJECTS += $(TOPDIR)/obj/jquant1.o
OBJECTS += $(TOPDIR)/obj/jquant2.o
OBJECTS += $(TOPDIR)/obj/jdmerge.o
OBJECTS += $(TOPDIR)/obj/jmemnobs.o
OBJECTS += $(TOPDIR)/obj/jmemmgr.o
OBJECTS += $(TOPDIR)/obj/jcomapi.o
OBJECTS += $(TOPDIR)/obj/jutils.o
OBJECTS += $(TOPDIR)/obj/jerror.o
OBJECTS += $(TOPDIR)/obj/BootFlash.o
OBJECTS += $(TOPDIR)/obj/BootEEPROM.o
OBJECTS += $(TOPDIR)/obj/BootAudio.o
OBJECTS += $(TOPDIR)/obj/BootUsbOhci.o
OBJECTS += $(TOPDIR)/obj/BootParser.o
OBJECTS += $(TOPDIR)/obj/BootFATX.o

LDFLAGS = -s -S -T $(TOPDIR)/scripts/ldscript.ld

RESOURCES = $(TOPDIR)/obj/xcodes11.elf $(TOPDIR)/obj/backdrop.elf

export INCLUDE
export TOPDIR

all: cromwellsubdirs backdrop.elf xcodes11.elf image.elf image.bin

cromwellsubdirs: $(patsubst %, _dir_%, $(SUBDIRS))
$(patsubst %, _dir_%, $(SUBDIRS)) : dummy
	$(MAKE) CFLAGS="$(CFLAGS)" -C $(patsubst _dir_%, %, $@)

dummy:

install:
	lmilk -f -p $(TOPDIR)/image/image.bin
	lmilk -f -a c0000 -p $(TOPDIR)/image/image.bin -q

clean:
	find . \( -name '*.[oas]' -o -name core -o -name '.*.flags' \) -type f -print \
		| grep -v lxdialog/ | xargs rm -f
	rm -f $(TOPDIR)/obj/*.bin rm -f $(TOPDIR)/obj/*.elf
	rm -f $(TOPDIR)/image/*.bin rm -f $(TOPDIR)/image/*.xbe

erase:
	cdrecord -dao -dev=0,0,0 -driveropts=burnfree blank=fast

backdrop.elf:
	${LD} -r --oformat elf32-i386 -o $(TOPDIR)/obj/$@ -T $(TOPDIR)/scripts/backdrop.ld -b binary $(TOPDIR)/pics/backdrop.jpg

xcodes11.elf:
	${LD} -r --oformat elf32-i386 -o $(TOPDIR)/obj/$@ -T $(TOPDIR)/scripts/xcodes11.ld -b binary $(TOPDIR)/bin/xcodes11.bin

image.elf:
	${LD} -o $(TOPDIR)/obj/$@ ${OBJECTS} ${RESOURCES} ${LDFLAGS}

image.bin:
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/image.elf $(TOPDIR)/image/$@
	cat $(TOPDIR)/image/$@ $(TOPDIR)/image/$@ $(TOPDIR)/image/$@ $(TOPDIR)/image/$@ > $(TOPDIR)/image/image_1024.bin
	@ls -l $(TOPDIR)/image/$@

