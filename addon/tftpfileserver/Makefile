#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= tftpfileserver.o

libtftpfileserver.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
