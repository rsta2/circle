#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= spidio.o

libdio.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
