#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= webconsole.o logbuffer.o

libwebconsole$(RASPPI).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
