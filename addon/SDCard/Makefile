#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= emmc.o

libsdcard.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
