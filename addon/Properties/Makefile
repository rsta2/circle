#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= properties.o propertiesfile.o

libproperties.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
