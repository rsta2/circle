#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= ff.o diskio.o ffsystem.o ffunicode.o

libfatfs.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
