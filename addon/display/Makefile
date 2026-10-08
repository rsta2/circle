#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= hd44780device.o st7789display.o ili9341display.o ssd1306display.o \
	  ssd1309display.o chardevice.o

libdisplay.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
