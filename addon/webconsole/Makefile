#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= webconsole.o logbuffer.o

libwebconsole.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
