#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o webserver.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/net/libnet.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

CONTENT	= index.h style.h ledoff.h ledon.h favicon.h

EXTRACLEAN = $(CONTENT)

include ../Rules.mk

%.h: %.html
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool $< > $@

%.h: %.css
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool $< > $@

%.h: %.png
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool -b $< > $@

%.h: %.ico
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool -b $< > $@

webserver.o: $(CONTENT)

-include $(DEPS)
