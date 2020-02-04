#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= linuxdevice.o linuxemu.o \
	  bug.o completion.o delay.o device.o dma-mapping.o interrupt.o kthread.o \
	  mutex.o platform_device.o printk.o pthread.o raspberrypi-firmware.o rwlock.o \
	  semaphore.o spinlock.o sprintf.o timer.o

liblinuxemu.a: $(OBJS)
	@echo "  AR    $@"
	@rm -f $@
	@$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
