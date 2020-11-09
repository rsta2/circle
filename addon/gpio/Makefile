#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= rtkgpio.o

libgpio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
