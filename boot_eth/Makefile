
O_TARGET :=  eth_Startup.o

eth_Startup.o:

%.o     : %.S
	${CC} -DASSEMBLER ${CFLAGS} -o $@ -c $<

#include $(TOPDIR)/Rules.make
