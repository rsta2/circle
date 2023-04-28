#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= zxutil.o

libzxutil.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
