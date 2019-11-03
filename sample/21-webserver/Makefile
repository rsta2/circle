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

EXTRACLEAN = $(CONTENT) converttool

include ../Rules.mk

%.h: %.html
	@echo "  GEN   $@"
	@./converttool $< > $@

%.h: %.css
	@echo "  GEN   $@"
	@./converttool $< > $@

%.h: %.png
	@echo "  GEN   $@"
	@./converttool -b $< > $@
	
%.h: %.ico
	@echo "  GEN   $@"
	@./converttool -b $< > $@
	
webserver.o: $(CONTENT)

$(CONTENT): converttool

converttool: converttool.c
	@echo "  TOOL  $@"
	@gcc -o converttool converttool.c
