#ifndef _linux_atomic_h
#define _linux_atomic_h

#include <circle/sysconfig.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARM_ALLOW_MULTI_CORE
	#define CIRCLE_MEMMODEL		__ATOMIC_SEQ_CST
#else
	#define CIRCLE_MEMMODEL		__ATOMIC_RELAXED
#endif

typedef struct
{
	volatile int counter;
}
atomic_t;

#define ATOMIC_INIT(val)	{ val }

static inline int atomic_read (atomic_t *v)
{
	return __atomic_load_n (&v->counter, CIRCLE_MEMMODEL);
}

static inline int atomic_set (atomic_t *v, int val)
{
	__atomic_store_n (&v->counter, val, CIRCLE_MEMMODEL);

	return val;
}


// returns previous value
static inline int atomic_xchg (atomic_t *v, int val)
{
	return __atomic_exchange_n (&v->counter, val, CIRCLE_MEMMODEL);
}

// set value "val" if previous value is "cmp"
// returns previous value
static inline int atomic_cmpxchg (atomic_t *v, int cmp, int val)
{
	__atomic_compare_exchange_n (&v->counter, &cmp, val, false, CIRCLE_MEMMODEL, CIRCLE_MEMMODEL);

	return cmp;
}


static inline void atomic_add (int val, atomic_t *v)
{
	__atomic_add_fetch (&v->counter, val, CIRCLE_MEMMODEL);
}

static inline void atomic_sub (int val, atomic_t *v)
{
	__atomic_sub_fetch (&v->counter, val, CIRCLE_MEMMODEL);
}

static inline void atomic_inc (atomic_t *v)
{
	atomic_add (1, v);
}

static inline void atomic_dec (atomic_t *v)
{
	atomic_sub (1, v);
}


static inline int atomic_add_return (int val, atomic_t *v)
{
	return __atomic_add_fetch (&v->counter, val, CIRCLE_MEMMODEL);
}

static inline int atomic_sub_return (int val, atomic_t *v)
{
	return __atomic_sub_fetch (&v->counter, val, CIRCLE_MEMMODEL);
}

static inline int atomic_inc_return (atomic_t *v)
{
	return atomic_add_return (1, v);
}

static inline int atomic_dec_return (atomic_t *v)
{
	return atomic_sub_return (1, v);
}

#ifdef __cplusplus
}
#endif

#endif
