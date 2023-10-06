#
# Makefile
#

CIRCLEHOME = ../../..

all: libvchiq.a

include $(CIRCLEHOME)/Rules.mk

OBJS	= vchiqdevice.o \
	  vchiq_arm.o vchiq_2835_arm.o vchiq_core.o vchiq_kern_lib.o vchiq_connected.o \
	  vchiq_shim.o vchiq_util.o

# using coherent memory region
ifeq ($(strip $(AARCH)),32)
CFLAGS += -mno-unaligned-access
else
CFLAGS += -mstrict-align
endif

libvchiq.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

-include $(DEPS)
