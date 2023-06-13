
#ifndef _txz_utils_h
#define _txz_utils_h


#include "tzx-types.h"

#define outputPin           9               // Audio Output PIN - Set accordingly to your hardware.
#define maxFilenameLength   100       //Maximum length for long filename support (ideally as large as possible to support very long filenames)
#define LowWrite() {}	// TODO
#define HighWrite() {}  // TODO


char fileName[maxFilenameLength + 1];     //Current filename
uint16_t fileIndex;                    //Index of current file, relative to current directory
extern FILETYPE entry, dir, tmpdir;        // SD card current file (=entry) and current directory (=dir) objects, plus one temporary
unsigned long filesize;             // filesize used for dimensioning AY files
byte outByte=0;

int readfile(byte bytes, unsigned long p);


#endif // _tzx_utils_h

/* HD_SPECCYS: END */