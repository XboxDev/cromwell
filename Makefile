CC	= gcc
INCLUDE = -I$(TOPDIR)/grub -I$(TOPDIR)/include -I$(TOPDIR)/ -I./ -I$(TOPDIR)/fs/cdrom \
	-I$(TOPDIR)/fs/fatx -I$(TOPDIR)/lib/eeprom -I$(TOPDIR)/lib/crypt -I$(TOPDIR)/drivers/usb \
	-I$(TOPDIR)/drivers/video -I$(TOPDIR)/drivers/flash -I$(TOPDIR)/lib/misc \
	-I$(TOPDIR)/boot_xbe/ -I$(TOPDIR)/fs/grub -I$(TOPDIR)/lib/font -I$(TOPDIR)/lib/jpeg-6b \
	-I$(TOPDIR)/startuploader -I$(TOPDIR)/drivers/cpu


CFLAGS	= -O2 -mcpu=pentium -Wall -Werror $(INCLUDE)
LD      = ld
OBJCOPY = objcopy

export CC

TOPDIR  := $(shell /bin/pwd)
SUBDIRS	= boot_rom fs drivers lib boot 

LDFLAGS-ROM     = -s -S -T $(TOPDIR)/scripts/ldscript-crom.ld
LDFLAGS-XBEBOOT = -s -S -T $(TOPDIR)/scripts/xbeboot.ld
LDFLAGS-ROMBOOT = -s -S -T $(TOPDIR)/boot_rom/bootrom.ld

RESOURCES-ROMBOOT = $(TOPDIR)/obj/xcodes11.elf 

OBJECTS-IMAGEBLD = $(TOPDIR)/bin/imagebld.o
OBJECTS-IMAGEBLD += $(TOPDIR)/bin/sha1.o

OBJECTS-XBE = $(TOPDIR)/boot_xbe/xbeboot.o
                                             
OBJECTS-ROMBOOT = $(TOPDIR)/obj/2bBootStartup.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bPicResponseAction.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bBootStartBios.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/sha1.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bBootLibrary.o
                                             
OBJECTS-CROM = $(TOPDIR)/obj/BootStartup.o
OBJECTS-CROM += $(TOPDIR)/obj/BootResetAction.o
OBJECTS-CROM += $(TOPDIR)/obj/BootPerformPicChallengeResponseAction.o
OBJECTS-CROM += $(TOPDIR)/obj/BootPciPeripheralInitialization.o
OBJECTS-CROM += $(TOPDIR)/obj/BootVgaInitialization.o
OBJECTS-CROM += $(TOPDIR)/obj/BootIde.o
OBJECTS-CROM += $(TOPDIR)/obj/BootHddKey.o
OBJECTS-CROM += $(TOPDIR)/obj/rc4.o
OBJECTS-CROM += $(TOPDIR)/obj/sha1.o
OBJECTS-CROM += $(TOPDIR)/obj/BootVideoHelpers.o
OBJECTS-CROM += $(TOPDIR)/obj/vsprintf.o
OBJECTS-CROM += $(TOPDIR)/obj/filtror.o
OBJECTS-CROM += $(TOPDIR)/obj/BootStartBios.o
OBJECTS-CROM += $(TOPDIR)/obj/setup.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFilesystemIso9660.o
OBJECTS-CROM += $(TOPDIR)/obj/BootLibrary.o
OBJECTS-CROM += $(TOPDIR)/obj/cputools.o
OBJECTS-CROM += $(TOPDIR)/obj/microcode.o
OBJECTS-CROM += $(TOPDIR)/obj/BootInterrupts.o
OBJECTS-CROM += $(TOPDIR)/obj/fsys_reiserfs.o
OBJECTS-CROM += $(TOPDIR)/obj/char_io.o
OBJECTS-CROM += $(TOPDIR)/obj/disk_io.o
OBJECTS-CROM += $(TOPDIR)/obj/jdapimin.o
OBJECTS-CROM += $(TOPDIR)/obj/jdapistd.o
OBJECTS-CROM += $(TOPDIR)/obj/jdtrans.o
OBJECTS-CROM += $(TOPDIR)/obj/jdatasrc.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmaster.o
OBJECTS-CROM += $(TOPDIR)/obj/jdinput.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmarker.o
OBJECTS-CROM += $(TOPDIR)/obj/jdhuff.o
OBJECTS-CROM += $(TOPDIR)/obj/jdphuff.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmainct.o
OBJECTS-CROM += $(TOPDIR)/obj/jdcoefct.o
OBJECTS-CROM += $(TOPDIR)/obj/jdpostct.o
OBJECTS-CROM += $(TOPDIR)/obj/jddctmgr.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctfst.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctflt.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctint.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctred.o
OBJECTS-CROM += $(TOPDIR)/obj/jdsample.o
OBJECTS-CROM += $(TOPDIR)/obj/jdcolor.o
OBJECTS-CROM += $(TOPDIR)/obj/jquant1.o
OBJECTS-CROM += $(TOPDIR)/obj/jquant2.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmerge.o
OBJECTS-CROM += $(TOPDIR)/obj/jmemnobs.o
OBJECTS-CROM += $(TOPDIR)/obj/jmemmgr.o
OBJECTS-CROM += $(TOPDIR)/obj/jcomapi.o
OBJECTS-CROM += $(TOPDIR)/obj/jutils.o
OBJECTS-CROM += $(TOPDIR)/obj/jerror.o
OBJECTS-CROM += $(TOPDIR)/obj/BootAudio.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFlash.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFlashUi.o
OBJECTS-CROM += $(TOPDIR)/obj/BootEEPROM.o
OBJECTS-CROM += $(TOPDIR)/obj/BootParser.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFATX.o
OBJECTS-CROM += $(TOPDIR)/obj/BootUsbOhci.o


