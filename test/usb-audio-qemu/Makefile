CIRCLEHOME = ../..
OBJS = main.o
LIBS = $(CIRCLEHOME)/lib/usb/gadget/libusbgadget.a \
       $(CIRCLEHOME)/addon/qemu/libqemusupport.a \
       $(CIRCLEHOME)/lib/libcircle.a
include $(CIRCLEHOME)/sample/Rules.mk
CFLAGS += -I$(CIRCLEHOME)/addon
-include $(DEPS)
