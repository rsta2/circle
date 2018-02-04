#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ugui.o uguicpp.o

libugui$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
