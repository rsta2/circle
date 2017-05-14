#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= myclass.o

libtempl.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
