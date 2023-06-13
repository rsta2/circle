//
// tzx-types.h
//
// Types to allow compiling TZXDuino for Circle
//
#ifndef _tzx_types_h
#define _tzx_types_h

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

#define PROGMEM	// nothing

#define HIGH				0x01
#define LOW					0x00
#define INPUT 				0x0
#define OUTPUT 				0x1

#define	O_RDONLY			0

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#define word(x0, x1)		(((x0 << 16) & 0xFF00) & (x1 & 0xFF))
#define long(x)				((x) & 0xFFFFFFFF)

#endif // _tzx_types_h
