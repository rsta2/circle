#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= qemuhostfile.o

libqemusupport.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
