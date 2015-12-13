#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= SpectrumScreen.o

libspectrum.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
