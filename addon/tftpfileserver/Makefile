#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= tftpfileserver.o

libtftpfileserver$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
