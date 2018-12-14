#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= properties.o propertiesfile.o

libproperties.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
