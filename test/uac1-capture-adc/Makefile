# Circle UAC1 capture experiment
CIRCLEHOME = ../..
OBJS = main.o
DEFINE += -DCAPTURE_ADC -I.
LIBS = $(CIRCLEHOME)/lib/usb/gadget/libusbgadget.a \
       $(CIRCLEHOME)/lib/usb/libusb.a \
       $(CIRCLEHOME)/lib/input/libinput.a \
       $(CIRCLEHOME)/lib/fs/libfs.a \
       $(CIRCLEHOME)/lib/sound/libsound.a \
       $(CIRCLEHOME)/lib/libcircle.a
include ../../sample/Rules.mk
-include $(DEPS)
