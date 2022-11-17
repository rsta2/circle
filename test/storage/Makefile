#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	  $(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

EXTRACLEAN = shasum.h testfile.bin

include ../Rules.mk

kernel.o: shasum.h

shasum.h: testfile.bin
	echo "static const char ExpectedSHASum[] = \""`sha256sum -b testfile.bin | cut -d' ' -f1`"\";" > shasum.h

testfile.bin:
	dd if=/dev/urandom of=testfile.bin bs=1M count=256

-include $(DEPS)
