#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= SpectrumScreen.o

libspectrum.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
