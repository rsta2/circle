#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= hd44780device.o st7789display.o

libdisplay.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
