#ifndef _linux_jiffies_h
#define _linux_jiffies_h

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long volatile jiffies;

unsigned long msecs_to_jiffies (const unsigned msecs);

#ifdef __cplusplus
}
#endif

#endif
