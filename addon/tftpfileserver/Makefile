#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= tftpfileserver.o

libtftpfileserver.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
