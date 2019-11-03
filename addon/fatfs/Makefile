#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ff.o diskio.o syscall.o ffunicode.o

libfatfs.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
