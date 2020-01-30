#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= bcm_host.o

libbcm_host.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

ifeq ($(strip $(AARCH)),64)
$(error AARCH = 64 is not supported here)
endif

-include $(DEPS)
