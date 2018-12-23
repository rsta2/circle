#ifndef _linux_barrier_h
#define _linux_barrier_h

#if RASPPI == 1
#define dsb()		asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory")
#define dmb() 		asm volatile ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory")
#else
#define dsb()		asm volatile ("dsb sy" ::: "memory")
#define dmb() 		asm volatile ("dmb sy" ::: "memory")
#endif

#define wmb		dsb
#define rmb		dmb
#define mb		dsb

#define smp_wmb		wmb
#define smp_rmb		rmb
#define smp_mb		mb

#endif
