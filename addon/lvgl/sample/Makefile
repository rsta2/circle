#
# Makefile
#

# define only one
#SPI_DISPLAY = DISPLAY_TYPE_ST7789
#SPI_DISPLAY = DISPLAY_TYPE_ILI9341
#I2C_DISPLAY = DISPLAY_TYPE_SSD1306

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o

LIBS	= $(CIRCLEHOME)/addon/lvgl/liblvgl.a \
	  $(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a

ifneq ($(strip $(SPI_DISPLAY)),)
LIBS	+= $(CIRCLEHOME)/addon/display/libdisplay.a

CFLAGS	+= -DSPI_DISPLAY=$(SPI_DISPLAY)
else ifneq ($(strip $(I2C_DISPLAY)),)
LIBS	+= $(CIRCLEHOME)/addon/display/libdisplay.a

CFLAGS	+= -DI2C_DISPLAY=$(I2C_DISPLAY)
endif

include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
