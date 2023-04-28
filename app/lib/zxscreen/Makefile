#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= zxscreen.o

libzxscreen.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
