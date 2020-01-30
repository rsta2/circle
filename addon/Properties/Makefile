#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= properties.o propertiesfile.o propertiesfatfsfile.o propertiesbasefile.o

libproperties.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
