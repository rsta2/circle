#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= vchiqsoundbasedevice.o vchiqsounddevice.o

libvchiqsound.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
