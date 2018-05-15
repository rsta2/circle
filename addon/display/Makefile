#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= hd44780device.o

libdisplay.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
