#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= spidio.o

libdio$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
