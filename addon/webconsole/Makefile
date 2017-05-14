#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= webconsole.o logbuffer.o

libwebconsole.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
