#
# Makefile
#
# The C build process was initially taken
#	from the "collection of low level examples"
# 	which is Copyright (c) 2012 David Welch dwelch@dwelch.com
#

OBJS	= main.o kernel.o

LIBS	= ../lib/libcircle.a

kernel.img: $(OBJS) $(LIBS)
	$(LD) -o kernel.elf -Map kernel.map -T ../circle.ld ../lib/startup.o $(OBJS) $(LIBS)
	$(PREFIX)objdump -D kernel.elf > kernel.lst
	$(PREFIX)objcopy kernel.elf -O binary kernel.img
	wc -c kernel.img

include ../Rules.mk
