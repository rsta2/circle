//
// zxutil.h
//
#ifndef _zxutil_h
#define _zxutil_h

#ifdef __cplusplus
extern "C" {
#endif

void rand_seed(unsigned long seed);
unsigned long rand_32();

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

#ifdef __cplusplus
}
#endif

#endif // _zxutil_h
