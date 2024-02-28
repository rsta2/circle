#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= emmc.o

ifneq ($(strip $(RASPPI)),5)
OBJS	+= mmchost.o sdhost.o
endif

libsdcard.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
