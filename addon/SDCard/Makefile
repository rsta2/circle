#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= emmc.o

all: libsdcard.a

include $(CIRCLEHOME)/Rules.mk

ifneq ($(strip $(RASPPI)),5)
OBJS	+= mmchost.o sdhost.o
endif

libsdcard.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

-include $(DEPS)
