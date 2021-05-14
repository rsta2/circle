#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= OneWire.o ds18x20.o

libonewire.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