RESOURCES = $(TOPDIR)/obj/backdrop.elf

export INCLUDE
export TOPDIR

all: clean xcodes11.elf cromsubdirs image.elf imagebld backdrop.elf image-crom.elf image-crom.bin default.xbe image.bin

cromsubdirs: $(patsubst %, _dir_%, $(SUBDIRS))
$(patsubst %, _dir_%, $(SUBDIRS)) : dummy
	$(MAKE) CFLAGS="$(CFLAGS)" -C $(patsubst _dir_%, %, $@)

dummy:

xcodes11.elf:
	${LD} -r --oformat elf32-i386 -o $(TOPDIR)/obj/$@ -T $(TOPDIR)/boot_rom/xcodes11.ld -b binary $(TOPDIR)/boot_rom/xcodes11.bin

image.elf:
	${LD} -o $(TOPDIR)/obj/$@ ${OBJECTS-ROMBOOT} ${RESOURCES-ROMBOOT} ${LDFLAGS-ROMBOOT}

image.bin:
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/image.elf $(TOPDIR)/image/$@
	$(TOPDIR)/bin/imagebld -rom $(TOPDIR)/image/image.bin $(TOPDIR)/obj/image-crom.bin  $(TOPDIR)/image/image_1024.bin

imagebld:
	gcc $(OBJECTS-IMAGEBLD) -o $(TOPDIR)/bin/imagebld $(INCLUDE)

install:
	lmilk -f -p $(TOPDIR)/image/image.bin
	lmilk -f -a c0000 -p $(TOPDIR)/image/image.bin -q	
clean:
	find . \( -name '*.[oas]' -o -name core -o -name '.*.flags' \) -type f -print \
		| grep -v lxdialog/ | xargs rm -f
	rm -f $(TOPDIR)/obj/*.bin rm -f $(TOPDIR)/obj/*.elf
	rm -f $(TOPDIR)/image/*.bin rm -f $(TOPDIR)/image/*.xbe rm -f $(TOPDIR)/xbe/*.xbe $(TOPDIR)/xbe/*.bin
	rm -f $(TOPDIR)/xbe/*.elf
	rm -f $(TOPDIR)/image/*.bin
	rm -f $(TOPDIR)/bin/imagebld*
	mkdir $(TOPDIR)/obj -p
	mkdir $(TOPDIR)/bin -p
	

backdrop.elf:
	${LD} -r --oformat elf32-i386 -o $(TOPDIR)/obj/$@ -T $(TOPDIR)/scripts/backdrop.ld -b binary $(TOPDIR)/pics/backdrop.jpg

default.elf: ${OBJECTS-XBE}
	${LD} -o $(TOPDIR)/obj/$@ ${OBJECTS-XBE} ${LDFLAGS-XBEBOOT}
	
default.xbe: default.elf
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/default.elf $(TOPDIR)/xbe/$@
	cat $(TOPDIR)/obj/image-crom.bin >> $(TOPDIR)/xbe/default.xbe
	$(TOPDIR)/bin/imagebld -xbe $(TOPDIR)/xbe/default.xbe 
	#mv $(TOPDIR)/out.xbe $(TOPDIR)/xbe/default.xbe -f
	@ls -l $(TOPDIR)/xbe/$@
				
image-crom.elf:
	${LD} -o $(TOPDIR)/obj/$@ ${OBJECTS-CROM} ${RESOURCES} ${LDFLAGS-ROM}

image-crom.bin:
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/image-crom.elf $(TOPDIR)/obj/$@
