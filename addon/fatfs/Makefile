#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ff.o diskio.o syscall.o unicode.o

libfatfs.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
