#ifndef _p9util_h
#define _p9util_h

#include <circle/alloc.h>
#include <circle/util.h>
#include <circle/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define nil			0

#define USED(var)		((void) var)

#define nelem(array)		(sizeof (array) / sizeof ((array)[0]))

#define ROUNDUP(val, num)	(((val)+(num)-1) / (num) * (num))
#define ROUND(val, num)		ROUNDUP(val, num)

#define MIN(a, b)		((a) < (b) ? (a) : (b))
#define MAX(a, b)		((a) > (b) ? (a) : (b))

#define Mhz			1000000U

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;
typedef unsigned long long	uvlong;

typedef u32			u32int;

#define sdmalloc		malloc
#define sdfree			free

#define print	__p9print
#define sprint	__p9sprint
#define snprint	__p9snprint
#define seprint	__p9seprint
int print (const char *fmt, ...);
int sprint (char *str, const char *fmt, ...);
int snprint (char *str, size_t len, const char *fmt, ...);
char *seprint (char *str, char *end, const char *fmt, ...);

#define readstr	__p9readstr
#define READSTR			1000
long readstr (ulong offset, void *buf, size_t len, const void *p);

#define cistrcmp		strcasecmp
#define cistrncmp		strncasecmp

#define hexdump	__p9hexdump
void hexdump (const void *p, size_t len, const char *from);

#ifdef __cplusplus
}
#endif

#endif
