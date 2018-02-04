#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ff.o diskio.o syscall.o unicode.o

libfatfs$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
