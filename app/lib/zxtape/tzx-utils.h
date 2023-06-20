
#ifndef _txz_utils_h
#define _txz_utils_h


#include "tzx-types.h"


char *strlwr(char str[]);
void ReadUEFHeader();
void writeUEFData();
void UEFCarrierToneBlock();
void OricBitWrite();
void OricDataBlock();
int readfile(byte bytes, unsigned long p);



#endif // _tzx_utils_h

/* HD_SPECCYS: END */