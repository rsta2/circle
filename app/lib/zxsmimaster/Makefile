#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= smimaster.o

libzxsmimaster.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
