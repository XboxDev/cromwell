CC	= gcc
INCLUDE = -I$(TOPDIR)/grub -I$(TOPDIR)/include -I$(TOPDIR)/ -I./ -I$(TOPDIR)/fs/cdrom \
	-I$(TOPDIR)/fs/fatx -I$(TOPDIR)/lib/eeprom -I$(TOPDIR)/lib/crypt -I$(TOPDIR)/drivers/usb \
	-I$(TOPDIR)/drivers/video -I$(TOPDIR)/drivers/flash -I$(TOPDIR)/lib/misc \
	-I$(TOPDIR)/fs/grub -I$(TOPDIR)/lib/font -I$(TOPDIR)/lib/jpeg-6b
CFLAGS	= -g -O2 -Wall -Werror $(INCLUDE)
LD      = ld
OBJCOPY = objcopy

TOPDIR  := $(shell /bin/pwd)
SUBDIRS	=	fs drivers lib boot boot_xbe
OBJECTS = $(TOPDIR)/obj/*.o

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

