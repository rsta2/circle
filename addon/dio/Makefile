#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= spidio.o

libdio.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
