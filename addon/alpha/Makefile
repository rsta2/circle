#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= gdbconsole.o fileio.o
CFLAGS += -Wall -Wextra
libaddonalpha.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
