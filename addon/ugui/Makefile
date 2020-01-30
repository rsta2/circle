#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ugui.o uguicpp.o

libugui.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
