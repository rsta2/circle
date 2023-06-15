//
// tzx-types.h
//
// Types to allow compiling TZXDuino for Circle
//
#ifndef _tzx_types_h
#define _tzx_types_h

#include <circle/util.h>

#define HD_SPECCYS_TZXDUINO	1

// Lang definitions
#ifdef __cplusplus
#else
typedef	char				bool;
#define	false				0
#define	true				1
#endif
typedef	unsigned char		byte;		// 8-bit
typedef	unsigned short		word;		// 16-bit
typedef	unsigned short		uint16_t;	// 16-bit
typedef	unsigned long		uint32_t;	// 32-bit
typedef	unsigned long long	uint64_t;	// 64-bit

#define HIGH				1
#define LOW					0
#define INPUT 				0x0
#define OUTPUT 				0x1


#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define word(x0, x1)		(((x0 << 16) & 0xFF00) & (x1 & 0xFF))
#define long(x)				((x) & 0xFFFFFFFF)
#define highByte(x)			((x >> 8) & 0xFF)
#define lowByte(x)			((x) & 0xFF)

// PROG MEM (just treat as normal mem)
#define PROGMEM	// nothing
#define strstr_P(x,y)		strstr(x,y)
#define memcmp_P(x,y,z)		memcmp(x,y,z)
#define PSTR(x)				x
#define pgm_read_byte(x)	(*(x))

// Timer
typedef struct _TIMER {
	void (*initialize)();
	void (*stop)();
	void (*setPeriod)(unsigned long period);
} TIMER;

// Files
typedef char				FsBaseFile;
typedef unsigned			oflag_t;
#define	O_RDONLY			0

typedef struct _FILETYPE {
	bool (*open)(struct _FILETYPE* dir, uint32_t index, oflag_t oflag);
	void (*close)();
	int (*read)(void* buf, unsigned long count);
	bool (*seekSet)(uint64_t pos);	
} FILETYPE;

// Logging
#define printtextF(...)		// nothing as of yet (could print to log)

#endif // _tzx_types_h
