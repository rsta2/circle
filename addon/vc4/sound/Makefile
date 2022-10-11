#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= vchiqsoundbasedevice.o vchiqsounddevice.o vchiqsoundcontroller.o

libvchiqsound.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
