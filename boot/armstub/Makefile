-include ../../Config.mk
-include ../../Config2.mk

PREFIX ?= arm-none-eabi-
PREFIX64 ?= aarch64-none-elf-

TARGET = armstub7-rpi4
TARGET64 = armstub8-rpi4

all: $(TARGET).bin $(TARGET64).bin

$(TARGET).bin: armstub7.S
	$(PREFIX)gcc -march=armv7-a -DGIC=1 -DBCM2710=1 -DBCM2711=1 -o $(TARGET).o -c armstub7.S
	$(PREFIX)ld --section-start=.text=0 -o $(TARGET).elf $(TARGET).o
	$(PREFIX)objdump -D $(TARGET).elf > $(TARGET).lst
	$(PREFIX)objcopy $(TARGET).elf -O binary $(TARGET).bin

$(TARGET64).bin: armstub8.S
	$(PREFIX64)gcc -DGIC=1 -o $(TARGET64).o -c armstub8.S
	$(PREFIX64)ld --section-start=.text=0 -o $(TARGET64).elf $(TARGET64).o
	$(PREFIX64)objdump -D $(TARGET64).elf > $(TARGET64).lst
	$(PREFIX64)objcopy $(TARGET64).elf -O binary $(TARGET64).bin

clean:
	rm -f *.o *.elf *.bin *.lst
