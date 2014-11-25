#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= OneWire.o

libonewire.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
