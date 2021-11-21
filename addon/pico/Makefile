#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= swdloader.o

libpico.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
