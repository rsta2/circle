#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= OneWire.o

libonewire$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
