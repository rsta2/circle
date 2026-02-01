#
# Makefile
#

CIRCLEHOME = ../..

all: libnvme.a

include $(CIRCLEHOME)/Rules.mk

ifeq ($(strip $(RASPPI)),5)
OBJS	= nvme.o nvmeprp.o nvmesharedmemallocator.o
endif

libnvme.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

# using coherent memory region
ifeq ($(strip $(AARCH)),32)
CFLAGS += -mno-unaligned-access
else
CFLAGS += -mstrict-align
endif

-include $(DEPS)
