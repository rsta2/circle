#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o httpbootserver.o tftpbootserver.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/net/libnet.a \
	  $(CIRCLEHOME)/lib/sched/libsched.a \
	  $(CIRCLEHOME)/lib/libcircle.a

CONTENT	= index.h style.h favicon.h

EXTRACLEAN = $(CONTENT)

include $(CIRCLEHOME)/sample/Rules.mk

%.h: %.html
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool $< > $@

%.h: %.css
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool $< > $@

%.h: %.ico
	@echo "  GEN   $@"
	@$(CIRCLEHOME)/tools/converttool -b $< > $@

httpbootserver.o: $(CONTENT)

-include $(DEPS)
