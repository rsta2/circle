#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= myclass.o

libtempl$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
