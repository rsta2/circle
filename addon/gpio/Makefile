#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= rtkgpio.o mcp23017.o

libgpio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
