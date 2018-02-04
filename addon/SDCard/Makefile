#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= emmc.o

libsdcard$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
