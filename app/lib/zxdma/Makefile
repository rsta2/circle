#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= dmacontrolblock.o dmachannel.o dma4channel.o

libzxdma.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
