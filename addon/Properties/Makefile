#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= properties.o propertiesfile.o

libproperties$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
