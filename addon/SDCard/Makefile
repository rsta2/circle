#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= emmc.o mmchost.o sdhost.o

libsdcard.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
