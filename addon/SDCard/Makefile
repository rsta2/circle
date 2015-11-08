#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= emmc.o

libsdcard.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
