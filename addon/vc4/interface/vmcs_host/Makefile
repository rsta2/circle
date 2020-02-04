#
# Makefile
#

CIRCLEHOME = ../../../..

OBJS	= vc_vchi_gencmd.o vc_vchi_dispmanx.o

libvmcs_host.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

ifeq ($(strip $(AARCH)),64)
$(error AARCH = 64 is not supported here)
endif

-include $(DEPS)
