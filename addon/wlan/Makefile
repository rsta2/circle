#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= bcm4343.o ether4330.o emmc.o \
	  p9arch.o p9chan.o p9cmd.o p9error.o p9ether.o p9proc.o p9util.o

libwlan.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
