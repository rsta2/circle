#ifndef _linux_delay_h
#define _linux_delay_h

#ifdef __cplusplus
extern "C" {
#endif

void udelay (unsigned long usecs);

void msleep (unsigned msecs);

#ifdef __cplusplus
}
#endif

#endif
