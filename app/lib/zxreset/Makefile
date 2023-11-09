#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= zxreset.o

libzxreset.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
