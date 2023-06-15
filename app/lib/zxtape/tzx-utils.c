
#include "tzx-utils.h"

// Basic (good enough) implementation of strlwr()
char *strlwr(char str[]) {
	int i = 0;
 
	while (str[i] != '\0') 
	{
    	if (str[i] >= 'A' && str[i] <= 'Z') 
		{
        	str[i] = str[i] + 32;
    	}
      	i++;
	}

	return str;
}


int readfile(byte bytes, unsigned long p) {
	// Only relevant for ORIC tap file, so just do nothing
	return 0;
}

void OricBitWrite() {
	// Only relevant for ORIC tap file, so just do nothing
}

void OricDataBlock() {
	// Only relevant for ORIC tap file, so just do nothing
}

void ReadUEFHeader() {
	// Only relevant for UEF file, so just do nothing
}

void writeUEFData() {
	// Only relevant for UEF file, so just do nothing
}

void UEFCarrierToneBlock() {
	// Only relevant for UEF file, so just do nothing
}



