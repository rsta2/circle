#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= myclass.o

libtempl.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
