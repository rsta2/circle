#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= microbitclient.o

libmicrobit.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
