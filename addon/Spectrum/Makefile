#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= SpectrumScreen.o

libspectrum$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
