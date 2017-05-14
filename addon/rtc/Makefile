#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= mcp7941x.o

librtc.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
