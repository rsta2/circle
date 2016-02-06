#
# Makefile
#
.SUFFIXES : .c .o
.SUFFIXES : .cpp .o

CIRCLEHOME = ../../..
CIRCLE=1
OBJDIR := obj

CFLAGS = -I ../Z80 -I ../EMULib -I ../fMSX -I ../Z80 -DCIRCLE -DFMSX -DLSB_FIRST -DDEBUG -DNEW_STATES -DNO_WAVE_INTERPOLATION 
LDFALGS = -lc -lg 

VPATH 	= 	../fMSX ../EMULib ../Z80
SRCS 	=  	../fMSX/fMSX.c 		\
			../fMSX/I8251.c 	\
			../fMSX/Menu.c 		\
			../fMSX/MSX.c 		\
			../fMSX/Patch.c		\
			../fMSX/V9938.c		\
			../EMULib/AY8910.c 	\
			../EMULib/EMULib.c 	\
			../EMULib/Floppy.c 	\
			../EMULib/I8255.c  	\
			../EMULib/Record.c 	\
			../EMULib/SCC.c    	\
			../EMULib/SHA1.c 	\
			../EMULib/Sound.c 	\
			../EMULib/WD1793.c 	\
			../EMULib/YM2413.c  \
			../Z80/Z80.c		\
			../Z80/Debug.c		\
			Circle.c			

SRCX	=	kernel.cpp

SRCS := $(notdir $(SRCS))
SRCX := $(notdir $(SRCX))
OBJS  = $(addprefix $(OBJDIR)/, $(SRCS:.c=.o)) 
OBJS += $(addprefix $(OBJDIR)/, $(SRCX:.cpp=.o)) 

	
LIBS =$(CIRCLEHOME)/lib/usb/libusb.a \
	  $(CIRCLEHOME)/lib/input/libinput.a \
	  $(CIRCLEHOME)/lib/fs/fat/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/libfs.a \
	  $(CIRCLEHOME)/lib/libcircle.a 
	  
include ../../Rules.mk

LIBS += $(ARMGCC)/lib/gcc/arm-none-eabi/4.7.4/libgcc.a  $(ARMGCC)/arm-none-eabi/lib/fpu/libg.a

all: $(OBJDIR) $(OBJS) kernel.img

$(OBJDIR):
	mkdir -p $@
	
$(OBJDIR)/%o:%c
	 $(CC) -c $(CFLAGS) -o $@ $<
$(OBJDIR)/%o:%cpp
	 $(CPP) -c $(CPPFLAGS) -o $@ $<	 
	 
depend: $(SRCS)
# 　　$(CC) -M $(CFLAGS) $^ > $@	 

