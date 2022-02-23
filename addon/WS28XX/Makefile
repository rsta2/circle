#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ws28xxstripe.o ws2812oversmi.o

libws28xx.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
