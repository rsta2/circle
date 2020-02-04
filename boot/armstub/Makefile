-include ../../Config.mk
-include ../../Config2.mk

PREFIX64 ?= aarch64-none-elf-

TARGET = armstub8-rpi4

$(TARGET).bin: armstub8.S
	$(PREFIX64)gcc -DGIC=1 -o $(TARGET).o -c armstub8.S
	$(PREFIX64)ld --section-start=.text=0 -o $(TARGET).elf $(TARGET).o
	$(PREFIX64)objdump -D $(TARGET).elf > $(TARGET).lst
	$(PREFIX64)objcopy $(TARGET).elf -O binary $(TARGET).bin

clean:
	rm -f *.o *.elf *.bin *.lst
