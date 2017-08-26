#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= hcsr04.o mpu6050.o

libsensor.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
