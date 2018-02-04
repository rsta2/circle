#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= vchiqsoundbasedevice.o vchiqsounddevice.o

libvchiqsound$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
