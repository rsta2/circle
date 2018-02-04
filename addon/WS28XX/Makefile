#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ws28xxstripe.o

libws28xx$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
