#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/addon/pico/libpico.a \
	  $(CIRCLEHOME)/lib/libcircle.a

FIRMWARE= firmware.h

EXTRACLEAN += $(FIRMWARE)

include $(CIRCLEHOME)/Rules.mk

%.h: %.bin
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool -b $< > $@

kernel.o: $(FIRMWARE)

-include $(DEPS)
