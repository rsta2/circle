#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ugui.o uguicpp.o

libugui.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
