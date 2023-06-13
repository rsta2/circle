#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= tzx.o zxtape.o

libzxtape.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
