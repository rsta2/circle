include ../../EMULib/Rules.Unix

# Depending on your CPU endianess, use either -DLSB_FIRST or -DMSB_FIRST.
# Depending on your X11 display mode, use -DBPP8, -DBPP16, or -DBPP32.
DEFINES+= -DFMSX -DLSB_FIRST -DCONDEBUG -DDEBUG -DNEW_STATES \
	  -DNO_WAVE_INTERPOLATION
CFLAGS += -I$(LIBZ80)
OBJECTS+= $(SHA1) $(FLOPPY) $(FDIDISK) $(HUNT) \
	  $(Z80) $(I8255) $(YM2413) $(AY8910) $(SCC) $(WD1793) \
	  ../fMSX.o ../MSX.o ../V9938.o ../I8251.o ../Patch.o \
	  ../Menu.o Unix.o

all:    fmsx

fmsx:	Makefile $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)
