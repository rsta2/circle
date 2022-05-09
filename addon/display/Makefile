#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= hd44780device.o st7789display.o hd44780devicei2c.o

libdisplay.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
