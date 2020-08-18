
O_TARGET := vml_Startup.o

vml_Startup.o:

%.o     : %.S
	@${CC} -DASSEMBLER ${CFLAGS} -o $@ -c $<

#include $(TOPDIR)/Rules.make
